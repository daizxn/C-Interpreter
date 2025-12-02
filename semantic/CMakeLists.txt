cmake_minimum_required(VERSION 3.10)
project(SemanticModule)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 LLVM（使用 llvm-config）
find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config llvm-config-18)

if(NOT LLVM_CONFIG_EXECUTABLE)
    message(FATAL_ERROR "llvm-config not found. Please install LLVM development package.")
endif()

# 获取 LLVM 配置
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --version OUTPUT_VARIABLE LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir OUTPUT_VARIABLE LLVM_LIBRARY_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs core support OUTPUT_VARIABLE LLVM_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags OUTPUT_VARIABLE LLVM_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "Found LLVM ${LLVM_VERSION}")
message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM library dir: ${LLVM_LIBRARY_DIRS}")

# 分离 LLVM 编译标志
separate_arguments(LLVM_CXXFLAGS_LIST UNIX_COMMAND "${LLVM_CXXFLAGS}")
separate_arguments(LLVM_LIBRARIES_LIST UNIX_COMMAND "${LLVM_LIBRARIES}")
separate_arguments(LLVM_LDFLAGS_LIST UNIX_COMMAND "${LLVM_LDFLAGS}")

add_library(semantic_lib STATIC
    semantic.cpp
)

# 暴露顶层 include 目录和 LLVM 头文件
target_include_directories(semantic_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 编译选项（包含 LLVM 编译标志）
target_compile_options(semantic_lib PRIVATE -Wall -Wextra ${LLVM_CXXFLAGS_LIST})

# 添加 LLVM 库目录
link_directories(${LLVM_LIBRARY_DIRS})

# 链接 LLVM 库和依赖
target_link_libraries(semantic_lib 
    PUBLIC 
        ast_lib
    PRIVATE
        ${LLVM_LIBRARIES_LIST}
)

# 测试程序
option(BUILD_SEMANTIC_TEST "Build semantic tests" ON)
if(BUILD_SEMANTIC_TEST)
    add_executable(test_semantic test_semantic.cpp)
    target_link_libraries(test_semantic PRIVATE 
        semantic_lib
        parse_lib
        lexer_lib
    )
    target_compile_options(test_semantic PRIVATE -Wall -Wextra)
    
    # 添加到 CTest
    if(BUILD_TESTING)
        # 使用内置测试
        add_test(NAME semantic_builtin_test 
                 COMMAND test_semantic --test)
        set_tests_properties(semantic_builtin_test PROPERTIES
            LABELS "semantic"
            TIMEOUT 10)
        
        # 使用外部文件测试
        if(EXISTS ${CMAKE_SOURCE_DIR}/test/test.txt)
            add_test(NAME semantic_file_test 
                     COMMAND test_semantic ${CMAKE_SOURCE_DIR}/test/test.txt)
            set_tests_properties(semantic_file_test PROPERTIES
                LABELS "semantic"
                TIMEOUT 10)
        endif()
    endif()
endif()

message(STATUS "Semantic module configured with LLVM IR generation")