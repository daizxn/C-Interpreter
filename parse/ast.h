#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>

static void printIndent(int indent)
{
    for (int i = 0; i < indent; i++)
        std::cout << "  ";
}

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
    IdentifierExpr(const std::string &n) : name(std::move(n)) {};
    void dump(int indent) const override;
};

/* --------------------------------- Integer -------------------------------- */
class NumberExpr : public Expr
{
private:
    int value;

public:
    NumberExpr(int v) : value(v) {};
    void dump(int indent) const override;
};

/* --------------------------------- String --------------------------------- */
class StringExpr : public Expr
{
private:
    std::string value;

public:
    StringExpr(const std::string &v) : value(std::move(v)) {};
    void dump(int indent) const override;
};

/* ---------------------------------- char ---------------------------------- */
class CharExpr : public Expr
{
private:
    char value;

public:
    CharExpr(char v) : value(v) {};
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
    LValExpr(const std::string &n) : name(std::move(n)) {}
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
    UnaryExpr(const std::string &o, std::unique_ptr<Expr> r)
        : op(std::move(o)), rhs(std::move(r)) {}
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
    BinaryExpr(std::unique_ptr<Expr> l, const std::string &o,
               std::unique_ptr<Expr> r)
        : op(std::move(o)), lhs(std::move(l)), rhs(std::move(r)) {}
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
    FuncCallExpr(const std::string &n) : name(std::move(n)) {}
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
    void dump(int indent) const override;
};

/* -------------------------------------------------------------------------- */
/*                                Declare class                               */
/* -------------------------------------------------------------------------- */

class Decl : public ASTNode
{
};

#endif