cmake_minimum_required(VERSION 3.10)
project(ParseModule)

# 创建 parse 静态库
add_library(parse_lib STATIC
  parser.cpp
)

# 暴露顶层 include，使其他模块可直接包含 <ast.h> 等头文件
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

option(BUILD_PARSE_TEST "Build parse tests" ON)
if(BUILD_PARSE_TEST)
  add_executable(test_parser test_parser.cpp)
  target_link_libraries(test_parser PRIVATE parse_lib)
  target_compile_options(test_parser PRIVATE -Wall -Wextra)
  
  # 添加到 CTest
  if(BUILD_TESTING)
    # 使用内置测试
    add_test(NAME parser_builtin_test 
             COMMAND test_parser --test)
    set_tests_properties(parser_builtin_test PROPERTIES
      LABELS "parser"
      TIMEOUT 10)
    
    # 使用外部文件测试
    add_test(NAME parser_file_test 
             COMMAND test_parser ${CMAKE_SOURCE_DIR}/test/test.txt)
    set_tests_properties(parser_file_test PROPERTIES
      LABELS "parser"
      TIMEOUT 10)
  endif()
endif()
