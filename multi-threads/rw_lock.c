//
// Created by 吨吨 on 2025/6/8.
//
/**
 * 并发编程示例 - 演示POSIX线程(pthread)的读写锁使用
 * 本程序创建多个读线程和写线程，演示读写锁的使用
 */

#include <stdio.h>    // 提供输入输出函数，如printf
#include <stdlib.h>   // 提供内存管理和程序控制函数，如exit()
#include <pthread.h>  // 提供POSIX线程相关函数和数据类型
#include <unistd.h>   // 提供sleep()函数

int number = 1;  // 全局变量，用于被读写线程访问
const int MAX = 10;  // 定义一个常量，表示最大值

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;  // 初始化读写锁

// 写线程的处理函数
void* writeNum(void* arg)
{
    for (int i = 0; i < MAX; i++)  // 循环10次
    {
        pthread_rwlock_wrlock(&rwlock);  // 获取写锁
        int cur = number;
        cur++;
        number = cur;
        printf("+1写操作完毕, number : %d, tid = %ld\n", number, pthread_self());
        pthread_rwlock_unlock(&rwlock);  // 释放锁
        // 添加sleep目的是要看到多个线程交替工作
        usleep(rand() % 100);
    }
    return NULL;
}

// 读线程的处理函数
void* readNum(void* arg)
{
    for (int i = 0; i < MAX; i++)  // 循环10次
    {
        pthread_rwlock_rdlock(&rwlock);  // 获取读锁
        printf("全局变量number = %d, tid = %ld\n", number, pthread_self());
        pthread_rwlock_unlock(&rwlock);  // 释放锁
        usleep(rand() % 100);
    }
    return NULL;
}

int main()
{
    pthread_t rpid[5];  // 定义5个读线程
    pthread_t wpid[3];  // 定义3个写线程

    // 创建读线程
    for (int i = 0; i < 5; i++)
    {
        pthread_create(&rpid[i], NULL, readNum, NULL);  // 创建读线程
    }

    // 创建写线程
    for (int i = 0; i < 3; i++)
    {
        pthread_create(&wpid[i], NULL, writeNum, NULL);  // 创建写线程
    }

    // 等待所有读线程结束
    for (int i = 0; i < 5; i++)
    {
        pthread_join(rpid[i], NULL);
    }

    // 等待所有写线程结束
    for (int i = 0; i < 3; i++)
    {
        pthread_join(wpid[i], NULL);
    }

    printf("all threads finished\n");
    pthread_rwlock_destroy(&rwlock);  // 销毁读写锁
    return 0;
}