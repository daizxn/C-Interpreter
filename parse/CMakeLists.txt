# 语法分析器模块

# 创建 parse 静态库
add_library(parse_lib STATIC
    parser.cpp
)

# 暴露顶层 include
target_include_directories(parse_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 编译选项
target_compile_options(parse_lib PRIVATE -Wall -Wextra)

# 链接依赖库：ast_lib 和 lexer_lib
target_link_libraries(parse_lib 
    PUBLIC 
        ast_lib
        lexer_lib
)

message(STATUS "Parser module configured")
