// 测试 const 声明
const int CONST_VAL = 50;

int main()
{
    const int localConst = 100;
    int x = CONST_VAL + localConst;
    return x;
}
