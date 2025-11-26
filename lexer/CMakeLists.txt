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
# 使用目标级别包含和编译定义，避免污染全局目录
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# 词法分析器库
add_library(lexer_lib STATIC
    lexer.cpp
)

# 将顶层 include 暴露为公共接口，便于其它模块引用 AST/parser 头
target_include_directories(lexer_lib
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    ${LLVM_INCLUDE_DIRS}
)

# 测试程序
add_executable(test_lexer test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE lexer_lib)

# 编译选项
target_compile_options(lexer_lib PRIVATE -Wall -Wextra)
target_compile_options(test_lexer PRIVATE -Wall -Wextra)

# 添加测试（通过 CTest）
if(BUILD_TESTING)
  add_test(NAME lexer_basic_test 
           COMMAND test_lexer ${CMAKE_SOURCE_DIR}/test/test.txt)
  set_tests_properties(lexer_basic_test PROPERTIES
    LABELS "lexer"
    TIMEOUT 10)
endif()


