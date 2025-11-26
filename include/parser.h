#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>
#include <vector>
#include <stdexcept>

/**
 * 递归下降语法分析器
 * 实现 C 语言子集的语法分析，生成抽象语法树
 */
class Parser
{
public:
    explicit Parser(Lexer &lexer);

    /* -------------------------------------------------------------------------- */
    /*                       Parse entry : Compilation unit                       */
    /* -------------------------------------------------------------------------- */
    std::unique_ptr<CompUnit> parse();

    /* -------------------------------------------------------------------------- */
    /*                               Error handling                               */
    /* -------------------------------------------------------------------------- */
    bool hasErrors() const { return hasErrors_; }
    const std::vector<std::string> &getErrors() const { return errors_; }

private:
    Lexer &lexer_;                    // 词法分析器
    Token current_;                   // 当前token
    bool hasErrors_;                  // 是否有错误
    std::vector<std::string> errors_; // 错误信息列表

    /* -------------------------------------------------------------------------- */
    /*                              Token management                              */
    /* -------------------------------------------------------------------------- */

    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token consume(TokenType type, const std::string &errorMsg);
    void synchronize(); // 恢复同步

    /* -------------------------------------------------------------------------- */
    /*                                Error report                                */
    /* -------------------------------------------------------------------------- */

    void error(const std::string &message);

    /* -------------------------------------------------------------------------- */
    /*                          Grammar analysis methods                          */
    /* -------------------------------------------------------------------------- */

    /* ---------------------------- Compilation Unit ---------------------------- */
    std::unique_ptr<CompUnit> parseCompUnit();

    /* ------------------------------- Declarations ----------------------------- */
    std::unique_ptr<Decl> parseDecl(TypeSpec type, const std::string &name);
    TypeSpec parseTypeSpec();
    std::unique_ptr<VarDef> parseVarDef();
    std::unique_ptr<Expr> parseInitVal();

    /* ------------------------------ Function Definition ----------------------- */
    std::unique_ptr<FuncDef> parseFuncDef(TypeSpec returnType, const std::string &name);
    std::unique_ptr<FuncParam> parseFuncParam();

    /* ---------------------------------- Statements ---------------------------- */
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseBreakStmt();
    std::unique_ptr<Stmt> parseContinueStmt();

    /* ------------------- Expressions(from lowest to highest) ------------------ */
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseConditionalExpr(); // 条件表达式 ?:
    std::unique_ptr<Expr> parseLOrExpr();         // 逻辑或 ||
    std::unique_ptr<Expr> parseLAndExpr();        // 逻辑与 &&
    std::unique_ptr<Expr> parseOrExpr();          // 按位或 |
    std::unique_ptr<Expr> parseXorExpr();         // 按位异或 ^
    std::unique_ptr<Expr> parseAndExpr();         // 按位与 &
    std::unique_ptr<Expr> parseEqExpr();          // 相等 == !=
    std::unique_ptr<Expr> parseRelExpr();         // 关系 < <= > >=
    std::unique_ptr<Expr> parseShiftExpr();       // 移位 << >>
    std::unique_ptr<Expr> parseAddExpr();         // 加法 + -
    std::unique_ptr<Expr> parseMulExpr();         // 乘法 * / %
    std::unique_ptr<Expr> parseUnaryExpr();       // 一元运算 + - ! ~
    std::unique_ptr<Expr> parsePrimaryExpr();     // 基本表达式 变量、常量、括号表达式
    std::unique_ptr<LValExpr> parseLVal();        // 左值表达式 变量或数组元素

    /* ------------------------------- Const Expr ------------------------------- */
    std::unique_ptr<Expr> parseConstExpr(); // 常量表达式（语法上等同于Exp）

    /* -------------------------------------------------------------------------- */
    /*                              Auxiliary methods                             */
    /* -------------------------------------------------------------------------- */
    bool isTypeSpec() const;
    bool isUnaryOp() const;
    std::string tokenToString(TokenType type) const;
};

#endif // PARSER_H