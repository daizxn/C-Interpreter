# AST模块

# 创建 AST 静态库
add_library(ast_lib STATIC
    ast.cpp
)

# 暴露顶层 include 目录
target_include_directories(ast_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(ast_lib PRIVATE -Wall -Wextra)

message(STATUS "AST module configured")
