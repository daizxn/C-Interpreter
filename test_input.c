// 简单的 C 语言测试程序
// 用于测试词法分析器和语法分析器

// 全局变量声明
int globalVar = 100;
int array[10];

// 函数声明
int add(int x, int y);
int factorial(int n);

// 加法函数
int add(int x, int y)
{
    return x + y;
}

// 阶乘函数（递归）
int factorial(int n)
{
    if (n <= 1)
    {
        return 1;
    }
    return n * factorial(n - 1);
}

// 主函数
int main()
{
    // 局部变量
    int a = 10;
    int b = 20;
    int result;

    // 算术运算
    result = add(a, b);

    // 条件语句
    if (result > 25)
    {
        result = result + 5;
    }
    else
    {
        result = result - 5;
    }

    // 循环语句 - while
    int i = 0;
    while (i < 5)
    {
        array[i] = i * 2;
        i = i + 1;
    }

    // 循环语句 - for
    for (int j = 0; j < 5; j = j + 1)
    {
        array[j + 5] = j * 3;
    }

    // 函数调用
    int fact = factorial(5);

    // 三元运算符
    int max = (a > b) ? a : b;

    // 位运算
    int bitwiseResult = (a & b) | (a ^ b);

    // 返回
    return 0;
}

// 带数组参数的函数
int sumArray(int arr[], int size)
{
    int sum = 0;
    for (int i = 0; i < size; i = i + 1)
    {
        sum = sum + arr[i];
    }
    return sum;
}

// 多参数函数
int calculate(int a, int b, int c)
{
    int temp = a + b;
    temp = temp * c;
    return temp;
}
