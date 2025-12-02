#include "semantic.h.old"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Constants.h>
#include <iostream>
#include <sstream>

/* -------------------------------------------------------------------------- */
/*                                Symbol Table                                */
/* -------------------------------------------------------------------------- */
SymbolTable::SymbolTable()
{
    enterScope(); // 全局作用域
}

void SymbolTable::enterScope()
{
    scopes.emplace_back();
}

void SymbolTable::exitScope()
{
    if (scopes.size() > 1)
    {
        scopes.pop_back();
    }
}

bool SymbolTable::declare(const std::string &name, const SymbolInfo &info)
{
    auto &currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end())
    {
        return false; // 重复声明
    }
    currentScope[name] = info;
    return true;
}

SymbolInfo *SymbolTable::lookup(const std::string &name)
{
    // 从内向外查找
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        auto found = it->find(name);
        if (found != it->end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

/* -------------------------------------------------------------------------- */
/*                               Code Generator                               */
/* -------------------------------------------------------------------------- */

CodeGenerator::CodeGenerator(const std::string &moduleName)
    : hasErrors(false), currentFunction(nullptr)
{
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

CodeGenerator::~CodeGenerator() = default;

void CodeGenerator::error(const std::string &message)
{
    hasErrors = true;
    errors.push_back(message);
    std::cerr << "Semantic Error: " << message << std::endl;
}

/* --------------------- Type system auxiliary functions -------------------- */
llvm::Type *CodeGenerator::getLLVMType(const TypeSpec &typeSpec)
{
    switch (typeSpec.kind)
    {
    case TypeSpec::INT:
        return llvm::Type::getInt32Ty(*context);
    case TypeSpec::CHAR:
        return llvm::Type::getInt8Ty(*context);
    case TypeSpec::VOID:
        return llvm::Type::getVoidTy(*context);
    default:
        error("Unknown type");
        return llvm::Type::getInt32Ty(*context);
    }
}

llvm::Type *CodeGenerator::getArrayType(llvm::Type *elementType,
                                        const std::vector<std::unique_ptr<Expr>> &dims)
{
    llvm::Type *type = elementType;

    // 从右到左构建数组类型（内层到外层）
    for (auto it = dims.rbegin(); it != dims.rend(); ++it)
    {
        if (auto *numExpr = dynamic_cast<NumberExpr *>(it->get()))
        {
            int size = numExpr->getValue();
            if (size <= 0)
            {
                error("Array size must be positive");
                size = 1;
            }
            type = llvm::ArrayType::get(type, size);
        }
        else
        {
            error("Array size must be a constant integer");
            type = llvm::ArrayType::get(type, 1);
        }
    }

    return type;
}

llvm::Type *CodeGenerator::getArrayElementType(llvm::Type *arrayType, size_t indexCount,
                                               const SymbolInfo *symInfo)
{
    llvm::Type *type = arrayType;

    // 剥离 indexCount 层数组类型
    for (size_t i = 0; i < indexCount; ++i)
    {
        if (auto *arrTy = llvm::dyn_cast<llvm::ArrayType>(type))
        {
            // 标准数组类型：直接获取元素类型
            type = arrTy->getElementType();
        }
        else if (type->isPointerTy())
        {
            // 指针类型（函数参数中的数组）
            // 在 opaque pointer 模式下，需要使用符号表信息来确定元素类型

            if (!symInfo || symInfo->arrayDims.empty())
            {
                // 如果没有维度信息，假设是基本类型的指针
                return llvm::Type::getInt32Ty(*context);
            }

            // 根据剩余的维度数量决定元素类型
            size_t remainingDims = symInfo->arrayDims.size() - i - 1;

            if (remainingDims == 0)
            {
                // 最后一维，返回基本类型
                // 假设所有数组元素都是 int 类型
                return llvm::Type::getInt32Ty(*context);
            }
            else
            {
                // 还有更多维度，构建内层数组类型
                llvm::Type *innerType = llvm::Type::getInt32Ty(*context);

                // 从最内层开始构建数组类型
                for (size_t j = symInfo->arrayDims.size() - 1; j > i; --j)
                {
                    if (symInfo->arrayDims[j] > 0)
                    {
                        innerType = llvm::ArrayType::get(innerType, symInfo->arrayDims[j]);
                    }
                    else
                    {
                        // 维度为 0 表示未指定大小（如 arr[]），返回指针
                        return llvm::PointerType::get(*context, 0);
                    }
                }

                return innerType;
            }
        }
        else
        {
            // 如果不是数组类型也不是指针类型，返回 nullptr
            error("Cannot get array element type: not an array or pointer");
            return nullptr;
        }
    }

    return type;
}

// 将任意值转换为 i1 (bool) 类型用于条件判断
llvm::Value *CodeGenerator::convertToBool(llvm::Value *val)
{
    if (!val)
        return nullptr;

    llvm::Type *valType = val->getType();

    // 如果已经是 i1 类型,直接返回
    if (valType->isIntegerTy(1))
        return val;

    // 如果是整数类型,与 0 比较
    if (valType->isIntegerTy())
    {
        llvm::Value *zero = llvm::ConstantInt::get(valType, 0);
        return builder->CreateICmpNE(val, zero, "tobool");
    }

    // 指针类型：与 null 比较
    if (valType->isPointerTy())
    {
        llvm::Value *null = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(valType));
        return builder->CreateICmpNE(val, null, "tobool");
    }

    error("Cannot convert value to boolean");
    return nullptr;
}

/* ----------------------- Expression code generation ----------------------- */
llvm::Value *CodeGenerator::generateExpr(Expr *expr)
{
    if (!expr)
        return nullptr;

    if (auto *numExpr = dynamic_cast<NumberExpr *>(expr))
        return generateNumberExpr(numExpr);
    if (auto *charExpr = dynamic_cast<CharExpr *>(expr))
        return generateCharExpr(charExpr);
    if (auto *strExpr = dynamic_cast<StringExpr *>(expr))
        return generateStringExpr(strExpr);
    if (auto *lvalExpr = dynamic_cast<LValExpr *>(expr))
        return generateLValExpr(lvalExpr);
    if (auto *binExpr = dynamic_cast<BinaryExpr *>(expr))
        return generateBinaryExpr(binExpr);
    if (auto *unExpr = dynamic_cast<UnaryExpr *>(expr))
        return generateUnaryExpr(unExpr);
    if (auto *ternExpr = dynamic_cast<TernaryExpr *>(expr))
        return generateTernaryExpr(ternExpr);
    if (auto *callExpr = dynamic_cast<FuncCallExpr *>(expr))
        return generateFuncCallExpr(callExpr);
    if (auto *initListExpr = dynamic_cast<InitListExpr *>(expr))
    {
        error("InitList expression can only be used in variable initialization");
        return nullptr;
    }

    error("Unknown expression type");
    return nullptr;
}

llvm::Value *CodeGenerator::generateNumberExpr(NumberExpr *expr)
{
    return llvm::ConstantInt::get(*context, llvm::APInt(32, expr->getValue(), true));
}

llvm::Value *CodeGenerator::generateCharExpr(CharExpr *expr)
{
    return llvm::ConstantInt::get(*context, llvm::APInt(8, (uint64_t)(unsigned char)expr->getValue(), false));
}

llvm::Value *CodeGenerator::generateStringExpr(StringExpr *expr)
{
    // 创建全局字符串常量
    return builder->CreateGlobalStringPtr(expr->getValue(), ".str");
}

llvm::Value *CodeGenerator::generateLValExpr(LValExpr *expr)
{
    SymbolInfo *sym = symbolTable.lookup(expr->getName());
    if (!sym)
    {
        error("Undeclared variable: " + expr->getName());
        return nullptr;
    }

    // 如果是数组访问，需要处理索引
    if (!expr->getIndices().empty())
    {
        llvm::Value *elemPtr = getArrayElementPtr(expr);
        if (!elemPtr)
            return nullptr;

        llvm::Type *elemType = llvm::Type::getInt32Ty(*context);

        // 加载数组元素的值
        return builder->CreateLoad(elemType, elemPtr, "arrayelem");
    }

    // 标量变量：加载变量的值
    if (auto *allocaInst = llvm::dyn_cast<llvm::AllocaInst>(sym->allocaInst))
    {
        return builder->CreateLoad(allocaInst->getAllocatedType(), sym->allocaInst, expr->getName());
    }
    else if (auto *globalVar = llvm::dyn_cast<llvm::GlobalVariable>(sym->allocaInst))
    {
        return builder->CreateLoad(globalVar->getValueType(), sym->allocaInst, expr->getName());
    }

    error("Cannot load value from: " + expr->getName());
    return nullptr;
}

llvm::Value *CodeGenerator::getArrayElementPtr(const LValExpr *lval)
{
    SymbolInfo *sym = symbolTable.lookup(lval->getName());
    if (!sym)
    {
        error("Undeclared variable: " + lval->getName());
        return nullptr;
    }

    // 生成索引值
    std::vector<llvm::Value *> indices;

    // 检查是否是函数参数（指针类型）
    bool isPointerParam = sym->type->isPointerTy();

    llvm::Value *basePtr = sym->allocaInst;

    if (isPointerParam)
    {
        // 对于指针类型（函数参数），先加载指针值
        basePtr = builder->CreateLoad(sym->type, sym->allocaInst, lval->getName() + ".ptr");

        // 计算元素类型和索引
        // 对于多维数组参数，需要根据维度信息逐层计算
        size_t numIndices = lval->getIndices().size();

        if (numIndices == 0)
        {
            // 没有索引，返回整个数组指针
            return basePtr;
        }

        // 第一个索引：直接访问最外层
        llvm::Value *firstIndex = generateExpr(const_cast<Expr *>(lval->getIndices()[0].get()));
        if (!firstIndex)
            return nullptr;

        if (numIndices == 1)
        {
            // 只有一个索引
            if (sym->arrayDims.empty() || sym->arrayDims.size() == 1)
            {
                // 一维数组或指针：直接GEP
                llvm::Type *elemType = llvm::Type::getInt32Ty(*context);
                return builder->CreateGEP(elemType, basePtr, firstIndex, "arrayidx");
            }
            else
            {
                // 多维数组，访问第一维，返回子数组指针
                // 需要构建内层数组类型
                llvm::Type *innerType = llvm::Type::getInt32Ty(*context);
                for (size_t i = sym->arrayDims.size() - 1; i > 0; --i)
                {
                    if (sym->arrayDims[i] > 0)
                    {
                        innerType = llvm::ArrayType::get(innerType, sym->arrayDims[i]);
                    }
                }
                return builder->CreateGEP(innerType, basePtr, firstIndex, "arrayidx");
            }
        }
        else
        {
            // 多个索引：需要逐层访问
            llvm::Value *currentPtr = basePtr;
            llvm::Type *currentType = sym->type;

            for (size_t i = 0; i < numIndices; ++i)
            {
                llvm::Value *indexVal = generateExpr(const_cast<Expr *>(lval->getIndices()[i].get()));
                if (!indexVal)
                    return nullptr;

                // 根据剩余维度数量确定元素类型
                size_t remainingDims = sym->arrayDims.size() - i - 1;

                if (remainingDims == 0)
                {
                    // 最后一维：返回基本类型元素
                    llvm::Type *elemType = llvm::Type::getInt32Ty(*context);
                    currentPtr = builder->CreateGEP(elemType, currentPtr, indexVal, "arrayidx");
                }
                else
                {
                    // 还有更多维度：构建内层数组类型
                    llvm::Type *innerType = llvm::Type::getInt32Ty(*context);
                    for (size_t j = sym->arrayDims.size() - 1; j > i; --j)
                    {
                        if (sym->arrayDims[j] > 0)
                        {
                            innerType = llvm::ArrayType::get(innerType, sym->arrayDims[j]);
                        }
                    }
                    currentPtr = builder->CreateGEP(innerType, currentPtr, indexVal, "arrayidx");
                }
            }

            return currentPtr;
        }
    }
    else
    {
        // 对于数组类型（局部/全局数组变量），第一个索引是 0
        indices.push_back(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));

        for (const auto &index : lval->getIndices())
        {
            llvm::Value *indexVal = generateExpr(const_cast<Expr *>(index.get()));
            if (!indexVal)
                return nullptr;
            indices.push_back(indexVal);
        }

        // 使用 GEP 获取数组元素指针
        return builder->CreateGEP(sym->type, basePtr, indices, "arrayidx");
    }
}

llvm::Value *CodeGenerator::generateBinaryExpr(BinaryExpr *expr)
{
    // 对于逻辑运算符 && 和 ||，需要短路求值
    std::string op = expr->getOp();

    if (op == "&&")
    {
        // 短路求值：左侧为 false 时不计算右侧
        llvm::Value *L = generateExpr(const_cast<Expr *>(expr->getLhs()));
        if (!L)
            return nullptr;
        L = convertToBool(L);
        if (!L)
            return nullptr;

        llvm::Function *func = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *rhsBB = llvm::BasicBlock::Create(*context, "and.rhs", func);
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context, "and.merge");

        builder->CreateCondBr(L, rhsBB, mergeBB);

        // RHS 块
        builder->SetInsertPoint(rhsBB);
        llvm::Value *R = generateExpr(const_cast<Expr *>(expr->getRhs()));
        if (!R)
            return nullptr;
        R = convertToBool(R);
        if (!R)
            return nullptr;
        builder->CreateBr(mergeBB);
        rhsBB = builder->GetInsertBlock();

        // Merge 块
        func->insert(func->end(), mergeBB);
        builder->SetInsertPoint(mergeBB);
        llvm::PHINode *phi = builder->CreatePHI(llvm::Type::getInt1Ty(*context), 2, "and.result");
        phi->addIncoming(llvm::ConstantInt::getFalse(*context), func->begin()->getNextNode());
        phi->addIncoming(R, rhsBB);
        return phi;
    }

    if (op == "||")
    {
        // 短路求值：左侧为 true 时不计算右侧
        llvm::Value *L = generateExpr(const_cast<Expr *>(expr->getLhs()));
        if (!L)
            return nullptr;
        L = convertToBool(L);
        if (!L)
            return nullptr;

        llvm::Function *func = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *rhsBB = llvm::BasicBlock::Create(*context, "or.rhs", func);
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context, "or.merge");

        builder->CreateCondBr(L, mergeBB, rhsBB);

        // RHS 块
        builder->SetInsertPoint(rhsBB);
        llvm::Value *R = generateExpr(const_cast<Expr *>(expr->getRhs()));
        if (!R)
            return nullptr;
        R = convertToBool(R);
        if (!R)
            return nullptr;
        builder->CreateBr(mergeBB);
        rhsBB = builder->GetInsertBlock();

        // Merge 块
        func->insert(func->end(), mergeBB);
        builder->SetInsertPoint(mergeBB);
        llvm::PHINode *phi = builder->CreatePHI(llvm::Type::getInt1Ty(*context), 2, "or.result");
        phi->addIncoming(llvm::ConstantInt::getTrue(*context), func->begin()->getNextNode());
        phi->addIncoming(R, rhsBB);
        return phi;
    }

    // 对于其他运算符，正常求值两侧
    llvm::Value *L = generateExpr(const_cast<Expr *>(expr->getLhs()));
    llvm::Value *R = generateExpr(const_cast<Expr *>(expr->getRhs()));

    if (!L || !R)
        return nullptr;

    // 算术运算
    if (op == "+")
        return builder->CreateAdd(L, R, "addtmp");
    if (op == "-")
        return builder->CreateSub(L, R, "subtmp");
    if (op == "*")
        return builder->CreateMul(L, R, "multmp");
    if (op == "/")
        return builder->CreateSDiv(L, R, "divtmp");
    if (op == "%")
        return builder->CreateSRem(L, R, "modtmp");

    // 比较运算
    if (op == "<")
        return builder->CreateICmpSLT(L, R, "cmptmp");
    if (op == ">")
        return builder->CreateICmpSGT(L, R, "cmptmp");
    if (op == "<=")
        return builder->CreateICmpSLE(L, R, "cmptmp");
    if (op == ">=")
        return builder->CreateICmpSGE(L, R, "cmptmp");
    if (op == "==")
        return builder->CreateICmpEQ(L, R, "eqtmp");
    if (op == "!=")
        return builder->CreateICmpNE(L, R, "netmp");

    // 位运算
    if (op == "&")
        return builder->CreateAnd(L, R, "bitand");
    if (op == "|")
        return builder->CreateOr(L, R, "bitor");
    if (op == "^")
        return builder->CreateXor(L, R, "xortmp");
    if (op == "<<")
        return builder->CreateShl(L, R, "shltmp");
    if (op == ">>")
        return builder->CreateAShr(L, R, "ashrtmp");

    error("Unknown binary operator: " + op);
    return nullptr;
}

llvm::Value *CodeGenerator::generateUnaryExpr(UnaryExpr *expr)
{
    llvm::Value *operand = generateExpr(const_cast<Expr *>(expr->getRhs()));
    if (!operand)
        return nullptr;

    std::string op = expr->getOp();

    if (op == "-")
        return builder->CreateNeg(operand, "negtmp");
    if (op == "!")
    {
        llvm::Value *boolVal = convertToBool(operand);
        if (!boolVal)
            return nullptr;
        return builder->CreateNot(boolVal, "nottmp");
    }
    if (op == "~")
        return builder->CreateNot(operand, "bitnot");
    if (op == "+")
        return operand; // 一元加号不做任何操作

    // ++, -- 暂不支持（需要左值）
    if (op == "++" || op == "--")
    {
        error("Prefix increment/decrement not yet supported");
        return nullptr;
    }

    error("Unknown unary operator: " + op);
    return nullptr;
}

llvm::Value *CodeGenerator::generateTernaryExpr(TernaryExpr *expr)
{
    llvm::Value *cond = generateExpr(const_cast<Expr *>(expr->getCond()));
    if (!cond)
        return nullptr;

    // 转换为布尔值
    cond = convertToBool(cond);
    if (!cond)
        return nullptr;

    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*context, "tern.then", func);
    llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*context, "tern.else");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context, "tern.merge");

    builder->CreateCondBr(cond, thenBB, elseBB);

    // Then 分支
    builder->SetInsertPoint(thenBB);
    llvm::Value *thenVal = generateExpr(const_cast<Expr *>(expr->getTrueExpr()));
    if (!thenVal)
        return nullptr;
    builder->CreateBr(mergeBB);
    thenBB = builder->GetInsertBlock();

    // Else 分支
    func->insert(func->end(), elseBB);
    builder->SetInsertPoint(elseBB);
    llvm::Value *elseVal = generateExpr(const_cast<Expr *>(expr->getFalseExpr()));
    if (!elseVal)
        return nullptr;
    builder->CreateBr(mergeBB);
    elseBB = builder->GetInsertBlock();

    // Merge 分支
    func->insert(func->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);
    llvm::PHINode *phi = builder->CreatePHI(thenVal->getType(), 2, "ternary");
    phi->addIncoming(thenVal, thenBB);
    phi->addIncoming(elseVal, elseBB);

    return phi;
}

llvm::Value *CodeGenerator::generateFuncCallExpr(FuncCallExpr *expr)
{
    llvm::Function *calleeF = module->getFunction(expr->getName());
    if (!calleeF)
    {
        error("Unknown function: " + expr->getName());
        return nullptr;
    }

    // 检查参数数量
    if (calleeF->arg_size() != expr->getArgs().size())
    {
        error("Incorrect number of arguments for function: " + expr->getName() +
              " (expected " + std::to_string(calleeF->arg_size()) +
              ", got " + std::to_string(expr->getArgs().size()) + ")");
        return nullptr;
    }

    // 生成参数
    std::vector<llvm::Value *> argsV;
    for (const auto &arg : expr->getArgs())
    {
        llvm::Value *argVal = generateExpr(arg.get());
        if (!argVal)
            return nullptr;
        argsV.push_back(argVal);
    }

    // 对于 void 函数，调用不返回值
    if (calleeF->getReturnType()->isVoidTy())
    {
        return builder->CreateCall(calleeF, argsV);
    }

    return builder->CreateCall(calleeF, argsV, "calltmp");
}

llvm::Value *CodeGenerator::generateInitListExpr(InitListExpr *expr, llvm::Type * /*targetType*/)
{
    // InitList 处理在数组初始化中完成
    // 这里仅用于标量初始化列表（C99 允许）
    if (expr->getItems().size() == 1)
    {
        return generateExpr(const_cast<Expr *>(expr->getItems()[0].get()));
    }

    error("InitList expression used in invalid context");
    return nullptr;
}

/* ------------------ Array processing auxiliary functions ------------------ */
void CodeGenerator::flattenInitList(InitListExpr *initList, std::vector<llvm::Value *> &values)
{
    for (const auto &item : initList->getItems())
    {
        if (auto *nestedList = dynamic_cast<InitListExpr *>(item.get()))
        {
            // 递归处理嵌套初始化列表
            flattenInitList(nestedList, values);
        }
        else
        {
            // 生成表达式值
            llvm::Value *val = generateExpr(item.get());
            if (val)
            {
                values.push_back(val);
            }
        }
    }
}

void CodeGenerator::initializeArray(llvm::Value *arrayPtr, llvm::Type *arrayType,
                                    Expr *initExpr, std::vector<int> &dims, int /*dimIndex*/)
{
    if (!initExpr)
        return;

    // 处理初始化列表
    if (auto *initList = dynamic_cast<InitListExpr *>(initExpr))
    {
        // 展平初始化列表
        std::vector<llvm::Value *> flatValues;
        flattenInitList(initList, flatValues);

        // 计算数组总大小
        int totalSize = 1;
        for (int dim : dims)
        {
            totalSize *= dim;
        }

        // 初始化数组元素
        for (size_t i = 0; i < flatValues.size() && i < (size_t)totalSize; ++i)
        {
            // 计算多维索引
            std::vector<llvm::Value *> indices;
            indices.push_back(llvm::ConstantInt::get(*context, llvm::APInt(32, 0))); // 基址

            int remainder = i;
            for (int d = dims.size() - 1; d >= 0; --d)
            {
                int index = remainder % dims[d];
                remainder /= dims[d];
                indices.insert(indices.begin() + 1,
                               llvm::ConstantInt::get(*context, llvm::APInt(32, index)));
            }

            // 获取元素指针并存储值
            llvm::Value *elemPtr = builder->CreateGEP(arrayType, arrayPtr, indices, "arrayinit");
            builder->CreateStore(flatValues[i], elemPtr);
        }
    }
    else
    {
        // 单个值初始化（标量初始化所有元素）
        llvm::Value *val = generateExpr(initExpr);
        if (!val)
            return;

        // 计算数组总大小
        int totalSize = 1;
        for (int dim : dims)
        {
            totalSize *= dim;
        }

        // 用同一个值初始化所有元素
        for (int i = 0; i < totalSize; ++i)
        {
            // 计算多维索引
            std::vector<llvm::Value *> indices;
            indices.push_back(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));

            int remainder = i;
            for (int d = dims.size() - 1; d >= 0; --d)
            {
                int index = remainder % dims[d];
                remainder /= dims[d];
                indices.insert(indices.begin() + 1,
                               llvm::ConstantInt::get(*context, llvm::APInt(32, index)));
            }

            llvm::Value *elemPtr = builder->CreateGEP(arrayType, arrayPtr, indices, "arrayinit");
            builder->CreateStore(val, elemPtr);
        }
    }
}

/* ------------------------ Statement code generation ----------------------- */
void CodeGenerator::generateStmt(Stmt *stmt)
{
    if (!stmt)
        return;

    if (auto *exprStmt = dynamic_cast<ExprStmt *>(stmt))
        return generateExprStmt(exprStmt);
    if (auto *assignStmt = dynamic_cast<AssignStmt *>(stmt))
        return generateAssignStmt(assignStmt);
    if (auto *blockStmt = dynamic_cast<BlockStmt *>(stmt))
        return generateBlockStmt(blockStmt);
    if (auto *ifStmt = dynamic_cast<IfStmt *>(stmt))
        return generateIfStmt(ifStmt);
    if (auto *whileStmt = dynamic_cast<WhileStmt *>(stmt))
        return generateWhileStmt(whileStmt);
    if (auto *forStmt = dynamic_cast<ForStmt *>(stmt))
        return generateForStmt(forStmt);
    if (auto *retStmt = dynamic_cast<ReturnStmt *>(stmt))
        return generateReturnStmt(retStmt);
    if (auto *breakStmt = dynamic_cast<BreakStmt *>(stmt))
        return generateBreakStmt(breakStmt);
    if (auto *contStmt = dynamic_cast<ContinueStmt *>(stmt))
        return generateContinueStmt(contStmt);

    error("Unknown statement type");
}

void CodeGenerator::generateExprStmt(ExprStmt *stmt)
{
    if (stmt->getExpr())
    {
        generateExpr(const_cast<Expr *>(stmt->getExpr()));
    }
}

void CodeGenerator::generateAssignStmt(AssignStmt *stmt)
{
    const LValExpr *lval = stmt->getLhs();
    SymbolInfo *sym = symbolTable.lookup(lval->getName());
    if (!sym)
    {
        error("Undeclared variable: " + lval->getName());
        return;
    }

    if (sym->isConst)
    {
        error("Cannot assign to const variable: " + lval->getName());
        return;
    }

    llvm::Value *val = generateExpr(const_cast<Expr *>(stmt->getRhs()));
    if (!val)
        return;

    // 检查是否是数组元素赋值
    if (!lval->getIndices().empty())
    {
        llvm::Value *elemPtr = getArrayElementPtr(lval);
        if (elemPtr)
        {
            builder->CreateStore(val, elemPtr);
        }
    }
    else
    {
        // 标量变量赋值
        builder->CreateStore(val, sym->allocaInst);
    }
}

void CodeGenerator::generateBlockStmt(BlockStmt *stmt)
{
    symbolTable.enterScope();

    for (const auto &item : stmt->getItems())
    {
        // 检查当前块是否已经有终止指令
        if (builder->GetInsertBlock()->getTerminator())
        {
            break; // 不再生成后续代码（dead code）
        }

        if (auto *decl = dynamic_cast<Decl *>(item.get()))
        {
            generateDecl(decl);
        }
        else if (auto *s = dynamic_cast<Stmt *>(item.get()))
        {
            generateStmt(s);
        }
    }

    symbolTable.exitScope();
}

void CodeGenerator::generateIfStmt(IfStmt *stmt)
{
    llvm::Value *cond = generateExpr(const_cast<Expr *>(stmt->getCond()));
    if (!cond)
        return;

    // 转换为布尔值
    cond = convertToBool(cond);
    if (!cond)
        return;

    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*context, "then", func);
    llvm::BasicBlock *elseBB = stmt->getElseStmt() ? llvm::BasicBlock::Create(*context, "else") : nullptr;
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context, "ifcont");

    if (elseBB)
        builder->CreateCondBr(cond, thenBB, elseBB);
    else
        builder->CreateCondBr(cond, thenBB, mergeBB);

    // Then 分支
    builder->SetInsertPoint(thenBB);
    generateStmt(const_cast<Stmt *>(stmt->getThenStmt()));
    // 只有在当前块没有终止指令时才添加跳转
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(mergeBB);

    // Else 分支
    if (elseBB)
    {
        func->insert(func->end(), elseBB);
        builder->SetInsertPoint(elseBB);
        generateStmt(const_cast<Stmt *>(stmt->getElseStmt()));
        // 只有在当前块没有终止指令时才添加跳转
        if (!builder->GetInsertBlock()->getTerminator())
            builder->CreateBr(mergeBB);
    }

    // Merge 分支
    func->insert(func->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);
}

void CodeGenerator::generateWhileStmt(WhileStmt *stmt)
{
    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(*context, "while.cond", func);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(*context, "while.body");
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*context, "while.end");

    // 压入循环上下文（用于 break/continue）
    loopStack.push(LoopContext(condBB, afterBB));

    builder->CreateBr(condBB);

    // 条件块
    builder->SetInsertPoint(condBB);
    llvm::Value *cond = generateExpr(const_cast<Expr *>(stmt->getCond()));
    if (!cond)
    {
        loopStack.pop();
        return;
    }
    cond = convertToBool(cond);
    if (!cond)
    {
        loopStack.pop();
        return;
    }
    builder->CreateCondBr(cond, bodyBB, afterBB);

    // 循环体
    func->insert(func->end(), bodyBB);
    builder->SetInsertPoint(bodyBB);
    generateStmt(const_cast<Stmt *>(stmt->getBody()));
    // 只有在当前块没有终止指令时才添加跳转
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(condBB);

    // 循环后
    func->insert(func->end(), afterBB);
    builder->SetInsertPoint(afterBB);

    // 弹出循环上下文
    loopStack.pop();
}

void CodeGenerator::generateForStmt(ForStmt *stmt)
{
    // For 循环：for (init; cond; step) body

    symbolTable.enterScope();

    // 初始化
    if (stmt->getInit())
    {
        if (auto *decl = dynamic_cast<Decl *>(const_cast<ASTNode *>(stmt->getInit())))
        {
            generateDecl(decl);
        }
        else if (auto *s = dynamic_cast<Stmt *>(const_cast<ASTNode *>(stmt->getInit())))
        {
            generateStmt(s);
        }
    }

    llvm::Function *func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(*context, "for.cond", func);
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(*context, "for.body");
    llvm::BasicBlock *stepBB = llvm::BasicBlock::Create(*context, "for.step");
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(*context, "for.end");

    // 压入循环上下文（continue 跳转到 step，break 跳转到 after）
    loopStack.push(LoopContext(stepBB, afterBB));

    builder->CreateBr(condBB);

    // 条件块
    builder->SetInsertPoint(condBB);
    if (stmt->getCond())
    {
        llvm::Value *cond = generateExpr(const_cast<Expr *>(stmt->getCond()));
        if (!cond)
        {
            loopStack.pop();
            symbolTable.exitScope();
            return;
        }
        cond = convertToBool(cond);
        if (!cond)
        {
            loopStack.pop();
            symbolTable.exitScope();
            return;
        }
        builder->CreateCondBr(cond, bodyBB, afterBB);
    }
    else
    {
        builder->CreateBr(bodyBB); // 无条件，无限循环
    }

    // 循环体
    func->insert(func->end(), bodyBB);
    builder->SetInsertPoint(bodyBB);
    generateStmt(const_cast<Stmt *>(stmt->getBody()));
    // 只有在当前块没有终止指令时才添加跳转
    if (!builder->GetInsertBlock()->getTerminator())
        builder->CreateBr(stepBB);

    // 步进
    func->insert(func->end(), stepBB);
    builder->SetInsertPoint(stepBB);
    if (stmt->getStep())
    {
        if (auto *s = dynamic_cast<Stmt *>(const_cast<ASTNode *>(stmt->getStep())))
        {
            generateStmt(s);
        }
    }
    builder->CreateBr(condBB);

    // 循环后
    func->insert(func->end(), afterBB);
    builder->SetInsertPoint(afterBB);

    // 弹出循环上下文
    loopStack.pop();

    symbolTable.exitScope();
}

void CodeGenerator::generateReturnStmt(ReturnStmt *stmt)
{
    if (stmt->getValue())
    {
        llvm::Value *retVal = generateExpr(const_cast<Expr *>(stmt->getValue()));
        if (retVal)
        {
            builder->CreateRet(retVal);
        }
    }
    else
    {
        builder->CreateRetVoid();
    }
}

void CodeGenerator::generateBreakStmt(BreakStmt * /*stmt*/)
{
    if (loopStack.empty())
    {
        error("Break statement outside loop");
        return;
    }

    // 跳转到循环的 break 目标
    builder->CreateBr(loopStack.top().breakBlock);
}

void CodeGenerator::generateContinueStmt(ContinueStmt * /*stmt*/)
{
    if (loopStack.empty())
    {
        error("Continue statement outside loop");
        return;
    }

    // 跳转到循环的 continue 目标
    builder->CreateBr(loopStack.top().continueBlock);
}

/* ------------------------- Declare code generation ------------------------ */

void CodeGenerator::generateDecl(Decl *decl)
{
    if (auto *varDecl = dynamic_cast<VarDecl *>(decl))
    {
        generateVarDecl(varDecl);
    }
}

void CodeGenerator::generateVarDecl(VarDecl *decl)
{
    llvm::Type *baseType = getLLVMType(decl->getType());
    bool isGlobal = symbolTable.isCurrentScopeGlobal();

    for (const auto &varDef : decl->getVars())
    {
        const std::string &name = varDef->getName();
        llvm::Type *type = baseType;
        std::vector<int> arrayDims;

        // 检查是否是数组
        if (!varDef->getDims().empty())
        {
            // 构建数组类型和维度信息
            type = getArrayType(baseType, varDef->getDims());

            // 提取维度值
            for (const auto &dim : varDef->getDims())
            {
                if (auto *numExpr = dynamic_cast<NumberExpr *>(dim.get()))
                {
                    arrayDims.push_back(numExpr->getValue());
                }
                else
                {
                    error("Array size must be constant: " + name);
                    arrayDims.push_back(1);
                }
            }
        }

        if (isGlobal)
        {
            generateGlobalVar(decl, varDef.get(), type);
        }
        else
        {
            generateLocalVar(decl, varDef.get(), type);
        }
    }
}

void CodeGenerator::generateGlobalVar(VarDecl *decl, VarDef *varDef, llvm::Type *type)
{
    const std::string &name = varDef->getName();
    llvm::Constant *initVal = nullptr;

    // 全局变量初始化
    if (varDef->getInit())
    {
        if (varDef->getDims().empty())
        {
            // 标量全局变量
            llvm::Value *val = generateExpr(const_cast<Expr *>(varDef->getInit()));
            if (auto *constVal = llvm::dyn_cast<llvm::Constant>(val))
            {
                initVal = constVal;
            }
            else
            {
                error("Global variable initializer must be constant: " + name);
                initVal = llvm::Constant::getNullValue(type);
            }
        }
        else
        {
            // 数组全局变量初始化（简化处理：使用零初始化）
            initVal = llvm::Constant::getNullValue(type);
            // TODO: 支持全局数组的完整初始化列表
        }
    }
    else
    {
        // 未初始化：使用零初始化
        initVal = llvm::Constant::getNullValue(type);
    }

    // 创建全局变量
    auto *globalVar = new llvm::GlobalVariable(
        *module,
        type,
        decl->getType().isConst, // isConstant
        llvm::GlobalValue::ExternalLinkage,
        initVal,
        name);

    // 注册到符号表
    SymbolInfo info;
    info.name = name;
    info.type = type;
    info.allocaInst = globalVar;
    info.isConst = decl->getType().isConst;
    info.isGlobal = true;
    info.isFunction = false;

    // 保存数组维度信息
    for (const auto &dim : varDef->getDims())
    {
        if (auto *numExpr = dynamic_cast<NumberExpr *>(dim.get()))
        {
            info.arrayDims.push_back(numExpr->getValue());
        }
        else
        {
            info.arrayDims.push_back(0); // 未知维度
        }
    }

    if (!symbolTable.declare(name, info))
    {
        error("Redeclaration of variable: " + name);
    }
}

void CodeGenerator::generateLocalVar(VarDecl *decl, VarDef *varDef, llvm::Type *type)
{
    const std::string &name = varDef->getName();

    // 创建局部变量
    llvm::AllocaInst *alloca = builder->CreateAlloca(type, nullptr, name);

    // 变量初始化
    if (varDef->getInit())
    {
        if (varDef->getDims().empty())
        {
            // 标量变量初始化
            if (auto *initList = dynamic_cast<InitListExpr *>(const_cast<Expr *>(varDef->getInit())))
            {
                // 初始化列表（对标量变量，取第一个元素）
                if (!initList->getItems().empty())
                {
                    llvm::Value *initVal = generateExpr(initList->getItems()[0].get());
                    if (initVal)
                    {
                        builder->CreateStore(initVal, alloca);
                    }
                }
            }
            else
            {
                // 普通表达式初始化
                llvm::Value *initVal = generateExpr(const_cast<Expr *>(varDef->getInit()));
                if (initVal)
                {
                    builder->CreateStore(initVal, alloca);
                }
            }
        }
        else
        {
            // 数组初始化
            std::vector<int> arrayDims;
            for (const auto &dim : varDef->getDims())
            {
                if (auto *numExpr = dynamic_cast<NumberExpr *>(dim.get()))
                {
                    arrayDims.push_back(numExpr->getValue());
                }
            }

            initializeArray(alloca, type, const_cast<Expr *>(varDef->getInit()), arrayDims);
        }
    }
    // 未初始化的局部变量保持未定义状态（LLVM 默认行为）

    // 注册到符号表
    SymbolInfo info;
    info.name = name;
    info.type = type;
    info.allocaInst = alloca;
    info.isConst = decl->getType().isConst;
    info.isGlobal = false;
    info.isFunction = false;

    // 保存数组维度信息
    for (const auto &dim : varDef->getDims())
    {
        if (auto *numExpr = dynamic_cast<NumberExpr *>(dim.get()))
        {
            info.arrayDims.push_back(numExpr->getValue());
        }
        else
        {
            info.arrayDims.push_back(0); // 未知维度
        }
    }

    if (!symbolTable.declare(name, info))
    {
        error("Redeclaration of variable: " + name);
    }
}

/* ------------------- Function definition code generation ------------------ */

llvm::Function *CodeGenerator::generateFuncDef(FuncDef *funcDef)
{
    // 函数返回类型
    llvm::Type *retType = getLLVMType(funcDef->getReturnType());

    // 函数参数类型（考虑数组参数）
    std::vector<llvm::Type *> paramTypes;
    for (const auto &param : funcDef->getParams())
    {
        llvm::Type *paramType = getLLVMType(param->getType());

        // 如果参数是数组，转换为指针类型
        if (param->getIsArray())
        {
            paramType = llvm::PointerType::get(paramType, 0);
        }

        paramTypes.push_back(paramType);
    }

    // 创建函数类型
    llvm::FunctionType *funcType = llvm::FunctionType::get(retType, paramTypes, false);

    // 创建函数
    llvm::Function *func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                                  funcDef->getName(), module.get());

    // 将函数注册到符号表（全局作用域）
    SymbolInfo funcInfo;
    funcInfo.name = funcDef->getName();
    funcInfo.type = funcType;
    funcInfo.allocaInst = func;
    funcInfo.isConst = false;
    funcInfo.isGlobal = true;
    funcInfo.isFunction = true;
    symbolTable.declare(funcDef->getName(), funcInfo);

    // 设置参数名称
    size_t idx = 0;
    for (auto &arg : func->args())
    {
        arg.setName(funcDef->getParams()[idx]->getName());
        idx++;
    }

    // 创建函数入口基本块
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(BB);

    // 进入新作用域
    symbolTable.enterScope();
    currentFunction = func;

    // 为参数创建 alloca 并存储
    generateFuncParams(func, funcDef->getParams());

    // 生成函数体
    generateBlockStmt(const_cast<BlockStmt *>(funcDef->getBody()));

    // 如果函数没有返回语句且返回类型为 void，添加 ret void
    if (retType->isVoidTy() && !builder->GetInsertBlock()->getTerminator())
    {
        builder->CreateRetVoid();
    }

    // 退出作用域
    symbolTable.exitScope();
    currentFunction = nullptr;

    // 验证函数
    if (llvm::verifyFunction(*func, &llvm::errs()))
    {
        error("Function verification failed: " + funcDef->getName());
        func->eraseFromParent();
        return nullptr;
    }

    return func;
}

void CodeGenerator::generateFuncParams(llvm::Function *func,
                                       const std::vector<std::unique_ptr<FuncParam>> &params)
{
    size_t idx = 0;
    for (auto &arg : func->args())
    {
        llvm::AllocaInst *alloca = builder->CreateAlloca(arg.getType(), nullptr, arg.getName());
        builder->CreateStore(&arg, alloca);

        // 注册到符号表
        SymbolInfo paramInfo;
        paramInfo.name = std::string(arg.getName());
        paramInfo.type = arg.getType();
        paramInfo.allocaInst = alloca;
        paramInfo.isConst = false;
        paramInfo.isGlobal = false;
        paramInfo.isFunction = false;

        // 保存数组参数的维度信息
        const FuncParam *param = params[idx].get();
        if (param->getIsArray())
        {
            const auto &arrayDims = param->getDims();

            // 第一维是 []，大小未指定（用 0 表示）
            paramInfo.arrayDims.push_back(0);

            // 后续维度
            for (const auto &dimExpr : arrayDims)
            {
                if (dimExpr)
                {
                    // 如果是常量表达式，计算其值
                    if (auto *numExpr = dynamic_cast<NumberExpr *>(dimExpr.get()))
                    {
                        paramInfo.arrayDims.push_back(numExpr->getValue());
                    }
                    else
                    {
                        // 如果不是字面量，暂时用 0 表示（需要运行时确定）
                        paramInfo.arrayDims.push_back(0);
                    }
                }
                else
                {
                    // 未指定维度
                    paramInfo.arrayDims.push_back(0);
                }
            }
        }

        symbolTable.declare(std::string(arg.getName()), paramInfo);
        idx++;
    }
}

/* --------------------- Top-level generation functions --------------------- */
bool CodeGenerator::generate(CompUnit *compUnit)
{
    for (const auto &unit : compUnit->getUnits())
    {
        if (auto *funcDef = dynamic_cast<FuncDef *>(unit.get()))
        {
            generateFuncDef(funcDef);
        }
        else if (auto *decl = dynamic_cast<Decl *>(unit.get()))
        {
            generateDecl(decl);
        }
    }

    // 验证模块
    if (llvm::verifyModule(*module, &llvm::errs()))
    {
        error("Module verification failed");
        return false;
    }

    return !hasErrors;
}

/* --------------------------- IR output function --------------------------- */
std::string CodeGenerator::getIRString()
{
    std::string str;
    llvm::raw_string_ostream os(str);
    module->print(os, nullptr);
    return os.str();
}

bool CodeGenerator::writeIRToFile(const std::string &filename)
{
    std::error_code EC;
    llvm::raw_fd_ostream file(filename, EC, llvm::sys::fs::OF_None);

    if (EC)
    {
        error("Cannot open file: " + filename);
        return false;
    }

    module->print(file, nullptr);
    return true;
}
