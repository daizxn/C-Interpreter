# 语义分析模块

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

message(STATUS "Semantic module configured with LLVM IR generation")
