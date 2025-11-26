#include "parser.h"
#include <sstream>

/* ========================================================================== */
/*                          Constructor & Entry Point                         */
/* ========================================================================== */

Parser::Parser(Lexer &lexer)
    : lexer_(lexer),
      current_(TokenType::TOK_EOF, "", SourceLocation()),
      hasErrors_(false)
{
    advance(); // 读取第一个token
}

std::unique_ptr<CompUnit> Parser::parse()
{
    return parseCompUnit();
}

/* ========================================================================== */
/*                              Token Management                              */
/* ========================================================================== */

void Parser::advance()
{
    current_ = lexer_.nextToken();
}

bool Parser::match(TokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const
{
    return current_.type == type;
}

Token Parser::consume(TokenType type, const std::string &errorMsg)
{
    if (check(type))
    {
        Token tok = current_;
        advance();
        return tok;
    }
    error(errorMsg);
    return current_;
}

void Parser::synchronize()
{
    advance();
    while (!check(TokenType::TOK_EOF))
    {
        // 在语句或声明的开始处停止
        if (current_.type == TokenType::TOK_SEMICOLON)
        {
            advance();
            return;
        }

        switch (current_.type)
        {
        case TokenType::TOK_INT:
        case TokenType::TOK_CHAR:
        case TokenType::TOK_VOID:
        case TokenType::TOK_IF:
        case TokenType::TOK_WHILE:
        case TokenType::TOK_FOR:
        case TokenType::TOK_RETURN:
            return;
        default:
            advance();
        }
    }
}

/* ========================================================================== */
/*                               Error Handling                               */
/* ========================================================================== */

void Parser::error(const std::string &message)
{
    hasErrors_ = true;
    std::ostringstream oss;
    oss << "Error at line " << current_.location.line
        << ", column " << current_.location.column
        << ": " << message;
    errors_.push_back(oss.str());
}

/* ========================================================================== */
/*                          CompUnit (Top Level)                              */
/* ========================================================================== */

// CompUnit ::= { Decl | FuncDef } EOF
std::unique_ptr<CompUnit> Parser::parseCompUnit()
{
    auto compUnit = std::make_unique<CompUnit>();

    while (!check(TokenType::TOK_EOF))
    {
        if (!isTypeSpec() && !check(TokenType::TOK_CONST))
        {
            error("Expected type specifier or const");
            synchronize();
            continue;
        }

        // 向前看判断是 Decl 还是 FuncDef
        // 需要看 TypeSpec IDENT 后面是 "(" 还是其他
        TypeSpec type = parseTypeSpec();
        Token name = consume(TokenType::TOK_IDENTIFIER, "Expected identifier");

        if (check(TokenType::TOK_LPAREN))
        {
            // FuncDef ::= TypeSpec IDENT "(" [ FuncParams ] ")" Block
            auto funcDef = parseFuncDef(type, name.lexeme);
            if (funcDef)
                compUnit->addUnit(std::move(funcDef));
        }
        else
        {
            // Decl ::= TypeSpec InitDeclList ";"
            auto decl = parseDecl(type, name.lexeme);
            if (decl)
                compUnit->addUnit(std::move(decl));
        }
    }

    return compUnit;
}

/* ========================================================================== */
/*                              Declarations                                  */
/* ========================================================================== */

// TypeSpec ::= ["const"] ("int" | "char" | "void")
TypeSpec Parser::parseTypeSpec()
{
    bool isConst = false;

    // 检查是否有 const 修饰符
    if (match(TokenType::TOK_CONST))
    {
        isConst = true;
    }

    if (match(TokenType::TOK_INT))
        return TypeSpec(TypeSpec::INT, isConst);
    if (match(TokenType::TOK_CHAR))
        return TypeSpec(TypeSpec::CHAR, isConst);
    if (match(TokenType::TOK_VOID))
        return TypeSpec(TypeSpec::VOID, isConst);

    error("Expected type specifier (int, char, or void)");
    return TypeSpec(TypeSpec::INT, isConst); // 默认返回 int
}

// InitDecl ::= IDENT ArraySuffix? ( "=" InitVal )?
std::unique_ptr<VarDef> Parser::parseVarDef()
{
    Token name = consume(TokenType::TOK_IDENTIFIER, "Expected identifier");
    auto varDef = std::make_unique<VarDef>(name.lexeme);

    // ArraySuffix ::= "[" ConstExp? "]" { "[" ConstExp? "]" }
    while (match(TokenType::TOK_LBRACKET))
    {
        if (!check(TokenType::TOK_RBRACKET))
        {
            auto dim = parseConstExpr();
            varDef->addDim(std::move(dim));
        }
        consume(TokenType::TOK_RBRACKET, "Expected ']'");
    }

    // ( "=" InitVal )?
    if (match(TokenType::TOK_ASSIGN))
    {
        auto initVal = parseInitVal();
        varDef->setInit(std::move(initVal));
    }

    return varDef;
}

// InitVal ::= Exp | "{" [ InitVal { "," InitVal } ] "}"
std::unique_ptr<Expr> Parser::parseInitVal()
{
    if (match(TokenType::TOK_LBRACE))
    {
        // 初始化列表
        auto initList = std::make_unique<InitListExpr>();

        if (!check(TokenType::TOK_RBRACE))
        {
            initList->addItem(parseInitVal());

            while (match(TokenType::TOK_COMMA))
            {
                initList->addItem(parseInitVal());
            }
        }

        consume(TokenType::TOK_RBRACE, "Expected '}'");
        return initList;
    }
    else
    {
        // 普通表达式
        return parseExpr();
    }
}

// Decl ::= TypeSpec InitDeclList ";"
// InitDeclList ::= InitDecl { "," InitDecl }
// type 和第一个 name 已经在 parseCompUnit 中被解析
std::unique_ptr<Decl> Parser::parseDecl(TypeSpec type, const std::string &firstName)
{
    auto decl = std::make_unique<VarDecl>(type);

    // 解析第一个 InitDecl (IDENT 已经被解析为 firstName)
    // InitDecl ::= IDENT ArraySuffix? ( "=" InitVal )?
    auto varDef = std::make_unique<VarDef>(firstName);

    // ArraySuffix ::= "[" ConstExp? "]" { "[" ConstExp? "]" }
    while (match(TokenType::TOK_LBRACKET))
    {
        if (!check(TokenType::TOK_RBRACKET))
        {
            auto dim = parseConstExpr();
            varDef->addDim(std::move(dim));
        }
        consume(TokenType::TOK_RBRACKET, "Expected ']'");
    }

    // ( "=" InitVal )?
    if (match(TokenType::TOK_ASSIGN))
    {
        auto initVal = parseInitVal();
        varDef->setInit(std::move(initVal));
    }

    decl->addVar(std::move(varDef));

    // { "," InitDecl }
    while (match(TokenType::TOK_COMMA))
    {
        // 现在解析完整的 InitDecl，包括 IDENT
        varDef = parseVarDef();
        decl->addVar(std::move(varDef));
    }

    consume(TokenType::TOK_SEMICOLON, "Expected ';' after declaration");
    return decl;
}

/* ========================================================================== */
/*                           Function Definition                              */
/* ========================================================================== */

// FuncDef ::= TypeSpec IDENT "(" [ FuncParams ] ")" Block
// type 和 name 已经在 parseCompUnit 中被解析
std::unique_ptr<FuncDef> Parser::parseFuncDef(TypeSpec returnType, const std::string &name)
{
    auto funcDef = std::make_unique<FuncDef>(returnType, name);

    consume(TokenType::TOK_LPAREN, "Expected '(' after function name");

    // [ FuncParams ]
    // FuncParams ::= FuncParam { "," FuncParam }
    if (!check(TokenType::TOK_RPAREN))
    {
        funcDef->addParam(parseFuncParam());

        while (match(TokenType::TOK_COMMA))
        {
            funcDef->addParam(parseFuncParam());
        }
    }

    consume(TokenType::TOK_RPAREN, "Expected ')' after parameters");

    // Block
    if (check(TokenType::TOK_SEMICOLON))
    {
        error("Function definition missing body");
        return nullptr;
    }
    auto body = parseBlock();
    funcDef->setBody(std::move(body));

    return funcDef;
}

// FuncParam ::= TypeSpec IDENT FuncParamArray?
// FuncParamArray ::= "[" "]" { "[" ConstExp? "]" }
std::unique_ptr<FuncParam> Parser::parseFuncParam()
{
    TypeSpec type = parseTypeSpec();
    Token name = consume(TokenType::TOK_IDENTIFIER, "Expected parameter name");

    auto param = std::make_unique<FuncParam>(type, name.lexeme);

    // FuncParamArray?
    if (match(TokenType::TOK_LBRACKET))
    {
        consume(TokenType::TOK_RBRACKET, "Expected ']' in array parameter");
        param->setArray();

        // { "[" ConstExp? "]" }
        while (match(TokenType::TOK_LBRACKET))
        {
            if (!check(TokenType::TOK_RBRACKET))
            {
                auto dim = parseConstExpr();
                param->addDim(std::move(dim));
            }
            consume(TokenType::TOK_RBRACKET, "Expected ']'");
        }
    }

    return param;
}

/* ========================================================================== */
/*                               Statements                                   */
/* ========================================================================== */

// Block ::= "{" { BlockItem } "}"
// BlockItem ::= Decl | Stmt
std::unique_ptr<BlockStmt> Parser::parseBlock()
{
    consume(TokenType::TOK_LBRACE, "Expected '{'");

    auto block = std::make_unique<BlockStmt>();

    while (!check(TokenType::TOK_RBRACE) && !check(TokenType::TOK_EOF))
    {
        // BlockItem ::= Decl | Stmt
        if (isTypeSpec() || check(TokenType::TOK_CONST))
        {
            // Decl ::= TypeSpec InitDeclList ";"
            TypeSpec type = parseTypeSpec();
            Token name = consume(TokenType::TOK_IDENTIFIER, "Expected identifier");

            auto decl = parseDecl(type, name.lexeme);
            if (decl)
                block->addItem(std::move(decl));
        }
        else
        {
            // Stmt
            auto stmt = parseStmt();
            if (stmt)
                block->addItem(std::move(stmt));
        }
    }

    consume(TokenType::TOK_RBRACE, "Expected '}'");
    return block;
}

// Stmt ::= LVal "=" Exp ";"
//        | [Exp] ";"
//        | Block
//        | "if" "(" Exp ")" Stmt [ "else" Stmt ]
//        | "while" "(" Exp ")" Stmt
//        | "for" "(" ForInit? ";" ForCond? ";" ForLoop? ")" Stmt
//        | "return" [Exp] ";"
//        | "break" ";"
//        | "continue" ";"
std::unique_ptr<Stmt> Parser::parseStmt()
{
    // Block
    if (check(TokenType::TOK_LBRACE))
    {
        return parseBlock();
    }

    // if
    if (check(TokenType::TOK_IF))
    {
        return parseIfStmt();
    }

    // while
    if (check(TokenType::TOK_WHILE))
    {
        return parseWhileStmt();
    }

    // for
    if (check(TokenType::TOK_FOR))
    {
        return parseForStmt();
    }

    // return
    if (check(TokenType::TOK_RETURN))
    {
        return parseReturnStmt();
    }

    // break
    if (check(TokenType::TOK_IDENTIFIER) && current_.lexeme == "break")
    {
        return parseBreakStmt();
    }

    // continue
    if (check(TokenType::TOK_IDENTIFIER) && current_.lexeme == "continue")
    {
        return parseContinueStmt();
    }

    // 空语句: [Exp] ";"
    if (check(TokenType::TOK_SEMICOLON))
    {
        advance();
        return std::make_unique<ExprStmt>(nullptr);
    }

    // LVal "=" Exp ";" 或 [Exp] ";"

    // 尝试解析完整表达式
    auto expr = parseExpr();

    // 检查是否是赋值语句的情况
    // 注意：在解析 Expr 时，赋值运算符 "=" 不会被解析（不在表达式运算符中）
    // 所以如果是 "a = b"，parseExpr() 只会返回 "a"（LValExpr）
    // 然后我们检查下一个 token 是否是 "="

    if (check(TokenType::TOK_ASSIGN))
    {
        // 这是赋值语句：LVal "=" Exp ";"
        // 验证左边是 LVal
        auto lval = dynamic_cast<LValExpr *>(expr.get());
        if (!lval)
        {
            error("Left side of assignment must be an lvalue");
            synchronize();
            return nullptr;
        }

        // 转移所有权
        expr.release();
        std::unique_ptr<LValExpr> lvalPtr(lval);

        advance(); // consume "="
        auto rhs = parseExpr();
        consume(TokenType::TOK_SEMICOLON, "Expected ';' after assignment");

        return std::make_unique<AssignStmt>(std::move(lvalPtr), std::move(rhs));
    }
    else
    {
        // 表达式语句：[Exp] ";"
        consume(TokenType::TOK_SEMICOLON, "Expected ';' after expression");
        return std::make_unique<ExprStmt>(std::move(expr));
    }
}

// "if" "(" Exp ")" Stmt [ "else" Stmt ]
std::unique_ptr<Stmt> Parser::parseIfStmt()
{
    consume(TokenType::TOK_IF, "Expected 'if'");
    consume(TokenType::TOK_LPAREN, "Expected '(' after 'if'");

    auto cond = parseExpr();

    consume(TokenType::TOK_RPAREN, "Expected ')' after condition");

    auto thenStmt = parseStmt();

    std::unique_ptr<Stmt> elseStmt = nullptr;
    if (match(TokenType::TOK_ELSE))
    {
        elseStmt = parseStmt();
    }

    return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), std::move(elseStmt));
}

// "while" "(" Exp ")" Stmt
std::unique_ptr<Stmt> Parser::parseWhileStmt()
{
    consume(TokenType::TOK_WHILE, "Expected 'while'");
    consume(TokenType::TOK_LPAREN, "Expected '(' after 'while'");

    auto cond = parseExpr();

    consume(TokenType::TOK_RPAREN, "Expected ')' after condition");

    auto body = parseStmt();

    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

// "for" "(" ForInit? ";" ForCond? ";" ForLoop? ")" Stmt
// ForInit ::= Decl | Exp | LVal "=" Exp
// ForCond ::= Exp
// ForLoop ::= Exp | LVal "=" Exp
std::unique_ptr<Stmt> Parser::parseForStmt()
{
    consume(TokenType::TOK_FOR, "Expected 'for'");
    consume(TokenType::TOK_LPAREN, "Expected '(' after 'for'");

    // ForInit?
    std::unique_ptr<ASTNode> init = nullptr;
    if (!check(TokenType::TOK_SEMICOLON))
    {
        // ForInit ::= Decl | Exp | LVal "=" Exp
        if (isTypeSpec() || check(TokenType::TOK_CONST))
        {
            // Decl
            TypeSpec type = parseTypeSpec();
            Token name = consume(TokenType::TOK_IDENTIFIER, "Expected identifier");
            init = parseDecl(type, name.lexeme);
            // parseDecl 已经消费了分号
        }
        else
        {
            // Exp | LVal "=" Exp
            auto expr = parseExpr();

            if (check(TokenType::TOK_ASSIGN))
            {
                // LVal "=" Exp
                auto lval = dynamic_cast<LValExpr *>(expr.get());
                if (!lval)
                {
                    error("Left side of assignment must be an lvalue");
                    synchronize();
                    return nullptr;
                }

                expr.release();
                std::unique_ptr<LValExpr> lvalPtr(lval);

                advance(); // consume "="
                auto rhs = parseExpr();
                init = std::make_unique<AssignStmt>(std::move(lvalPtr), std::move(rhs));
            }
            else
            {
                // Exp
                init = std::make_unique<ExprStmt>(std::move(expr));
            }

            consume(TokenType::TOK_SEMICOLON, "Expected ';'");
        }
    }
    else
    {
        consume(TokenType::TOK_SEMICOLON, "Expected ';'");
    }

    // ForCond?
    std::unique_ptr<Expr> cond = nullptr;
    if (!check(TokenType::TOK_SEMICOLON))
    {
        cond = parseExpr();
    }
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after for condition");

    // ForLoop? ::= Exp | LVal "=" Exp
    std::unique_ptr<ASTNode> step = nullptr;
    if (!check(TokenType::TOK_RPAREN))
    {
        auto expr = parseExpr();

        if (check(TokenType::TOK_ASSIGN))
        {
            // LVal "=" Exp
            auto lval = dynamic_cast<LValExpr *>(expr.get());
            if (!lval)
            {
                error("Left side of assignment must be an lvalue");
                synchronize();
                return nullptr;
            }

            expr.release();
            std::unique_ptr<LValExpr> lvalPtr(lval);

            advance(); // consume "="
            auto rhs = parseExpr();
            step = std::make_unique<AssignStmt>(std::move(lvalPtr), std::move(rhs));
        }
        else
        {
            // Exp
            step = std::make_unique<ExprStmt>(std::move(expr));
        }
    }

    consume(TokenType::TOK_RPAREN, "Expected ')' after for clauses");

    auto body = parseStmt();

    return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(step), std::move(body));
}

// "return" [Exp] ";"
std::unique_ptr<Stmt> Parser::parseReturnStmt()
{
    consume(TokenType::TOK_RETURN, "Expected 'return'");

    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::TOK_SEMICOLON))
    {
        value = parseExpr();
    }

    consume(TokenType::TOK_SEMICOLON, "Expected ';' after return");
    return std::make_unique<ReturnStmt>(std::move(value));
}

// "break" ";"
std::unique_ptr<Stmt> Parser::parseBreakStmt()
{
    advance(); // consume "break"
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after break");
    return std::make_unique<BreakStmt>();
}

// "continue" ";"
std::unique_ptr<Stmt> Parser::parseContinueStmt()
{
    advance(); // consume "continue"
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after continue");
    return std::make_unique<ContinueStmt>();
}

/* ========================================================================== */
/*                              Expressions                                   */
/* ========================================================================== */

// Exp ::= ConditionalExp
std::unique_ptr<Expr> Parser::parseExpr()
{
    return parseConditionalExpr();
}

// ConditionalExp ::= LOrExp | LOrExp "?" Exp ":" ConditionalExp
std::unique_ptr<Expr> Parser::parseConditionalExpr()
{
    auto expr = parseLOrExpr();

    if (match(TokenType::TOK_QUESTION))
    {
        auto trueExpr = parseExpr();
        consume(TokenType::TOK_COLON, "Expected ':' in ternary expression");
        auto falseExpr = parseConditionalExpr();

        return std::make_unique<TernaryExpr>(std::move(expr), std::move(trueExpr), std::move(falseExpr));
    }

    return expr;
}

// LOrExp ::= LAndExp { "||" LAndExp }
std::unique_ptr<Expr> Parser::parseLOrExpr()
{
    auto left = parseLAndExpr();

    while (match(TokenType::TOK_LOR))
    {
        auto right = parseLAndExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), "||", std::move(right));
    }

    return left;
}

// LAndExp ::= OrExp { "&&" OrExp }
std::unique_ptr<Expr> Parser::parseLAndExpr()
{
    auto left = parseOrExpr();

    while (match(TokenType::TOK_LAND))
    {
        auto right = parseOrExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), "&&", std::move(right));
    }

    return left;
}

// OrExp ::= XorExp { "|" XorExp }
std::unique_ptr<Expr> Parser::parseOrExpr()
{
    auto left = parseXorExpr();

    while (match(TokenType::TOK_OR))
    {
        auto right = parseXorExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), "|", std::move(right));
    }

    return left;
}

// XorExp ::= AndExp { "^" AndExp }
std::unique_ptr<Expr> Parser::parseXorExpr()
{
    auto left = parseAndExpr();

    while (match(TokenType::TOK_XOR))
    {
        auto right = parseAndExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), "^", std::move(right));
    }

    return left;
}

// AndExp ::= EqExp { "&" EqExp }
std::unique_ptr<Expr> Parser::parseAndExpr()
{
    auto left = parseEqExpr();

    while (match(TokenType::TOK_AND))
    {
        auto right = parseEqExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), "&", std::move(right));
    }

    return left;
}

// EqExp ::= RelExp { ("==" | "!=") RelExp }
std::unique_ptr<Expr> Parser::parseEqExpr()
{
    auto left = parseRelExpr();

    while (check(TokenType::TOK_EQ) || check(TokenType::TOK_NE))
    {
        std::string op = current_.lexeme;
        advance();
        auto right = parseRelExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// RelExp ::= ShiftExp { ("<" | ">" | "<=" | ">=") ShiftExp }
std::unique_ptr<Expr> Parser::parseRelExpr()
{
    auto left = parseShiftExpr();

    while (check(TokenType::TOK_LT) || check(TokenType::TOK_GT) ||
           check(TokenType::TOK_LE) || check(TokenType::TOK_GE))
    {
        std::string op = current_.lexeme;
        advance();
        auto right = parseShiftExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// ShiftExp ::= AddExp { ("<<" | ">>") AddExp }
std::unique_ptr<Expr> Parser::parseShiftExpr()
{
    auto left = parseAddExpr();

    while (check(TokenType::TOK_SHL) || check(TokenType::TOK_SHR))
    {
        std::string op = current_.lexeme;
        advance();
        auto right = parseAddExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// AddExp ::= MulExp { ("+" | "-") MulExp }
std::unique_ptr<Expr> Parser::parseAddExpr()
{
    auto left = parseMulExpr();

    while (check(TokenType::TOK_PLUS) || check(TokenType::TOK_MINUS))
    {
        std::string op = current_.lexeme;
        advance();
        auto right = parseMulExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// MulExp ::= UnaryExp { ("*" | "/" | "%") UnaryExp }
std::unique_ptr<Expr> Parser::parseMulExpr()
{
    auto left = parseUnaryExpr();

    while (check(TokenType::TOK_STAR) || check(TokenType::TOK_SLASH) || check(TokenType::TOK_PERCENT))
    {
        std::string op = current_.lexeme;
        advance();
        auto right = parseUnaryExpr();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

// UnaryExp ::= PrimaryExp
//            | IDENT "(" [ FuncRParams ] ")"
//            | UnaryOp UnaryExp
// UnaryOp ::= "+" | "-" | "!" | "~" | "++" | "--"
std::unique_ptr<Expr> Parser::parseUnaryExpr()
{
    // UnaryOp UnaryExp
    if (isUnaryOp())
    {
        std::string op = current_.lexeme;
        advance();
        auto rhs = parseUnaryExpr();
        return std::make_unique<UnaryExpr>(op, std::move(rhs));
    }

    // IDENT "(" [ FuncRParams ] ")"
    if (check(TokenType::TOK_IDENTIFIER))
    {
        // 向前看一个 token
        Token name = current_;
        advance();

        if (match(TokenType::TOK_LPAREN))
        {
            // 函数调用
            auto funcCall = std::make_unique<FuncCallExpr>(name.lexeme);

            // FuncRParams ::= Exp { "," Exp }
            if (!check(TokenType::TOK_RPAREN))
            {
                funcCall->addArg(parseExpr());

                while (match(TokenType::TOK_COMMA))
                {
                    funcCall->addArg(parseExpr());
                }
            }

            consume(TokenType::TOK_RPAREN, "Expected ')' after arguments");
            return funcCall;
        }
        else
        {
            // 不是函数调用，是 LVal
            // 回退并解析为 PrimaryExp
            auto lval = std::make_unique<LValExpr>(name.lexeme);

            // LVal ::= IDENT { "[" Exp "]" }
            while (match(TokenType::TOK_LBRACKET))
            {
                auto index = parseExpr();
                lval->addIndex(std::move(index));
                consume(TokenType::TOK_RBRACKET, "Expected ']'");
            }

            return lval;
        }
    }

    // PrimaryExp
    return parsePrimaryExpr();
}

// PrimaryExp ::= "(" Exp ")"
//              | LVal
//              | Number
//              | String
//              | CharLiteral
std::unique_ptr<Expr> Parser::parsePrimaryExpr()
{
    // "(" Exp ")"
    if (match(TokenType::TOK_LPAREN))
    {
        auto expr = parseExpr();
        consume(TokenType::TOK_RPAREN, "Expected ')' after expression");
        return expr;
    }

    // Number
    if (check(TokenType::TOK_NUMBER))
    {
        int value = std::stoi(current_.lexeme);
        advance();
        return std::make_unique<NumberExpr>(value);
    }

    // CharLiteral
    if (check(TokenType::TOK_CHAR_LITERAL))
    {
        // 简化处理：取第一个字符
        char value = current_.lexeme.empty() ? '\0' : current_.lexeme[0];
        advance();
        return std::make_unique<CharExpr>(value);
    }

    // String
    if (check(TokenType::TOK_STRING))
    {
        std::string value = current_.lexeme;
        advance();
        return std::make_unique<StringExpr>(value);
    }

    // LVal
    if (check(TokenType::TOK_IDENTIFIER))
    {
        return parseLVal();
    }

    error("Expected expression");
    return nullptr;
}

// LVal ::= IDENT { "[" Exp "]" }
std::unique_ptr<LValExpr> Parser::parseLVal()
{
    Token name = consume(TokenType::TOK_IDENTIFIER, "Expected identifier");
    auto lval = std::make_unique<LValExpr>(name.lexeme);

    // { "[" Exp "]" }
    while (match(TokenType::TOK_LBRACKET))
    {
        auto index = parseExpr();
        lval->addIndex(std::move(index));
        consume(TokenType::TOK_RBRACKET, "Expected ']'");
    }

    return lval;
}

// ConstExp ::= Exp (语法上等同于 Exp，语义上要求是常量表达式)
std::unique_ptr<Expr> Parser::parseConstExpr()
{
    return parseExpr();
}

/* ========================================================================== */
/*                            Auxiliary Methods                               */
/* ========================================================================== */

bool Parser::isTypeSpec() const
{
    return check(TokenType::TOK_INT) ||
           check(TokenType::TOK_CHAR) ||
           check(TokenType::TOK_VOID);
}

bool Parser::isUnaryOp() const
{
    return check(TokenType::TOK_PLUS) ||
           check(TokenType::TOK_MINUS) ||
           check(TokenType::TOK_NOT) ||
           check(TokenType::TOK_TILDE) ||
           check(TokenType::TOK_INC) ||
           check(TokenType::TOK_DEC);
}

std::string Parser::tokenToString(TokenType type) const
{
    // 简化实现
    return std::to_string(static_cast<int>(type));
}
