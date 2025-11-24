#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <memory>

// Token 类型枚举
enum class TokenType
{
    // 文件结束
    TOK_EOF,

    // 关键字
    TOK_INT,
    TOK_CHAR,
    TOK_VOID,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,

    // 标识符和字面量
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_CHAR_LITERAL,

    // 运算符
    TOK_PLUS,    // +
    TOK_MINUS,   // -
    TOK_STAR,    // *
    TOK_SLASH,   // /
    TOK_PERCENT, // %
    TOK_ASSIGN,  // =
    TOK_EQ,      // ==
    TOK_NE,      // !=
    TOK_LT,      // <
    TOK_GT,      // >
    TOK_LE,      // <=
    TOK_GE,      // >=
    TOK_AND,     // &
    TOK_OR,      // |
    TOK_XOR,     // ^
    TOK_NOT,     // !
    TOK_TILDE,   // ~
    TOK_LAND,    // &&
    TOK_LOR,     // ||
    TOK_SHL,     // <<
    TOK_SHR,     // >>
    TOK_INC,     // ++
    TOK_DEC,     // --

    // 分隔符
    TOK_LPAREN,    // (
    TOK_RPAREN,    // )
    TOK_LBRACE,    // {
    TOK_RBRACE,    // }
    TOK_LBRACKET,  // [
    TOK_RBRACKET,  // ]
    TOK_SEMICOLON, // ;
    TOK_COMMA,     // ,
    TOK_DOT,       // .
    TOK_COLON,     // :
    TOK_QUESTION,  // ?

    // 错误
    TOK_ERROR
};

// 源代码位置信息
struct SourceLocation
{
    int line;
    int column;
    std::string filename;

    SourceLocation(const std::string &file = "", int ln = 1, int col = 1)
        : line(ln), column(col), filename(file) {}
};

// Token 结构
struct Token
{
    TokenType type;
    std::string lexeme;
    SourceLocation location;

    // 用于存储数值
    union
    {
        long long intValue;
        double floatValue;
    } value;

    Token(TokenType t, const std::string &lex, const SourceLocation &loc)
        : type(t), lexeme(lex), location(loc)
    {
        value.intValue = 0;
    }

    // 辅助函数
    bool is(TokenType t) const { return type == t; }
    bool isNot(TokenType t) const { return type != t; }
    std::string toString() const;
};

// 词法分析器类
class Lexer
{
public:
    explicit Lexer(const std::string &filename, const std::string &source);

    // 获取下一个 token
    Token nextToken();

    // 查看下一个 token（不消费）
    Token peekToken();

    // 获取当前位置
    SourceLocation getCurrentLocation() const
    {
        return SourceLocation(filename_, line_, column_);
    }

    // 错误报告
    void reportError(const std::string &message);
    bool hasErrors() const { return hasErrors_; }

private:
    std::string filename_;
    std::string source_;
    size_t position_;
    int line_;
    int column_;
    bool hasErrors_;
    std::vector<std::string> errorMessages_;

    // 辅助函数
    char currentChar() const;
    char peekChar(int offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipComment();

    // Token 识别
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readString();
    Token readCharLiteral();
    Token readOperator();

    // 关键字检查
    TokenType getKeywordType(const std::string &identifier);

    // 字符分类
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlnum(char c) const;
    bool isHexDigit(char c) const;
    bool isOctalDigit(char c) const;
};

#endif // LEXER_H
