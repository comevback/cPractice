//
// Created by 吨吨 on 2025/6/10.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "threadpool.h"

void runTask(void *arg)
{
    int id = *(int*)arg;
    FILE *write = fopen("./testThread.txt", "a");

    time_t now = time(NULL); // 获取当前时间戳
    struct tm *t = localtime(&now); // 转换为本地时间
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t); // 格式化为字符串

    fprintf(write, "write from id %d at %s\n", id, buf);
    fclose(write);
    sleep(1);
    return;
}

int main()
{
    struct ThreadPool *pool = ThreadPoolCreate(10, 3, 50);
    for (int i = 0; i < 100; i++)
    {
        int *num = malloc(sizeof(int));
        *num = i;
        ThreadPoolAdd(pool, runTask, num);
    }

    sleep(25);
    ThreadPoolDestroy(pool);
    return 0;
}