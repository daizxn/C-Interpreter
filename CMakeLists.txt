cmake_minimum_required(VERSION 3.10)
project(compiler VERSION 1.0 LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建选项
option(BUILD_TESTS "Build test binaries if available" ON)

# 方便设置构建类型（若用户未指定）
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "BUILD_TESTS=${BUILD_TESTS}")

# 在顶层暴露 include 路径
include(GNUInstallDirs)

# 查找 LLVM
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

# 分离 LLVM 编译标志
separate_arguments(LLVM_CXXFLAGS_LIST UNIX_COMMAND "${LLVM_CXXFLAGS}")
separate_arguments(LLVM_LIBRARIES_LIST UNIX_COMMAND "${LLVM_LIBRARIES}")
separate_arguments(LLVM_LDFLAGS_LIST UNIX_COMMAND "${LLVM_LDFLAGS}")

# 启用测试支持（CTest）
if(BUILD_TESTS)
  enable_testing()
  include(CTest)
  message(STATUS "Tests are enabled.")
endif()

# 子模块
add_subdirectory(lexer)
add_subdirectory(ast)
add_subdirectory(parse)
add_subdirectory(semantic)

# ============================
# 主编译器可执行文件
# ============================
add_executable(compiler src/main.cpp)

target_include_directories(compiler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_options(compiler PRIVATE -Wall -Wextra ${LLVM_CXXFLAGS_LIST})

target_link_libraries(compiler PRIVATE
    semantic_lib
    parse_lib
    ast_lib
    lexer_lib
    ${LLVM_LIBRARIES_LIST}
)

# 链接 LLVM
target_link_directories(compiler PRIVATE ${LLVM_LIBRARY_DIRS})
target_link_options(compiler PRIVATE ${LLVM_LDFLAGS_LIST})

# 安装编译器
install(TARGETS compiler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ============================
# 集成测试
# ============================
if(BUILD_TESTS)
  # 词法分析测试
  add_test(NAME compiler_lexer_test 
           COMMAND compiler -l ${CMAKE_SOURCE_DIR}/test/test.txt)
  set_tests_properties(compiler_lexer_test PROPERTIES
    LABELS "compiler;lexer"
    TIMEOUT 10)
  
  # 语法分析测试
  add_test(NAME compiler_parser_test 
           COMMAND compiler -p ${CMAKE_SOURCE_DIR}/test/test.txt)
  set_tests_properties(compiler_parser_test PROPERTIES
    LABELS "compiler;parser"
    TIMEOUT 10)
  
  # IR生成测试
  add_test(NAME compiler_ir_test 
           COMMAND compiler -s ${CMAKE_SOURCE_DIR}/test/test.txt)
  set_tests_properties(compiler_ir_test PROPERTIES
    LABELS "compiler;semantic"
    TIMEOUT 10)
  
  # 简单函数测试
  add_test(NAME compiler_add_fun_test 
           COMMAND compiler ${CMAKE_SOURCE_DIR}/test/add_fun.sysy)
  set_tests_properties(compiler_add_fun_test PROPERTIES
    LABELS "compiler;semantic"
    TIMEOUT 10)
endif()

message(STATUS "Compiler executable will be built as 'compiler'")

