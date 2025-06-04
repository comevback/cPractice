//
// Created by 吨吨 on 2025/6/4.
//
#include <stdio.h>

int main(const int argc, char *args[])
{
    if (argc == 1)
    {
        printf("didn't input any argurment");
        return 0;
    }
    for (int i = 1; i< argc; i++)
    {
        printf("argurment in number %d is %s.\n", i, args[i]);
    }
}