//
// Created by 吨吨 on 2025/6/7.
//
/**
 * 并发编程示例 - 演示POSIX线程(pthread)的基本使用
 * 本程序创建一个子线程，然后主线程和子线程分别执行相同的函数
 */

#include <stdio.h>    // 提供输入输出函数，如printf
#include <stdlib.h>   // 提供内存管理和程序控制函数，如exit()
#include <pthread.h>  // 提供POSIX线程相关函数和数据类型
#include <unistd.h>   // 提供sleep()函数
#include <string.h>

/**
 * 线程执行函数 - 被创建的线程将执行此函数
 * @param arg 传递给线程的参数
 * @return 线程的返回值
 */
void *start_thread(void *arg);
void *writeNum(void* arg);
void *readNum(void* arg);
int rw_main();

int number = 1;  // 全局变量，用于在不同线程中被加一，试验线程锁。
const int MAX = 10;  // 定义一个常量，表示最大值

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  // 初始化互斥锁
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;  // 初始化读写锁

int main()
{
    pthread_t pid1;    // 线程ID，用于标识创建的线程
    pthread_t pid2;   // 另一个线程ID，用于标识第二个线程
    int T1 = 1;    // 线程参数
    int T2 = 2;    // 另一个线程参数
    void *arg1 = &T1;    // 传递给线程函数的参数
    void *arg2 = &T2;    // 另一个线程参数

    int res1 = pthread_create(&pid1, NULL, &start_thread, arg1);
    int res2 = pthread_create(&pid2, NULL, &start_thread, arg2);
    if (res1 != 0 || res2 != 0)  // 检查线程创建是否成功
    {
        // 创建线程失败时的错误处理
        perror("something wrong");
        exit(EXIT_FAILURE);
    }

    int join_res1 = pthread_join(pid1, NULL);
    int join_res2 = pthread_join(pid2, NULL);
    if (join_res1 != 0 || join_res2 != 0)  // 检查线程等待是否成功
    {
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_destroy(&lock);

    // 打印主线程ID
    printf("main thread begins with thread pid: %lu\n", (unsigned long)pthread_self());

    // rw_main();
    return 0;
}

int rw_main()
{
    pthread_t rpid[5];  // 定义5个读线程
    pthread_t wpid[3];  // 定义2个写线程

    for (int i = 0; i < 5; i++)
    {
        pthread_create(&rpid[i], NULL, readNum, NULL);  // 创建读线程
    }

    for (int i = 0; i < 3; i++)
    {
        pthread_create(&wpid[i], NULL, writeNum, NULL);  // 创建写线程
    }

    for (int i = 0; i < 5; i++)
    {
        pthread_join(rpid[i], NULL);  // 等待所有读线程结束
    }

    for (int i = 0; i < 3; i++)
    {
        pthread_join(wpid[i], NULL);  // 等待写线程结束
    }

    printf("all threads finished\n");
    pthread_rwlock_destroy(&rwlock);  // 销毁读写锁
    return 0;
}

/**
 * 线程执行函数的实现
 * @param arg 线程参数
 * @return 返回NULL表示成功完成
 */
void *start_thread(void *arg)
{
    int threadNum = *(int *)arg;  // 将传入的参数转换为整数
    for (int i = 0; i < MAX; i++)  // 循环10次
    {
        pthread_mutex_lock(&lock);  // 加锁，确保对共享资源的访问是线程安全的
        int cur = number;  // 获取主线程的参数
        cur += 1;  // 在子线程中对参数进行加一操作
        printf("thread number %d\n add the number: %d to current: %d\n", threadNum, number, cur);
        number = cur;  // 更新主线程的参数
        pthread_mutex_unlock(&lock);  // 解锁，允许其他线程访问共享资源

        //usleep(1);  // 模拟一些工作，休眠1秒
    }
}

// 写的线程的处理函数
void* writeNum(void* arg)
{
    for (int i = 0; i < MAX; i++)  // 循环10次
    {
        pthread_rwlock_wrlock(&rwlock);
        int cur = number;
        cur ++;
        number = cur;
        printf("+1写操作完毕, number : %d, tid = %ld\n", number, pthread_self());
        pthread_rwlock_unlock(&rwlock);
        // 添加sleep目的是要看到多个线程交替工作
        usleep(rand() % 100);
    }
}

// 读线程的处理函数
// 多个线程可以如果处理动作相同, 可以使用相同的处理函数
// 每个线程中的栈资源是独享
void* readNum(void* arg)
{
    for (int i = 0; i < MAX; i++)  // 循环10次
    {
        pthread_rwlock_rdlock(&rwlock);
        printf("全局变量number = %d, tid = %ld\n", number, pthread_self());
        pthread_rwlock_unlock(&rwlock);
        usleep(rand() % 100);
    }
}
