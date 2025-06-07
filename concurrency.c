//
// Created by 吨吨 on 2025/6/6.
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

struct Person
{
    int id;
    char name[20];
    int age;
};

int main()
{
    pthread_t pid;    // 线程ID，用于标识创建的线程
    int realArg = 42;    // 线程参数，这里用一个整数作为示例
    void *arg = &realArg;    // 传递给线程函数的参数

    // 创建新线程
    // &pid: 存储新线程ID的指针
    // NULL: 使用默认线程属性
    // &start_thread: 线程将执行的函数指针(也可以不加&，函数名会自动转换为指针)
    // arg: 传递给线程函数的参数
    int res = pthread_create(&pid, NULL, &start_thread, arg);
    if (res != 0)
    {
        // 创建线程失败时的错误处理
        perror("something wrong");
        exit(EXIT_FAILURE);
    }

    // 等待子线程结束
    // pid: 要等待的线程ID
    // NULL: 不关心线程的返回值
    // 如果需要获取线程返回值，可以传入一个指向void*的指针
    void *ptr = NULL;
    // pthread_join() 需要修改主线程中某个“指针变量”的值，所以必须传入该变量的地址 → void **
    // 所以要用二级指针来接收线程的返回值
    int join_res = pthread_join(pid, &ptr);
    if (join_res != 0) {
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
    }

    struct Person p = *(struct Person *)ptr;  // 将返回值转换为Person结构体
    printf("thread returns: id: %d, name: %s, age: %d\n", p.id, p.name, p.age);

    // 打印主线程ID
    printf("main thread begins with thread pid: %lu\n", (unsigned long)pthread_self());

    free(ptr);  // 释放线程返回的堆区内存
    return 0;
}

/**
 * 线程执行函数的实现
 * @param arg 线程参数
 * @return 返回NULL表示成功完成
 */
void *start_thread(void *arg)
{
    sleep(1);  // 休眠1秒，模拟执行某些工作
    int id = *(int *)arg;
    printf("thread begins with thread pid: %lu\n", (unsigned long)pthread_self());
    struct Person *p = malloc(sizeof(struct Person));
    p->id = id;
    p->age = 5;
    strcpy(p->name, "dun dun");
    pthread_exit(p);
}