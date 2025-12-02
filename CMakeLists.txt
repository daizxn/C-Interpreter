cmake_minimum_required(VERSION 3.10)
project(interpreter VERSION 0.1 LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认构建选项：允许在顶层控制子模块是否构建
option(BUILD_LEXER "Build lexer module" ON)
option(BUILD_PARSE "Build parse module" ON)
option(BUILD_AST "Build ast module" ON)
option(BUILD_SEMANTIC "Build semantic analysis module" ON)
option(BUILD_TESTS "Build test binaries if available" ON)

# 方便设置构建类型（若用户未指定）
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "BUILD_LEXER=${BUILD_LEXER} BUILD_AST=${BUILD_AST} BUILD_PARSE=${BUILD_PARSE} BUILD_SEMANTIC=${BUILD_SEMANTIC} BUILD_TESTS=${BUILD_TESTS}")

# 在顶层暴露 include 路径（子模块可分别设置自己的 include）
include(GNUInstallDirs)

# 启用测试支持（CTest）- 必须在 add_subdirectory 之前
if(BUILD_TESTS)
  enable_testing()
  include(CTest)
  message(STATUS "Tests are enabled. Build available test targets in subprojects.")
endif()

# 子模块：按需添加
if(BUILD_LEXER)
  add_subdirectory(lexer)
endif()

if(BUILD_AST)
  add_subdirectory(ast)
endif()

if(BUILD_PARSE)
  add_subdirectory(parse)
endif()

if(BUILD_SEMANTIC)
  add_subdirectory(semantic)
endif()

