# C 解释器项目 - 构建与测试指南

## 项目结构

```
interpreter/
├── CMakeLists.txt          # 顶层 CMake 配置
├── build.sh                # 便捷构建脚本
├── lexer/                  # 词法分析器模块
│   ├── CMakeLists.txt
│   ├── lexer.h
│   ├── lexer.cpp
│   └── test_lexer.cpp
└── parse/                  # 语法分析器模块
    ├── CMakeLists.txt
    ├── ast.h
    ├── ast.cpp
    ├── parser.h
    ├── parser.cpp
    └── test_parser.cpp
```

## 快速开始

### 方法1：使用便捷脚本（推荐）

```bash
# 赋予执行权限
chmod +x build.sh

# 调试构建（包含测试）
./build.sh debug

# 发布构建
./build.sh release

# 构建并运行测试
./build.sh test

# 清理构建目录
./build.sh clean

# 完全重新构建
./build.sh rebuild
```

### 方法2：手动使用 CMake

#### 1. 基本构建（发布模式，不含测试）

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j4
```

#### 2. 调试构建（包含测试）

```bash
mkdir -p build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DBUILD_PARSE_TEST=ON
cmake --build . -j4
```

#### 3. 仅构建特定模块

```bash
# 仅构建词法分析器
cmake .. -DBUILD_LEXER=ON -DBUILD_PARSE=OFF
cmake --build . --target lexer_lib

# 仅构建语法分析器
cmake .. -DBUILD_LEXER=ON -DBUILD_PARSE=ON
cmake --build . --target parse_lib
```

## 运行测试

### 词法分析器测试

```bash
# 从 build 目录
cd build
./lexer/test_lexer

# 或指定测试文件
./lexer/test_lexer ../test_input.c
```

### 语法分析器测试

```bash
# 从 build 目录
cd build
./parse/test_parser

# 或指定测试文件
./parse/test_parser ../test_input.c
```

### 创建测试输入文件

创建 `test_input.c`：

```c
// 简单测试
int x = 10;
int y = 20;

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(x, y);
    if (result > 25) {
        return 1;
    }
    return 0;
}
```

运行：
```bash
./build/parse/test_parser test_input.c
```

## 开发工作流

### 日常开发

```bash
# 1. 首次设置（调试模式）
./build.sh debug

# 2. 修改代码后增量编译
cd build
cmake --build . -j4

# 3. 运行测试验证
./parse/test_parser
```

### 完整测试流程

```bash
# 一键完成：配置、编译、测试
./build.sh test
```

### 清理与重建

```bash
# 清理所有构建产物
./build.sh clean

# 完全重新构建（发布模式）
./build.sh rebuild
```

## CMake 配置选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_LEXER` | ON | 是否构建词法分析器 |
| `BUILD_PARSE` | ON | 是否构建语法分析器 |
| `BUILD_TESTS` | ON | 是否构建测试程序 |
| `BUILD_PARSE_TEST` | OFF | 是否构建语法分析器测试 |
| `CMAKE_BUILD_TYPE` | Release | 构建类型 (Debug/Release) |

### 自定义配置示例

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_TESTS=ON \
    -DBUILD_PARSE_TEST=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local
```

## 编译选项

### Debug 模式特点
- 包含调试符号
- 不进行优化
- 启用断言
- 适合开发和调试

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Release 模式特点
- 完全优化 (-O3)
- 无调试符号
- 禁用断言
- 适合生产环境

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### RelWithDebInfo 模式
- 优化编译
- 包含调试符号
- 适合性能分析

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## 故障排查

### 1. 找不到 LLVM

如果看到 `Could NOT find LLVM`：

```bash
# 安装 LLVM
sudo apt install llvm-14 llvm-14-dev  # Ubuntu/Debian
# 或
brew install llvm  # macOS

# 指定 LLVM 路径
cmake .. -DLLVM_DIR=/usr/lib/llvm-14/cmake
```

### 2. 链接错误

确保按顺序编译：

```bash
cd build
cmake --build . --target lexer_lib
cmake --build . --target parse_lib
cmake --build . --target test_parser
```

### 3. 找不到头文件

检查 include 路径：

```bash
# 查看编译命令
cmake --build . --verbose
```

### 4. 查看详细构建日志

```bash
cmake --build . --verbose
# 或
make VERBOSE=1
```

## IDE 集成

### VS Code

1. 安装 CMake Tools 扩展
2. 打开项目文件夹
3. 按 `Ctrl+Shift+P`，选择 "CMake: Configure"
4. 按 `F7` 构建

配置 `.vscode/settings.json`：

```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureSettings": {
        "BUILD_TESTS": "ON",
        "BUILD_PARSE_TEST": "ON"
    }
}
```

### CLion

1. 打开项目
2. CLion 自动识别 CMakeLists.txt
3. 选择构建配置（Debug/Release）
4. 点击 Build 或按 `Ctrl+F9`

## 性能分析

### 使用 gprof

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-pg"
cmake --build .
./parse/test_parser test_input.c
gprof ./parse/test_parser gmon.out > analysis.txt
```

### 使用 valgrind

```bash
valgrind --leak-check=full ./parse/test_parser test_input.c
```

## 安装

```bash
cd build
sudo cmake --install .
```

默认安装到 `/usr/local`，可通过 `-DCMAKE_INSTALL_PREFIX` 修改。

## 常用命令速查

```bash
# 快速调试构建
./build.sh debug

# 快速测试
./build.sh test

# 增量编译
cd build && cmake --build . -j4

# 运行单个测试
./build/parse/test_parser test.c

# 清理重建
./build.sh clean && ./build.sh release

# 查看构建选项
cmake -L build/

# 生成 compile_commands.json (for clangd)
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## 贡献指南

提交代码前请确保：

1. 代码通过编译：`./build.sh release`
2. 所有测试通过：`./build.sh test`
3. 无内存泄漏：运行 valgrind 检查
4. 代码格式化：使用 clang-format

---

## 更多帮助

- 查看 CMakeLists.txt 中的详细配置
- 查看各模块的 README（如果有）
- 提 Issue 或联系维护者
