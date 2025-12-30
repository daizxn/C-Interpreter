/**
 * @file main.cpp
 * @brief C语言编译器主程序入口
 *
 * 支持以下命令行选项：
 *   -l          输出词法分析结果
 *   -p          输出语法分析结果（AST）
 *   -s          输出LLVM IR（默认模式）
 *   -o <file>   指定输出文件
 *   -h, --help  显示帮助信息
 *
 * 用法: compiler [options] <source_file>
 */

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// 编译模式
enum class CompileMode
{
    LEXER,   // 词法分析
    PARSER,  // 语法分析
    SEMANTIC // 语义分析/IR生成（默认）
};

// 程序配置
struct CompilerConfig
{
    CompileMode mode = CompileMode::SEMANTIC;
    std::string inputFile;
    std::string outputFile;
    bool help = false;
};

// 打印帮助信息
void printHelp(const char *programName)
{
    std::cout << "用法: " << programName << " [选项] <源文件>\n\n"
              << "选项:\n"
              << "  -l          输出词法分析结果（Token列表）\n"
              << "  -p          输出语法分析结果（AST）\n"
              << "  -s          输出LLVM IR（默认模式）\n"
              << "  -o <file>   指定输出文件（默认输出到终端）\n"
              << "  -h, --help  显示此帮助信息\n\n"
              << "示例:\n"
              << "  " << programName << " test.c              # 编译并输出IR到终端\n"
              << "  " << programName << " -s test.c -o out.ll # 编译并保存IR到文件\n"
              << "  " << programName << " -l test.c -o lex.txt # 输出词法分析结果到文件\n"
              << "  " << programName << " -p test.c           # 输出语法分析结果到终端\n";
}

// 解析命令行参数
CompilerConfig parseArgs(int argc, char *argv[])
{
    CompilerConfig config;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            config.help = true;
            return config;
        }
        else if (arg == "-l")
        {
            config.mode = CompileMode::LEXER;
        }
        else if (arg == "-p")
        {
            config.mode = CompileMode::PARSER;
        }
        else if (arg == "-s")
        {
            config.mode = CompileMode::SEMANTIC;
        }
        else if (arg == "-o")
        {
            if (i + 1 < argc)
            {
                config.outputFile = argv[++i];
            }
            else
            {
                std::cerr << "错误: -o 选项需要指定输出文件名\n";
                exit(1);
            }
        }
        else if (arg[0] == '-')
        {
            std::cerr << "错误: 未知选项 '" << arg << "'\n";
            std::cerr << "使用 '" << argv[0] << " --help' 查看帮助\n";
            exit(1);
        }
        else
        {
            if (config.inputFile.empty())
            {
                config.inputFile = arg;
            }
            else
            {
                std::cerr << "错误: 只能指定一个源文件\n";
                exit(1);
            }
        }
    }

    return config;
}

// 读取源文件
std::string readSourceFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "错误: 无法打开文件 '" << filename << "'\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 输出结果到文件或终端
void output(const std::string &content, const std::string &outputFile)
{
    if (outputFile.empty())
    {
        std::cout << content;
    }
    else
    {
        std::ofstream file(outputFile);
        if (!file)
        {
            std::cerr << "错误: 无法创建输出文件 '" << outputFile << "'\n";
            exit(1);
        }
        file << content;
        std::cout << "已输出到文件: " << outputFile << "\n";
    }
}

// 词法分析模式
int runLexer(const std::string &filename, const std::string &source, const std::string &outputFile)
{
    Lexer lexer(filename, source);
    std::ostringstream oss;

    oss << "=== 词法分析结果 ===\n";
    oss << "源文件: " << filename << "\n";
    oss << "-------------------------------------------\n";
    oss << std::left << std::setw(20) << "Token类型"
        << std::setw(20) << "词素"
        << std::setw(10) << "行号"
        << std::setw(10) << "列号" << "\n";
    oss << "-------------------------------------------\n";

    Token token = lexer.nextToken();
    while (token.type != TokenType::TOK_EOF)
    {
        oss << std::left << std::setw(20) << token.toString()
            << std::setw(20) << token.lexeme
            << std::setw(10) << token.location.line
            << std::setw(10) << token.location.column << "\n";
        token = lexer.nextToken();
    }

    oss << "-------------------------------------------\n";
    oss << "词法分析完成";
    if (lexer.hasErrors())
    {
        oss << "（有错误）";
    }
    oss << "\n";

    output(oss.str(), outputFile);
    return lexer.hasErrors() ? 1 : 0;
}

// 语法分析模式
int runParser(const std::string &filename, const std::string &source, const std::string &outputFile)
{
    Lexer lexer(filename, source);
    Parser parser(lexer);

    auto ast = parser.parse();

    std::ostringstream oss;
    oss << "=== 语法分析结果 ===\n";
    oss << "源文件: " << filename << "\n";
    oss << "-------------------------------------------\n";

    if (parser.hasErrors())
    {
        oss << "语法分析错误:\n";
        for (const auto &error : parser.getErrors())
        {
            oss << "  " << error << "\n";
        }
    }
    else if (ast)
    {
        oss << "AST结构:\n";
        // 重定向 stdout 到字符串流来捕获 dump 输出
        std::streambuf *oldCoutBuf = std::cout.rdbuf();
        std::ostringstream dumpStream;
        std::cout.rdbuf(dumpStream.rdbuf());
        ast->dump(0);
        std::cout.rdbuf(oldCoutBuf);
        oss << dumpStream.str();
    }

    oss << "-------------------------------------------\n";
    oss << "语法分析完成";
    if (parser.hasErrors())
    {
        oss << "（有错误）";
    }
    oss << "\n";

    output(oss.str(), outputFile);
    return parser.hasErrors() ? 1 : 0;
}

// 语义分析/IR生成模式
int runSemantic(const std::string &filename, const std::string &source, const std::string &outputFile)
{
    // 词法分析
    Lexer lexer(filename, source);
    if (lexer.hasErrors())
    {
        std::cerr << "词法分析错误，终止编译\n";
        return 1;
    }

    // 语法分析
    Parser parser(lexer);
    auto ast = parser.parse();

    if (parser.hasErrors())
    {
        std::cerr << "语法分析错误:\n";
        for (const auto &error : parser.getErrors())
        {
            std::cerr << "  " << error << "\n";
        }
        return 1;
    }

    // 语义分析和IR生成
    fs::path filePath(filename);
    std::string moduleName = filePath.stem().string();
    CodeGenerator codegen(moduleName);

    if (!codegen.generate(ast.get()))
    {
        std::cerr << "语义分析/代码生成错误:\n";
        for (const auto &error : codegen.getErrors())
        {
            std::cerr << "  " << error << "\n";
        }
        return 1;
    }

    // 输出IR
    std::string ir = codegen.getIRString();

    if (outputFile.empty())
    {
        std::cout << ir;
    }
    else
    {
        if (codegen.writeIRToFile(outputFile))
        {
            std::cout << "IR已输出到文件: " << outputFile << "\n";
        }
        else
        {
            std::cerr << "错误: 无法写入输出文件 '" << outputFile << "'\n";
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    // 解析命令行参数
    CompilerConfig config = parseArgs(argc, argv);

    // 显示帮助
    if (config.help)
    {
        printHelp(argv[0]);
        return 0;
    }

    // 检查输入文件
    if (config.inputFile.empty())
    {
        std::cerr << "错误: 未指定源文件\n";
        std::cerr << "使用 '" << argv[0] << " --help' 查看帮助\n";
        return 1;
    }

    // 读取源文件
    std::string source = readSourceFile(config.inputFile);

    // 根据模式执行相应操作
    switch (config.mode)
    {
    case CompileMode::LEXER:
        return runLexer(config.inputFile, source, config.outputFile);

    case CompileMode::PARSER:
        return runParser(config.inputFile, source, config.outputFile);

    case CompileMode::SEMANTIC:
        return runSemantic(config.inputFile, source, config.outputFile);
    }

    return 0;
}
