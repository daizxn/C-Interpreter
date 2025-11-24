cmake_minimum_required(VERSION 3.10)
project(ParseModule)

# 创建 parse 静态库
add_library(parse_lib STATIC
    ast.cpp
    ast.h
    AST.hpp
)

target_include_directories(parse_lib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(parse_lib PRIVATE -Wall -Wextra)

option(BUILD_PARSE_TEST "Build parse tests" OFF)
if(BUILD_PARSE_TEST)
  # 如需测试，可在此添加 test 可执行文件
  # add_executable(test_parse test_parse.cpp)
  # target_link_libraries(test_parse parse_lib)
endif()

install(TARGETS parse_lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ast.h AST.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
