# 词法分析器模块

# 词法分析器库
add_library(lexer_lib STATIC
    lexer.cpp
)

# 将顶层 include 暴露为公共接口
target_include_directories(lexer_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 编译选项
target_compile_options(lexer_lib PRIVATE -Wall -Wextra)

message(STATUS "Lexer module configured")

