//
// Created by 吨吨 on 2025/6/4.
//

#include <stdio.h>
#include <string.h>

int toNum(char *str);
int pow_int(int num, int sq);

int main(int argc, char **args)
{
    int sum = 0;
    for (int i = 1; i < argc; i++)
    {
        sum += toNum(args[i]);
    }

    printf("sum is %d", sum);
    return 0;
}

// 将字符串转换为数字
// 如果数字较大，可以把返回值改为 long long 类型
int toNum(char *str)
{
    size_t len = strlen(str);
    int res = 0;
    int minus = 0;
    int first = 0;
    int powTime = 0;

    if (str[0] == '-' && len > 1)
    {
        minus = 1;
        first += 1;
    }

    for (int i = len-1; i >= first; i--)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            printf("%s is not a number, so will ignore\n", str);
            return 0;
        }

        int num = str[i] - '0';
        res += num * pow_int(10, powTime);
        powTime += 1;
    }

    if (minus == 1)
    {
        res *= -1;
    }
    return res;
}

// 计算 num 的 powTime 次方
// 如果数字较大，可以把返回值改为 long long 类型
int pow_int(int num, int powTime)
{
    int res = 1;
    for (int i = 0; i < powTime; i++)
    {
        res *= num;
    }

    return res;
}