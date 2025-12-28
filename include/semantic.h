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
/*                              Symbol Table                                  */
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

    // 默认构造函数（std::map 需要）
    SymbolInfo()
        : type(nullptr), allocaInst(nullptr),
          isConst(false), isGlobal(false), isFunction(false) {}

    SymbolInfo(const std::string &n, llvm::Type *t, llvm::Value *a,
               bool c = false, bool g = false, bool f = false)
        : name(n), type(t), allocaInst(a),
          isConst(c), isGlobal(g), isFunction(f) {}
};

// 符号表（支持嵌套作用域）
class SymbolTable
{
private:
    std::vector<std::map<std::string, SymbolInfo>> scopes;

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
/*                              Loop Context                                  */
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
/*                           Code Generator (Semantic Analyzer)               */
/* -------------------------------------------------------------------------- */

class CodeGenerator
{
private:
    // LLVM 核心组件
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // 符号表
    SymbolTable symbolTable;

    // 当前函数上下文
    llvm::Function *currentFunction;

    // 循环上下文栈（用于 break/continue）
    std::stack<LoopContext> loopStack;

    // 错误信息
    std::vector<std::string> errors;
    bool hasErrors;

    /* ======================== 类型系统辅助函数 ======================== */
    llvm::Type *getLLVMType(const TypeSpec &typeSpec);
    llvm::Type *getArrayType(llvm::Type *elementType, const std::vector<std::unique_ptr<Expr>> &dims);
    llvm::Type *getArrayElementType(llvm::Type *arrayType, size_t indexCount,
                                    const SymbolInfo *symInfo = nullptr);
    llvm::Value *convertToBool(llvm::Value *val);

    /* ======================== 表达式代码生成 ========================== */
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

    /* ======================== 数组处理辅助函数 ======================== */
    llvm::Value *getArrayElementPtr(const LValExpr *lval);
    void initializeArray(llvm::Value *arrayPtr, llvm::Type *arrayType,
                         Expr *initExpr, std::vector<int> &dims, int dimIndex = 0);
    void flattenInitList(InitListExpr *initList, std::vector<llvm::Value *> &values);

    /* ======================== 语句代码生成 ============================ */
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

    /* ======================== 声明代码生成 ============================ */
    void generateDecl(Decl *decl);
    void generateVarDecl(VarDecl *decl);
    void generateGlobalVar(VarDecl *decl, VarDef *varDef, llvm::Type *type);
    void generateLocalVar(VarDecl *decl, VarDef *varDef, llvm::Type *type);

    /* ======================== 函数定义代码生成 ======================== */
    llvm::Function *generateFuncDef(FuncDef *funcDef);
    void generateFuncParams(llvm::Function *func, const std::vector<std::unique_ptr<FuncParam>> &params);

    /* ======================== 错误处理 ================================ */
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
