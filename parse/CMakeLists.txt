cmake_minimum_required(VERSION 3.10)
project(ParseModule)

# 创建 parse 静态库
add_library(parse_lib STATIC
    ast.cpp
    ast.h
    parser.cpp
    parser.h
)

target_include_directories(parse_lib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lexer
)

target_compile_options(parse_lib PRIVATE -Wall -Wextra)

# 链接词法分析器库
target_link_libraries(parse_lib PUBLIC lexer_lib)

option(BUILD_PARSE_TEST "Build parse tests" ON)
if(BUILD_PARSE_TEST)
  add_executable(test_parser test_parser.cpp)
  target_link_libraries(test_parser parse_lib)
  target_compile_options(test_parser PRIVATE -Wall -Wextra)
  
  # 可选：添加到 CTest
  # enable_testing()
  # add_test(NAME parser_test COMMAND test_parser)
endif()

install(TARGETS parse_lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ast.h parser.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
