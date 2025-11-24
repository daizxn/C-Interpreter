cmake_minimum_required(VERSION 3.10)
project(interpreter VERSION 0.1 LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认构建选项：允许在顶层控制子模块是否构建
option(BUILD_LEXER "Build lexer module" ON)
option(BUILD_PARSE "Build parse module" ON)
option(BUILD_TESTS "Build test binaries if available" ON)

# 方便设置构建类型（若用户未指定）
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "BUILD_LEXER=${BUILD_LEXER} BUILD_PARSE=${BUILD_PARSE} BUILD_TESTS=${BUILD_TESTS}")

# 在顶层暴露 include 路径（子模块可分别设置自己的 include）
include(GNUInstallDirs)

# 子模块：按需添加
if(BUILD_LEXER)
  add_subdirectory(lexer)
endif()

if(BUILD_PARSE)
  add_subdirectory(parse)
endif()

# 顶层测试（若需要，可以在子模块中声明 test targets 并由此处启用）
if(BUILD_TESTS)
  # 子项目的 test targets 会在各自子 CMakeLists 中添加
  message(STATUS "Tests are enabled. Build available test targets in subprojects.")
endif()

# 安装示例（将来可扩展：导出目标、生成 pkg-config 或 CMake 包）
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        OPTIONAL)

message(STATUS "Top-level CMake configuration complete.")
