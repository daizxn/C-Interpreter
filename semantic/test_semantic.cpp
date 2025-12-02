#include "semantic.h.old"
#include "parser.h"
#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <source_file|--test>" << std::endl;
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
int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int x = 5;
    int y = 10;
    int sum = add(x, y);
    int fact = factorial(5);
    
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

    // 词法分析
    Lexer lexer(filename, source);

    // 语法分析
    Parser parser(lexer);
    auto ast = parser.parse();

    // 检查解析错误
    if (parser.hasErrors())
    {
        std::cerr << "\n=== Parse Errors ===" << std::endl;
        for (const auto &error : parser.getErrors())
        {
            std::cerr << error << std::endl;
        }
        return 1;
    }

    std::cout << "\n=== Abstract Syntax Tree ===" << std::endl;
    if (ast)
    {
        ast->dump(0);
    }

    // 语义分析和 IR 生成
    std::cout << "\n=== Generating LLVM IR ===" << std::endl;
    CodeGenerator codegen(filename);

    if (!codegen.generate(ast.get()))
    {
        std::cerr << "\n=== Semantic Errors ===" << std::endl;
        for (const auto &error : codegen.getErrors())
        {
            std::cerr << error << std::endl;
        }
        return 1;
    }

    // 输出 LLVM IR
    std::cout << "\n=== LLVM IR ===" << std::endl;
    std::cout << codegen.getIRString() << std::endl;

    // 可选：写入文件
    std::string irFilename = filename + ".ll";
    if (codegen.writeIRToFile(irFilename))
    {
        std::cout << "\n=== IR written to " << irFilename << " ===" << std::endl;
    }

    std::cout << "\n=== Code generation completed successfully ===" << std::endl;
    return 0;
}
