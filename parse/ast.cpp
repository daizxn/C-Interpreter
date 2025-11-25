#include "ast.h"

static void printIndent(int indent)
{
    for (int i = 0; i < indent; i++)
        std::cout << "  ";
}

/* -------------------------------- TypeSpec -------------------------------- */
std::string TypeSpec::toString() const
{
    switch (kind)
    {
    case INT:
        return "int";
    case CHAR:
        return "char";
    case VOID:
        return "void";
    default:
        return "unknown";
    }
}

TypeSpec TypeSpec::fromString(const std::string &s)
{
    if (s == "int")
        return TypeSpec(TypeSpec::INT);
    else if (s == "char")
        return TypeSpec(TypeSpec::CHAR);
    else if (s == "void")
        return TypeSpec(TypeSpec::VOID);
    throw std::invalid_argument("Unknown type specifier: " + s);
}

void CompUnit::addUnit(std::unique_ptr<ASTNode> u)
{
    units.push_back(std::move(u));
}

void CompUnit::dump(int indent) const
{
    printIndent(indent);
    std::cout << "CompUnit\n";
    for (const auto &u : units)
        u->dump(indent + 1);
}

void IdentifierExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "Identifier(" << name << ")\n";
}

void NumberExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "Number(" << value << ")\n";
}

void CharExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "Char('" << value << "')\n";
}

void StringExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "String(\"" << value << "\")\n";
}

void InitListExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "InitList\n";
    for (const auto &item : items)
        item->dump(indent + 1);
}

void LValExpr::addIndex(std::unique_ptr<Expr> e)
{
    indices.push_back(std::move(e));
}

void InitListExpr::addItem(std::unique_ptr<Expr> item)
{
    items.push_back(std::move(item));
}

void LValExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "LVal(" << name << ")\n";
    for (const auto &idx : indices)
        idx->dump(indent + 1);
}

void UnaryExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "Unary(" << op << ")\n";
    rhs->dump(indent + 1);
}

void BinaryExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "Binary(" << op << ")\n";
    lhs->dump(indent + 1);
    rhs->dump(indent + 1);
}

void TernaryExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "Ternary\n";
    printIndent(indent + 1);
    std::cout << "Condition:\n";
    cond->dump(indent + 2);
    printIndent(indent + 1);
    std::cout << "Expr1:\n";
    expr1->dump(indent + 2);
    printIndent(indent + 1);
    std::cout << "Expr2:\n";
    expr2->dump(indent + 2);
}

void FuncCallExpr::addArg(std::unique_ptr<Expr> e)
{
    args.push_back(std::move(e));
}

void FuncCallExpr::dump(int indent) const
{
    printIndent(indent);
    std::cout << "FuncCall(" << name << ")\n";
    for (const auto &arg : args)
        arg->dump(indent + 1);
}

void ExprStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "ExprStmt\n";
    expr->dump(indent + 1);
}

void AssignStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "AssignStmt\n";
    lhs->dump(indent + 1);
    rhs->dump(indent + 1);
}

void BlockStmt::addItem(std::unique_ptr<ASTNode> item)
{
    items.push_back(std::move(item));
}

void BlockStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "BlockStmt\n";
    for (const auto &item : items)
        item->dump(indent + 1);
}

void IfStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "IfStmt\n";
    printIndent(indent + 1);
    std::cout << "Condition:\n";
    cond->dump(indent + 2);
    printIndent(indent + 1);
    std::cout << "Then:\n";
    thenStmt->dump(indent + 2);
    if (elseStmt)
    {
        printIndent(indent + 1);
        std::cout << "Else:\n";
        elseStmt->dump(indent + 2);
    }
}

void WhileStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "WhileStmt\n";
    printIndent(indent + 1);
    std::cout << "Condition:\n";
    cond->dump(indent + 2);
    printIndent(indent + 1);
    std::cout << "Body:\n";
    body->dump(indent + 2);
}

void ForStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "ForStmt\n";

    printIndent(indent + 1);
    std::cout << "Init:\n";
    if (init)
        init->dump(indent + 2);

    printIndent(indent + 1);
    std::cout << "Condition:\n";
    if (cond)
        cond->dump(indent + 2);

    printIndent(indent + 1);
    std::cout << "Step:\n";
    if (step)
        step->dump(indent + 2);

    printIndent(indent + 1);
    std::cout << "Body:\n";
    body->dump(indent + 2);
}

void BreakStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "BreakStmt\n";
}

void ContinueStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "ContinueStmt\n";
}

void ReturnStmt::dump(int indent) const
{
    printIndent(indent);
    std::cout << "ReturnStmt\n";
    if (value)
        value->dump(indent + 1);
}

void VarDef::addDim(std::unique_ptr<Expr> e)
{
    dims.push_back(std::move(e));
}

void VarDef::setInit(std::unique_ptr<Expr> e)
{
    init = std::move(e);
}

void VarDef::dump(int indent) const
{
    printIndent(indent);
    std::cout << "VarDef(" << name << ")\n";
    for (const auto &d : dims)
        d->dump(indent + 1);
    if (init)
    {
        printIndent(indent + 1);
        std::cout << "Init:\n";
        init->dump(indent + 2);
    }
}

void VarDecl::addVar(std::unique_ptr<VarDef> v)
{
    vars.push_back(std::move(v));
}

void VarDecl::dump(int indent) const
{
    printIndent(indent);
    std::cout << "VarDecl(" << type.toString() << ")\n";
    for (const auto &v : vars)
        v->dump(indent + 1);
}

void FuncParam::setArray()
{
    isArray = true;
}

void FuncParam::addDim(std::unique_ptr<Expr> e)
{
    dims.push_back(std::move(e));
}

void FuncParam::dump(int indent) const
{
    printIndent(indent);
    std::cout << "FuncParam(" << type.toString() << " " << name << ")\n";
    for (const auto &d : dims)
        d->dump(indent + 1);
}

void FuncDef::addParam(std::unique_ptr<FuncParam> p)
{
    params.push_back(std::move(p));
}

void FuncDef::setBody(std::unique_ptr<BlockStmt> b)
{
    body = std::move(b);
}

void FuncDef::dump(int indent) const
{
    printIndent(indent);
    std::cout << "FuncDef(" << returnType.toString() << " " << name << ")\n";
    printIndent(indent + 1);
    std::cout << "Params:\n";
    for (const auto &param : params)
        param->dump(indent + 2);
    printIndent(indent + 1);
    std::cout << "Body:\n";
    body->dump(indent + 2);
}