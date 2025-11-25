#include "lexer.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include <map>

// Token 到字符串的转换
std::string Token::toString() const
{
    std::stringstream ss;
    ss << "Token(" << static_cast<int>(type) << ", \"" << lexeme << "\", "
       << location.line << ":" << location.column << ")";
    return ss.str();
}

// 关键字映射表
static const std::map<std::string, TokenType> keywords = {
    {"int", TokenType::TOK_INT},
    {"char", TokenType::TOK_CHAR},
    {"void", TokenType::TOK_VOID},
    {"const", TokenType::TOK_CONST},
    {"if", TokenType::TOK_IF},
    {"else", TokenType::TOK_ELSE},
    {"while", TokenType::TOK_WHILE},
    {"for", TokenType::TOK_FOR},
    {"return", TokenType::TOK_RETURN}};

// 构造函数
Lexer::Lexer(const std::string &filename, const std::string &source)
    : filename_(filename),
      source_(source), position_(0), line_(1), column_(1), hasErrors_(false)
{
}

// 获取当前字符
char Lexer::currentChar() const
{
    if (position_ >= source_.length())
    {
        return '\0';
    }
    return source_[position_];
}

// 查看前面的字符
char Lexer::peekChar(int offset) const
{
    size_t pos = position_ + offset;
    if (pos >= source_.length())
    {
        return '\0';
    }
    return source_[pos];
}

// 前进一个字符
void Lexer::advance()
{
    if (position_ < source_.length())
    {
        if (source_[position_] == '\n')
        {
            line_++;
            column_ = 1;
        }
        else
        {
            column_++;
        }
        position_++;
    }
}

// 跳过空白字符
void Lexer::skipWhitespace()
{
    while (std::isspace(currentChar()))
    {
        advance();
    }
}

// 跳过注释
void Lexer::skipComment()
{
    if (currentChar() == '/' && peekChar() == '/')
    {
        // 单行注释
        while (currentChar() != '\n' && currentChar() != '\0')
        {
            advance();
        }
    }
    else if (currentChar() == '/' && peekChar() == '*')
    {
        // 多行注释
        advance(); // skip '/'
        advance(); // skip '*'
        while (currentChar() != '\0')
        {
            if (currentChar() == '*' && peekChar() == '/')
            {
                advance(); // skip '*'
                advance(); // skip '/'
                break;
            }
            advance();
        }
    }
}

// 字符分类函数
bool Lexer::isAlpha(char c) const
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isDigit(char c) const
{
    return c >= '0' && c <= '9';
}

bool Lexer::isAlnum(char c) const
{
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isHexDigit(char c) const
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Lexer::isOctalDigit(char c) const
{
    return c >= '0' && c <= '7';
}

// 检查是否为关键字
TokenType Lexer::getKeywordType(const std::string &identifier)
{
    auto it = keywords.find(identifier);
    if (it != keywords.end())
    {
        return it->second;
    }
    return TokenType::TOK_IDENTIFIER;
}

// 读取标识符或关键字
Token Lexer::readIdentifierOrKeyword()
{
    SourceLocation loc = getCurrentLocation();
    std::string identifier;

    while (isAlnum(currentChar()))
    {
        identifier += currentChar();
        advance();
    }

    TokenType type = getKeywordType(identifier);
    return Token(type, identifier, loc);
}

// 读取数字
Token Lexer::readNumber()
{
    SourceLocation loc = getCurrentLocation();
    std::string number;
    long long value = 0;
    int base = 10;

    // 检查十六进制
    if (currentChar() == '0' && (peekChar() == 'x' || peekChar() == 'X'))
    {
        base = 16;
        number += currentChar();
        advance(); // '0'
        number += currentChar();
        advance(); // 'x' or 'X'

        while (isHexDigit(currentChar()))
        {
            char c = currentChar();
            number += c;
            int digit = (c >= '0' && c <= '9') ? (c - '0') : (c >= 'a' && c <= 'f') ? (c - 'a' + 10)
                                                                                    : (c - 'A' + 10);
            value = value * 16 + digit;
            advance();
        }
    }
    // 检查八进制
    else if (currentChar() == '0' && isOctalDigit(peekChar()))
    {
        base = 8;
        number += currentChar();
        advance(); // '0'

        while (isOctalDigit(currentChar()))
        {
            char c = currentChar();
            number += c;
            value = value * 8 + (c - '0');
            advance();
        }
    }
    // 十进制
    else
    {
        while (isDigit(currentChar()))
        {
            char c = currentChar();
            number += c;
            value = value * 10 + (c - '0');
            advance();
        }
    }

    Token token(TokenType::TOK_NUMBER, number, loc);
    token.value.intValue = value;
    return token;
}

// 读取字符串字面量
Token Lexer::readString()
{
    SourceLocation loc = getCurrentLocation();
    std::string strLit;
    advance(); // skip opening "

    while (currentChar() != '"' && currentChar() != '\0')
    {
        if (currentChar() == '\\')
        {
            advance();
            char esc = currentChar();
            switch (esc)
            {
            case 'n':
                strLit += '\n';
                break;
            case 't':
                strLit += '\t';
                break;
            case 'r':
                strLit += '\r';
                break;
            case '\\':
                strLit += '\\';
                break;
            case '"':
                strLit += '"';
                break;
            default:
                strLit += esc; // Unknown escape, treat literally
                break;
            }
        }
        else
        {
            strLit += currentChar();
        }
        advance();
    }

    advance(); // skip closing "
    return Token(TokenType::TOK_STRING, strLit, loc);
}

// 读取字符字面量
Token Lexer::readCharLiteral()
{
    SourceLocation loc = getCurrentLocation();
    std::string charLit;

    advance(); // skip opening quote

    if (currentChar() == '\\')
    {
        advance();
        switch (currentChar())
        {
        case 'n':
            charLit += '\n';
            break;
        case 't':
            charLit += '\t';
            break;
        case 'r':
            charLit += '\r';
            break;
        case '\\':
            charLit += '\\';
            break;
        case '\'':
            charLit += '\'';
            break;
        case '0':
            charLit += '\0';
            break;
        default:
            charLit += currentChar();
            break;
        }
        advance();
    }
    else if (currentChar() != '\'')
    {
        charLit += currentChar();
        advance();
    }

    if (currentChar() == '\'')
    {
        advance(); // skip closing quote
    }
    else
    {
        reportError("Unterminated character literal");
    }

    Token token(TokenType::TOK_CHAR_LITERAL, charLit, loc);
    if (!charLit.empty())
    {
        token.value.intValue = static_cast<unsigned char>(charLit[0]);
    }
    return token;
}

// 读取运算符
Token Lexer::readOperator()
{
    SourceLocation loc = getCurrentLocation();
    char c = currentChar();

    // 双字符运算符
    if (c == '+' && peekChar() == '+')
    {
        advance();
        advance();
        return Token(TokenType::TOK_INC, "++", loc);
    }
    if (c == '-' && peekChar() == '-')
    {
        advance();
        advance();
        return Token(TokenType::TOK_DEC, "--", loc);
    }
    if (c == '=' && peekChar() == '=')
    {
        advance();
        advance();
        return Token(TokenType::TOK_EQ, "==", loc);
    }
    if (c == '!' && peekChar() == '=')
    {
        advance();
        advance();
        return Token(TokenType::TOK_NE, "!=", loc);
    }
    if (c == '<' && peekChar() == '=')
    {
        advance();
        advance();
        return Token(TokenType::TOK_LE, "<=", loc);
    }
    if (c == '>' && peekChar() == '=')
    {
        advance();
        advance();
        return Token(TokenType::TOK_GE, ">=", loc);
    }
    if (c == '<' && peekChar() == '<')
    {
        advance();
        advance();
        return Token(TokenType::TOK_SHL, "<<", loc);
    }
    if (c == '>' && peekChar() == '>')
    {
        advance();
        advance();
        return Token(TokenType::TOK_SHR, ">>", loc);
    }
    if (c == '&' && peekChar() == '&')
    {
        advance();
        advance();
        return Token(TokenType::TOK_LAND, "&&", loc);
    }
    if (c == '|' && peekChar() == '|')
    {
        advance();
        advance();
        return Token(TokenType::TOK_LOR, "||", loc);
    }

    // 单字符运算符和分隔符
    advance();
    switch (c)
    {
    case '+':
        return Token(TokenType::TOK_PLUS, "+", loc);
    case '-':
        return Token(TokenType::TOK_MINUS, "-", loc);
    case '*':
        return Token(TokenType::TOK_STAR, "*", loc);
    case '/':
        return Token(TokenType::TOK_SLASH, "/", loc);
    case '%':
        return Token(TokenType::TOK_PERCENT, "%", loc);
    case '=':
        return Token(TokenType::TOK_ASSIGN, "=", loc);
    case '<':
        return Token(TokenType::TOK_LT, "<", loc);
    case '>':
        return Token(TokenType::TOK_GT, ">", loc);
    case '&':
        return Token(TokenType::TOK_AND, "&", loc);
    case '|':
        return Token(TokenType::TOK_OR, "|", loc);
    case '^':
        return Token(TokenType::TOK_XOR, "^", loc);
    case '!':
        return Token(TokenType::TOK_NOT, "!", loc);
    case '~':
        return Token(TokenType::TOK_TILDE, "~", loc);
    case '(':
        return Token(TokenType::TOK_LPAREN, "(", loc);
    case ')':
        return Token(TokenType::TOK_RPAREN, ")", loc);
    case '{':
        return Token(TokenType::TOK_LBRACE, "{", loc);
    case '}':
        return Token(TokenType::TOK_RBRACE, "}", loc);
    case '[':
        return Token(TokenType::TOK_LBRACKET, "[", loc);
    case ']':
        return Token(TokenType::TOK_RBRACKET, "]", loc);
    case ';':
        return Token(TokenType::TOK_SEMICOLON, ";", loc);
    case ',':
        return Token(TokenType::TOK_COMMA, ",", loc);
    case '.':
        return Token(TokenType::TOK_DOT, ".", loc);
    case ':':
        return Token(TokenType::TOK_COLON, ":", loc);
    case '?':
        return Token(TokenType::TOK_QUESTION, "?", loc);
    default:
        reportError("Unknown character: " + std::string(1, c));
        return Token(TokenType::TOK_ERROR, std::string(1, c), loc);
    }
}

// 获取下一个 token
Token Lexer::nextToken()
{
    // 跳过空白和注释
    while (true)
    {
        skipWhitespace();
        if (currentChar() == '/' && (peekChar() == '/' || peekChar() == '*'))
        {
            skipComment();
        }
        else
        {
            break;
        }
    }

    SourceLocation loc = getCurrentLocation();
    char c = currentChar();

    // 文件结束
    if (c == '\0')
    {
        return Token(TokenType::TOK_EOF, "", loc);
    }

    // 标识符或关键字
    if (isAlpha(c))
    {
        return readIdentifierOrKeyword();
    }

    // 数字
    if (isDigit(c))
    {
        return readNumber();
    }

    // 字符串字面量
    if (c == '"')
    {
        return readString();
    }

    // 字符字面量
    if (c == '\'')
    {
        return readCharLiteral();
    }

    // 运算符和分隔符
    return readOperator();
}

// 错误报告
void Lexer::reportError(const std::string &message)
{
    hasErrors_ = true;
    std::string errorMsg = filename_ + ":" + std::to_string(line_) + ":" +
                           std::to_string(column_) + ": error: " + message;
    errorMessages_.push_back(errorMsg);
    std::cerr << errorMsg << std::endl;
}
