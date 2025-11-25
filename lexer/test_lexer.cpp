#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Token 类型名称映射
std::string getTokenTypeName(TokenType type)
{
    switch (type)
    {
    case TokenType::TOK_EOF:
        return "EOF";
    case TokenType::TOK_INT:
        return "INT";
    case TokenType::TOK_CHAR:
        return "CHAR";
    case TokenType::TOK_VOID:
        return "VOID";
    case TokenType::TOK_IF:
        return "IF";
    case TokenType::TOK_ELSE:
        return "ELSE";
    case TokenType::TOK_WHILE:
        return "WHILE";
    case TokenType::TOK_FOR:
        return "FOR";
    case TokenType::TOK_RETURN:
        return "RETURN";
    case TokenType::TOK_IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::TOK_NUMBER:
        return "NUMBER";
    case TokenType::TOK_STRING:
        return "STRING";
    case TokenType::TOK_CHAR_LITERAL:
        return "CHAR_LITERAL";
    case TokenType::TOK_PLUS:
        return "PLUS";
    case TokenType::TOK_MINUS:
        return "MINUS";
    case TokenType::TOK_STAR:
        return "STAR";
    case TokenType::TOK_SLASH:
        return "SLASH";
    case TokenType::TOK_PERCENT:
        return "PERCENT";
    case TokenType::TOK_ASSIGN:
        return "ASSIGN";
    case TokenType::TOK_EQ:
        return "EQ";
    case TokenType::TOK_NE:
        return "NE";
    case TokenType::TOK_LT:
        return "LT";
    case TokenType::TOK_GT:
        return "GT";
    case TokenType::TOK_LE:
        return "LE";
    case TokenType::TOK_GE:
        return "GE";
    case TokenType::TOK_AND:
        return "AND";
    case TokenType::TOK_OR:
        return "OR";
    case TokenType::TOK_XOR:
        return "XOR";
    case TokenType::TOK_NOT:
        return "NOT";
    case TokenType::TOK_TILDE:
        return "TILDE";
    case TokenType::TOK_LAND:
        return "LAND";
    case TokenType::TOK_LOR:
        return "LOR";
    case TokenType::TOK_SHL:
        return "SHL";
    case TokenType::TOK_SHR:
        return "SHR";
    case TokenType::TOK_INC:
        return "INC";
    case TokenType::TOK_DEC:
        return "DEC";
    case TokenType::TOK_LPAREN:
        return "LPAREN";
    case TokenType::TOK_RPAREN:
        return "RPAREN";
    case TokenType::TOK_LBRACE:
        return "LBRACE";
    case TokenType::TOK_RBRACE:
        return "RBRACE";
    case TokenType::TOK_LBRACKET:
        return "LBRACKET";
    case TokenType::TOK_RBRACKET:
        return "RBRACKET";
    case TokenType::TOK_SEMICOLON:
        return "SEMICOLON";
    case TokenType::TOK_COMMA:
        return "COMMA";
    case TokenType::TOK_DOT:
        return "DOT";
    case TokenType::TOK_COLON:
        return "COLON";
    case TokenType::TOK_QUESTION:
        return "QUESTION";
    case TokenType::TOK_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void testLexer(const std::string &code)
{
    std::cout << "=== Testing Lexer ===" << std::endl;
    std::cout << "Source Code:" << std::endl;
    std::cout << code << std::endl;
    std::cout << "\n=== Tokens ===" << std::endl;

    Lexer lexer("test.c", code);
    Token token = lexer.nextToken();

    while (token.type != TokenType::TOK_EOF)
    {
        std::cout << "[" << token.location.line << ":" << token.location.column << "] "
                  << getTokenTypeName(token.type) << " \t'" << token.lexeme << "'";

        if (token.type == TokenType::TOK_NUMBER)
        {
            std::cout << " \t(value: " << token.value.intValue << ")";
        }

        std::cout << std::endl;
        token = lexer.nextToken();
    }

    std::cout << "\n=== End of Tokens ===" << std::endl;

    if (lexer.hasErrors())
    {
        std::cout << "\nLexer encountered errors!" << std::endl;
    }
}

int main(int argc, char **argv)
{
    // 测试用例 1: 简单的变量声明和赋值
    std::string test1 = R"(
int x = 42;
char c = 'a';
)";
    testLexer(test1);

    std::cout << "\n\n";

    // 测试用例 2: 函数定义
    std::string test2 = R"(
int add(int a, int b) {
    return a + b;
}
)";
    testLexer(test2);

    std::cout << "\n\n";

    // 测试用例 3: 控制流语句
    std::string test3 = R"(
if (x > 0) {
    y = x * 2;
} else {
    y = -x;
}
)";
    testLexer(test3);

    std::cout << "\n\n";

    // 测试用例 4: 运算符
    std::string test4 = R"(
a = b + c - d * e / f % g;
flag = (x == y) && (a != b) || (c < d);
result = x++ + --y;
value = array[index];
)";
    testLexer(test4);

    std::cout << "\n\n";

    // 测试用例 5: 字符串和字符
    std::string test5 = R"(
char* str = "Hello, World!\n";
char newline = '\n';
)";
    testLexer(test5);

    std::cout << "\n\n";

    // 测试用例 6: 数字字面量（十进制、十六进制、八进制）
    std::string test6 = R"(
int dec = 123;
int hex = 0xFF;
int oct = 0755;
)";
    testLexer(test6);

    std::cout << "\n\n";

    // 如果提供了命令行参数，读取并词法分析文件
    if (argc > 1)
    {
        std::string filename = argv[1];
        std::ifstream file(filename);

        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string code = buffer.str();
        file.close();

        std::cout << "=== Analyzing file: " << filename << " ===" << std::endl;
        testLexer(code);
    }

    return 0;
}
