#!/bin/bash
# 便捷构建脚本
# 用法: ./build.sh [clean|debug|release|test]

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 清理构建目录
clean_build() {
    print_info "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    print_info "Clean complete."
}

# 配置项目
configure_project() {
    local build_type=$1
    local enable_tests=$2
    
    print_info "Configuring project (Build Type: ${build_type}, Tests: ${enable_tests})..."
    
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE="${build_type}" \
        -DBUILD_TESTS="${enable_tests}" \
        -DBUILD_PARSE_TEST="${enable_tests}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    print_info "Configuration complete."
}

# 编译项目
build_project() {
    print_info "Building project..."
    cd "${BUILD_DIR}"
    cmake --build . -j$(nproc) || cmake --build . -j4
    print_info "Build complete."
}

# 运行测试
run_tests() {
    print_info "Running tests..."
    cd "${BUILD_DIR}"
    
    # 词法分析器测试
    if [ -f "lexer/test_lexer" ]; then
        print_info "Running lexer tests..."
        ./lexer/test_lexer
    fi
    
    # 语法分析器测试
    if [ -f "parse/test_parser" ]; then
        print_info "Running parser tests..."
        ./parse/test_parser
    fi
    
    print_info "All tests completed."
}

# 主函数
main() {
    local command=${1:-release}
    
    case "$command" in
        clean)
            clean_build
            ;;
        debug)
            clean_build
            configure_project "Debug" "ON"
            build_project
            ;;
        release)
            configure_project "Release" "OFF"
            build_project
            ;;
        test)
            configure_project "Debug" "ON"
            build_project
            run_tests
            ;;
        rebuild)
            clean_build
            configure_project "Release" "OFF"
            build_project
            ;;
        *)
            echo "Usage: $0 [clean|debug|release|test|rebuild]"
            echo ""
            echo "Commands:"
            echo "  clean    - Clean build directory"
            echo "  debug    - Clean, configure and build in Debug mode with tests"
            echo "  release  - Configure and build in Release mode (incremental)"
            echo "  test     - Configure, build and run all tests"
            echo "  rebuild  - Clean and rebuild in Release mode"
            exit 1
            ;;
    esac
}

main "$@"
