#include "parser.h"
#include "ast.h"
#include "../lexer/lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <source_file>" << std::endl;
        std::cout << "\nExample: " << argv[0] << " test.c" << std::endl;
        std::cout << "\nOr run with inline test:" << std::endl;
        std::cout << "  " << argv[0] << " --test" << std::endl;
        return 1;
    }

    std::string source;
    std::string filename;

    if (std::string(argv[1]) == "--test")
    {
        // 内置测试代码
        filename = "test.c";
        source = R"(
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int x;
    int result;
    
    x = 5;
    result = factorial(x);
    
    return 0;
}
)";
    }
    else
    {
        // 从文件读取
        filename = argv[1];
        std::ifstream file(filename);
        if (!file)
        {
            std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();
    }

    std::cout << "=== Parsing " << filename << " ===" << std::endl;
    std::cout << "\nSource code:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << source << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 创建词法分析器
    Lexer lexer(filename, source);

    // 创建语法分析器
    Parser parser(lexer);

    // 解析
    std::cout << "\n=== Parsing ===" << std::endl;
    auto ast = parser.parse();

    // 检查错误
    if (parser.hasErrors())
    {
        std::cerr << "\n=== Parse Errors ===" << std::endl;
        for (const auto &error : parser.getErrors())
        {
            std::cerr << error << std::endl;
        }
        return 1;
    }

    // 输出 AST
    std::cout << "\n=== Abstract Syntax Tree ===" << std::endl;
    if (ast)
    {
        ast->dump(0);
    }
    else
    {
        std::cout << "(empty)" << std::endl;
    }

    std::cout << "\n=== Parse completed successfully ===" << std::endl;
    return 0;
}
