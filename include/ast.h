#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>

static void printIndent(int indent);

struct TypeSpec
{
    enum Kind
    {
        INT,
        CHAR,
        VOID
    } kind;

    bool isConst;

    TypeSpec(Kind k, bool isConstQualified = false) 
        : kind(k), isConst(isConstQualified) {}

    std::string toString() const;

    // 可选：支持从字符串构造
    static TypeSpec fromString(const std::string &s);
};

/* -------------------------------------------------------------------------- */
/*                               AST base class                               */
/* -------------------------------------------------------------------------- */
class ASTNode
{
public:
    virtual ~ASTNode() = default;
    virtual void dump(int indent = 0) const = 0;
};

/* -------------------------------------------------------------------------- */
/*                         Top-level compilation unit                         */
/* -------------------------------------------------------------------------- */
class CompUnit : public ASTNode
{
private:
    std::vector<std::unique_ptr<ASTNode>> units;

public:
    const std::vector<std::unique_ptr<ASTNode>> &getUnits() const { return units; }
    void addUnit(std::unique_ptr<ASTNode> u);
    void dump(int indent) const override;
};

/* -------------------------------------------------------------------------- */
/*                              Expression class                              */
/* -------------------------------------------------------------------------- */
class Expr : public ASTNode
{
};

/* ------------------------------- Identifier ------------------------------- */
class IdentifierExpr : public Expr
{
private:
    std::string name;

public:
    IdentifierExpr(std::string n) : name(std::move(n)) {}
    const std::string &getName() const { return name; }
    void dump(int indent) const override;
};

/* --------------------------------- Integer -------------------------------- */
class NumberExpr : public Expr
{
private:
    int value;

public:
    NumberExpr(int v) : value(v) {}
    int getValue() const { return value; }
    void dump(int indent) const override;
};

/* ---------------------------------- char ---------------------------------- */
class CharExpr : public Expr
{
private:
    char value;

public:
    CharExpr(char v) : value(v) {}
    char getValue() const { return value; }
    void dump(int indent) const override;
};

/* --------------------------------- String --------------------------------- */
class StringExpr : public Expr
{
private:
    std::string value;

public:
    StringExpr(std::string v) : value(std::move(v)) {}
    const std::string &getValue() const { return value; }
    void dump(int indent) const override;
};

/* -------------------------- Initialization list --------------------------- */
class InitListExpr : public Expr
{
private:
    std::vector<std::unique_ptr<Expr>> items;

public:
    void addItem(std::unique_ptr<Expr> e) { items.push_back(std::move(e)); }
    const std::vector<std::unique_ptr<Expr>> &getItems() const { return items; }
    void dump(int indent) const override;
};

/* ---------------------------- Lvalue expression --------------------------- */
class LValExpr : public Expr
{
private:
    std::string name;
    /**
     * 数组下标列表
     * 将[]中的表达式依次存入该列表
     */
    std::vector<std::unique_ptr<Expr>> indices;

public:
    LValExpr(std::string n) : name(std::move(n)) {}
    const std::string &getName() const { return name; }
    const std::vector<std::unique_ptr<Expr>> &getIndices() const { return indices; }
    void addIndex(std::unique_ptr<Expr> e);
    void dump(int indent) const override;
};

/* ---------------------------- Unary expressions --------------------------- */
class UnaryExpr : public Expr
{
private:
    std::string op;
    std::unique_ptr<Expr> rhs;

public:
    UnaryExpr(std::string o, std::unique_ptr<Expr> r)
        : op(std::move(o)), rhs(std::move(r)) {}
    const std::string &getOp() const { return op; }
    const Expr *getRhs() const { return rhs.get(); }
    void dump(int indent) const override;
};

/* --------------------------- Binary expressions --------------------------- */
class BinaryExpr : public Expr
{
private:
    std::string op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

public:
    BinaryExpr(std::unique_ptr<Expr> l, std::string o,
               std::unique_ptr<Expr> r)
        : op(std::move(o)), lhs(std::move(l)), rhs(std::move(r)) {}
    const std::string &getOp() const { return op; }
    const Expr *getLhs() const { return lhs.get(); }
    const Expr *getRhs() const { return rhs.get(); }
    void dump(int indent) const override;
};

/* --------------------------- ternary expressions -------------------------- */
class TernaryExpr : public Expr // 实现三元表达式: cond ? expr1 : expr2
{
private:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> expr1;
    std::unique_ptr<Expr> expr2;

public:
    TernaryExpr(std::unique_ptr<Expr> c, std::unique_ptr<Expr> e1,
                std::unique_ptr<Expr> e2)
        : cond(std::move(c)), expr1(std::move(e1)), expr2(std::move(e2)) {}
    const Expr *getCond() const { return cond.get(); }
    const Expr *getTrueExpr() const { return expr1.get(); }
    const Expr *getFalseExpr() const { return expr2.get(); }
    void dump(int indent) const override;
};

/* ------------------------ Function call expressions ----------------------- */
class FuncCallExpr : public Expr
{
private:
    std::string name;
    /**
     * 函数调用参数列表
     * 将函数调用中的实参依次存入args列表中
     */
    std::vector<std::unique_ptr<Expr>> args;

public:
    FuncCallExpr(std::string n) : name(std::move(n)) {}
    const std::string &getName() const { return name; }
    const std::vector<std::unique_ptr<Expr>> &getArgs() const { return args; }
    void addArg(std::unique_ptr<Expr> e);
    void dump(int indent) const override;
};

/* -------------------------------------------------------------------------- */
/*                              statement class                               */
/* -------------------------------------------------------------------------- */
class Stmt : public ASTNode
{
};

/* -------------------------- Expression statement -------------------------- */
class ExprStmt : public Stmt
{
private:
    std::unique_ptr<Expr> expr;

public:
    ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
    const Expr *getExpr() const { return expr.get(); }
    void dump(int indent) const override;
};

/* --------------------------- Assignment statement ------------------------- */
class AssignStmt : public Stmt
{
private:
    std::unique_ptr<LValExpr> lhs;
    std::unique_ptr<Expr> rhs;

public:
    AssignStmt(std::unique_ptr<LValExpr> l, std::unique_ptr<Expr> r)
        : lhs(std::move(l)), rhs(std::move(r)) {}
    const LValExpr *getLhs() const { return lhs.get(); }
    const Expr *getRhs() const { return rhs.get(); }
    void dump(int indent) const override;
};

/* ------------------------------ Block statement --------------------------- */
class BlockStmt : public Stmt
{
private:
    std::vector<std::unique_ptr<ASTNode>> items; // Decl 或 Stmtc

public:
    const std::vector<std::unique_ptr<ASTNode>> &getItems() const { return items; }
    void addItem(std::unique_ptr<ASTNode> item);
    void dump(int indent) const override;
};

/* ------------------------------- If statement ----------------------------- */
class IfStmt : public Stmt
{
private:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> thenStmt;
    std::unique_ptr<Stmt> elseStmt; // else 语句可选

public:
    IfStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> e = nullptr)
        : cond(std::move(c)), thenStmt(std::move(t)), elseStmt(std::move(e)) {}
    const Expr *getCond() const { return cond.get(); }
    const Stmt *getThenStmt() const { return thenStmt.get(); }
    const Stmt *getElseStmt() const { return elseStmt.get(); }
    void dump(int indent) const override;
};

/* ------------------------------ While statement --------------------------- */
class WhileStmt : public Stmt
{
private:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;

public:
    WhileStmt() {}
    WhileStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> b)
        : cond(std::move(c)), body(std::move(b)) {}
    const Expr *getCond() const { return cond.get(); }
    const Stmt *getBody() const { return body.get(); }
    void dump(int indent) const override;
};

/* ------------------------------ For statement ----------------------------- */
class ForStmt : public Stmt
{
private:
    std::unique_ptr<ASTNode> init; // 可能是 Decl 或 Stmt (ExprStmt/AssignStmt)
    std::unique_ptr<Expr> cond;
    std::unique_ptr<ASTNode> step; // 可能是 Stmt (ExprStmt/AssignStmt)
    std::unique_ptr<Stmt> body;

public:
    ForStmt() {}
    ForStmt(std::unique_ptr<ASTNode> i, std::unique_ptr<Expr> c, std::unique_ptr<ASTNode> s, std::unique_ptr<Stmt> b)
        : init(std::move(i)), cond(std::move(c)), step(std::move(s)), body(std::move(b)) {}
    const ASTNode *getInit() const { return init.get(); }
    const Expr *getCond() const { return cond.get(); }
    const ASTNode *getStep() const { return step.get(); }
    const Stmt *getBody() const { return body.get(); }
    void dump(int indent) const override;
};

/* ----------------------------- Break statement ---------------------------- */
class BreakStmt : public Stmt
{
public:
    void dump(int indent) const override;
};

/* --------------------------- Continue statement --------------------------- */
class ContinueStmt : public Stmt
{
public:
    void dump(int indent) const override;
};

/* ---------------------------- return statement ---------------------------- */
class ReturnStmt : public Stmt
{
private:
    std::unique_ptr<Expr> value;

public:
    ReturnStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
    const Expr *getValue() const { return value.get(); }
    void dump(int indent) const override;
};
/* -------------------------------------------------------------------------- */
/*                                Declare class                               */
/* -------------------------------------------------------------------------- */
class Decl : public ASTNode
{
};

/* ------------------------------- Var define ------------------------------- */
class VarDef : public ASTNode
{
private:
    std::string name;
    /**
     * 数组维度表达式列表
     * 将变量定义中的每个维度表达式依次存入dims列表中
     */
    std::vector<std::unique_ptr<Expr>> dims;
    std::unique_ptr<Expr> init; // 可选的初始化表达式

public:
    VarDef(std::string n) : name(std::move(n)) {}
    const std::string &getName() const { return name; }
    const std::vector<std::unique_ptr<Expr>> &getDims() const { return dims; }
    const Expr *getInit() const { return init.get(); }
    void addDim(std::unique_ptr<Expr> e);
    void setInit(std::unique_ptr<Expr> e);
    void dump(int indent) const override;
};

/* ------------------------------ Var declare ------------------------------ */
class VarDecl : public Decl
{
private:
    TypeSpec type;
    /**
     * 变量定义列表
     * 将变量声明中的每个变量定义依次存入vars列表中
     */
    std::vector<std::unique_ptr<VarDef>> vars;

public:
    VarDecl(TypeSpec t) : type(std::move(t)) {}
    const TypeSpec &getType() const { return type; }
    const std::vector<std::unique_ptr<VarDef>> &getVars() const { return vars; }
    void addVar(std::unique_ptr<VarDef> v);
    void dump(int indent) const override;
};

/* -------------------------------------------------------------------------- */
/*                           Function declare class                           */
/* -------------------------------------------------------------------------- */

/* --------------------------- Function parameters -------------------------- */
class FuncParam : public ASTNode
{
private:
    TypeSpec type;
    std::string name;
    bool isArray = false;                    // 是否为数组参数
    std::vector<std::unique_ptr<Expr>> dims; // 数组维度表达式列表
public:
    FuncParam(TypeSpec t, std::string n)
        : type(std::move(t)), name(std::move(n)) {}
    const TypeSpec &getType() const { return type; }
    const std::string &getName() const { return name; }
    bool getIsArray() const { return isArray; }
    const std::vector<std::unique_ptr<Expr>> &getDims() const { return dims; }
    void setArray();
    void addDim(std::unique_ptr<Expr> e);
    void dump(int indent) const override;
};

/* --------------------------- Function definition ---------------------------- */
class FuncDef : public ASTNode
{
private:
    TypeSpec returnType;
    std::string name;
    /**
     * 函数参数列表
     * 将函数定义中的每个参数依次存入params列表中
     */
    std::vector<std::unique_ptr<FuncParam>> params;
    std::unique_ptr<BlockStmt> body;

public:
    FuncDef(TypeSpec retType, std::string n)
        : returnType(std::move(retType)), name(std::move(n)) {}
    const TypeSpec &getReturnType() const { return returnType; }
    const std::string &getName() const { return name; }
    const std::vector<std::unique_ptr<FuncParam>> &getParams() const { return params; }
    const BlockStmt *getBody() const { return body.get(); }
    void addParam(std::unique_ptr<FuncParam> p);
    void setBody(std::unique_ptr<BlockStmt> b);
    void dump(int indent) const override;
};

#endif