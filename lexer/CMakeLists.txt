cmake_minimum_required(VERSION 3.10)
project(CLexer VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 包含 LLVM 头文件
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# 词法分析器库
add_library(lexer_lib STATIC
    lexer.cpp
    lexer.h
)

# 测试程序
add_executable(test_lexer test_lexer.cpp)
target_link_libraries(test_lexer lexer_lib)

# 编译选项
target_compile_options(lexer_lib PRIVATE -Wall -Wextra)
target_compile_options(test_lexer PRIVATE -Wall -Wextra)

# 安装规则
install(TARGETS test_lexer DESTINATION bin)
install(FILES lexer.h DESTINATION include)
