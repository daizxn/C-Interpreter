cmake_minimum_required(VERSION 3.10)
project(ASTModule)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 创建 AST 静态库
add_library(ast_lib STATIC
    ast.cpp
)

# 暴露顶层 include 目录（ast.h 位于 ${CMAKE_SOURCE_DIR}/include）
target_include_directories(ast_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(ast_lib PRIVATE -Wall -Wextra)

message(STATUS "AST module configured as independent library")
