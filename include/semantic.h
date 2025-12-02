#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stack>

/* -------------------------------------------------------------------------- */
/*                                Symbol Table                                */
/* -------------------------------------------------------------------------- */

// 符号表条目
struct SymbolInfo
{
    std::string name;
    llvm::Type *type;           // 变量的完整类型（包括数组维度）
    llvm::Value *allocaInst;    // 变量的 alloca 指令（用于局部变量）或全局变量
    bool isConst;               // 是否为常量
    bool isGlobal;              // 是否为全局变量
    bool isFunction;            // 是否为函数
    std::vector<int> arrayDims; // 数组维度信息（用于类型检查）

    // 默认构造函数
    SymbolInfo()
        : type(nullptr), allocaInst(nullptr),
          isConst(false), isGlobal(false), isFunction(false) {}

    SymbolInfo(const std::string &n, llvm::Type *t, llvm::Value *a,
               bool c = false, bool g = false, bool f = false)
        : name(n), type(t), allocaInst(a),
          isConst(c), isGlobal(g), isFunction(f) {}
};

// 符号表
class SymbolTable
{
private:
    std::vector<std::map<std::string, SymbolInfo>> scopes; // 作用域栈

public:
    SymbolTable();

    void enterScope();
    void exitScope();
    bool declare(const std::string &name, const SymbolInfo &info);
    SymbolInfo *lookup(const std::string &name);
    bool isCurrentScopeGlobal() const { return scopes.size() == 1; }
    int getScopeLevel() const { return scopes.size(); }
};

/* -------------------------------------------------------------------------- */
/*                                Loop context                                */
/* -------------------------------------------------------------------------- */

// 循环上下文，用于处理 break/continue
struct LoopContext
{
    llvm::BasicBlock *continueBlock; // continue 跳转目标
    llvm::BasicBlock *breakBlock;    // break 跳转目标

    LoopContext(llvm::BasicBlock *cont, llvm::BasicBlock *brk)
        : continueBlock(cont), breakBlock(brk) {}
};

/* -------------------------------------------------------------------------- */
/*                               Code Generator                               */
/* -------------------------------------------------------------------------- */

class CodeGenerator
{
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    SymbolTable symbolTable;

    std::stack<LoopContext> loopStack;

    llvm::Function *currentFunction;

    std::vector<std::string> errors;
    bool hasErrors;

    /* --------------------- Type system auxiliary functions -------------------- */
    llvm::Type *getLLVMType(const TypeSpec *typeSpec);
    llvm::Type *getArrayType(llvm::Type *elementType,
                             const std::vector<std::unique_ptr<Expr>> &dims);
    llvm::Type *getArrayElementType(llvm::Type *arrayType, size_t indexCount,
                                    const SymbolInfo *symInfo = nullptr);
    llvm::Value *convertToBool(llvm::Value *val); // 将值转换为bool类型，用于判断语句

    /* ----------------------- Expression code generation ----------------------- */
    llvm::Value *generateExpr(Expr *expr);
    llvm::Value *generateNumberExpr(NumberExpr *expr);
    llvm::Value *generateCharExpr(CharExpr *expr);
    llvm::Value *generateStringExpr(StringExpr *expr);
    llvm::Value *generateLValExpr(LValExpr *expr);
    llvm::Value *generateBinaryExpr(BinaryExpr *expr);
    llvm::Value *generateUnaryExpr(UnaryExpr *expr);
    llvm::Value *generateTernaryExpr(TernaryExpr *expr);
    llvm::Value *generateFuncCallExpr(FuncCallExpr *expr);
    llvm::Value *generateInitListExpr(InitListExpr *expr, llvm::Type *targetType);

    /* ------------------ Array processing auxiliary functions ------------------ */
    llvm::Value *getArrayElementPtr(const LValExpr *lval); // 获取数组元素地址，用于处理函数数组参数与数组地址处理
    void initializeArray(llvm::Value *arrayPtr, llvm::Type *arrayType,
                         Expr *initExpr, std::vector<int> &dims, int dimIndex = 0);
    void flattenInitList(InitListExpr *initList, std::vector<llvm::Value *> &values);

    /* ------------------------ Statement code generation ----------------------- */
    void generateStmt(Stmt *stmt);
    void generateExprStmt(ExprStmt *stmt);
    void generateAssignStmt(AssignStmt *stmt);
    void generateBlockStmt(BlockStmt *stmt);
    void generateIfStmt(IfStmt *stmt);
    void generateWhileStmt(WhileStmt *stmt);
    void generateForStmt(ForStmt *stmt);
    void generateReturnStmt(ReturnStmt *stmt);
    void generateBreakStmt(BreakStmt *stmt);
    void generateContinueStmt(ContinueStmt *stmt);

    /* ------------------------- Declare code generation ------------------------ */
    void generateDecl(Decl *decl);
    void generateVarDecl(VarDecl *decl);
    void generateGlobalVar(VarDecl *decl, VarDef *varDef, llvm::Type *type);
    void generateLocalVar(VarDecl *decl, VarDef *varDef, llvm::Type *type);

    /* ------------------- Function definition code generation ------------------ */
    llvm::Function *generateFuncDef(FuncDef *funcDef);
    void generateFuncParams(llvm::Function *func, const std::vector<std::unique_ptr<FuncParam>> &params);

    /* ----------------------------- Error handling ----------------------------- */
    void error(const std::string &message);

public:
    CodeGenerator(const std::string &moduleName);
    ~CodeGenerator();

    // 生成完整编译单元的 IR
    bool generate(CompUnit *compUnit);

    // 获取生成的模块（用于输出 IR）
    llvm::Module *getModule() { return module.get(); }

    // 错误信息
    bool hasError() const { return hasErrors; }
    const std::vector<std::string> &getErrors() const { return errors; }

    // 输出 IR 到字符串
    std::string getIRString();

    // 输出 IR 到文件
    bool writeIRToFile(const std::string &filename);
};

#endif // SEMANTIC_H