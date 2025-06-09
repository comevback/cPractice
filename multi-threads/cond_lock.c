//
// Created by 吨吨 on 2025/6/9.
//
// 示例程序：多生产者/多消费者
// - 环形数组缓冲区示例（producer_array / customer_array）
// - 链表缓冲区示例（producer_listnode / customer_listnode）
//
// 生产者往缓冲区写入随机数，消费者从缓冲区消费并打印。
// 用互斥锁 + 条件变量实现满/空等待与唤醒。

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

//---------------------------
// 全局数据结构与变量
//---------------------------

// 链表节点定义，用于链表缓冲区
struct Node {
    int number;         // 存放的数据
    struct Node *next;  // 指向下一个节点
};

// 链表头指针，NULL 表示链表为空
struct Node *head = NULL;

// 环形数组缓冲区定义
int list[10];  // 存放数据的数组，容量为 10
int put = 0;   // 下一个写入位置索引（[0,9] 循环）
int take = 0;  // 下一个读取位置索引（[0,9] 循环）
int count = 0; // 当前缓冲区中元素数量

//---------------------------
// 链表缓冲区同步原语
//---------------------------

// 保护对 head 的互斥锁
pthread_mutex_t mutex_listnode = PTHREAD_MUTEX_INITIALIZER;
// 当链表为空时，消费者在此等待；生产者写入后发出 signal
pthread_cond_t cond_listnode = PTHREAD_COND_INITIALIZER;

//---------------------------
// 数组缓冲区同步原语
//---------------------------

// 保护对 list, put, take, count 访问的互斥锁
pthread_mutex_t mutex_array = PTHREAD_MUTEX_INITIALIZER;
// 当缓冲区满 (count==10) 时，生产者在 cond_array_full 上等待
pthread_cond_t cond_array_full = PTHREAD_COND_INITIALIZER;
// 当缓冲区空 (count==0) 时，消费者在 cond_array_empty 上等待
pthread_cond_t cond_array_empty = PTHREAD_COND_INITIALIZER;

//---------------------------
// 环形数组生产者
//---------------------------
void *producer_array(void *arg)
{
    int id = *(int *)arg;  // 线程编号，用于打印区分
    while (1)
    {
        // 获取数组缓冲区锁
        pthread_mutex_lock(&mutex_array);

        // 如果缓冲区满，则等待生产者条件变量
        while (count == 10)
        {
            // 等待时会自动释放 mutex_array，唤醒后重新加锁
            pthread_cond_wait(&cond_array_full, &mutex_array);
            printf("[Producer %d] 缓冲区已满，等待中...\n", id);
        }

        // 生成随机数并写入缓冲区
        int num = rand() % 100;
        list[put] = num;              // 写入当前 put 位置
        printf("[Producer %d] 写入数组: list[%d] = %d\n", id, put, num);

        // 更新索引和计数
        put = (put + 1) % 10;         // 环形递增
        count += 1;                      // 元素计数 +1

        // 通知一个等待的消费者：缓冲区不空了
        pthread_cond_signal(&cond_array_empty);

        // 释放数组缓冲区锁
        pthread_mutex_unlock(&mutex_array);

        // 模拟生产耗时
        sleep(1);
    }
    return NULL;
}

//---------------------------
// 环形数组消费者
//---------------------------
void *customer_array(void *arg)
{
    int id = *(int *)arg;  // 线程编号
    while (1)
    {
        // 获取数组缓冲区锁
        pthread_mutex_lock(&mutex_array);

        // 如果缓冲区空，则等待消费者条件变量
        while (count == 0)
        {
            // 等待时自动释放锁，唤醒后重新加锁
            pthread_cond_wait(&cond_array_empty, &mutex_array);
            printf("[Consumer %d] 缓冲区为空，等待中...\n", id);
        }

        // 从缓冲区读取数据
        int num = list[take];         // 读当前位置
        printf("[Consumer %d] 读取数组: list[%d] = %d\n", id, take, num);

        // 更新索引和计数
        take = (take + 1) % 10;       // 环形递增
        count -= 1;                      // 元素计数 -1

        // 通知一个等待的生产者：缓冲区不满了
        pthread_cond_signal(&cond_array_full);

        // 释放数组缓冲区锁
        pthread_mutex_unlock(&mutex_array);

        // 模拟消费耗时（500ms）
        sleep(5);
    }
    return NULL;
}

//---------------------------
// 链表缓冲区生产者
//---------------------------
void *producer_listnode(void *arg)
{
    int id = *(int *)arg;  // 线程编号
    while (1)
    {
        // 获取链表缓冲区锁
        pthread_mutex_lock(&mutex_listnode);

        // 分配并初始化新节点
        struct Node *cur = malloc(sizeof(*cur));
        if (!cur) {
            perror("malloc 节点失败");
            pthread_mutex_unlock(&mutex_listnode);
            break;
        }
        cur->number = rand() % 100;
        cur->next   = head;   // 插入到链表头
        head        = cur;
        printf("[Producer %d] 插入链表: %d\n", id, cur->number);

        // 释放链表缓冲区锁
        pthread_mutex_unlock(&mutex_listnode);

        // 通知一个等待的消费者：链表不空了
        pthread_cond_signal(&cond_listnode);

        // 模拟生产耗时
        sleep(1);
    }
    return NULL;
}

//---------------------------
// 链表缓冲区消费者
//---------------------------
void *customer_listnode(void *arg)
{
    int id = *(int *)arg;  // 线程编号
    while (1)
    {
        // 获取链表缓冲区锁
        pthread_mutex_lock(&mutex_listnode);

        // 如果链表空，则等待
        while (head == NULL)
        {
            pthread_cond_wait(&cond_listnode, &mutex_listnode);
            printf("[Consumer %d] 链表为空，等待中...\n", id);
        }

        // 取出链表头节点并释放
        struct Node *cur = head;
        printf("[Consumer %d] 取出链表: %d\n", id, cur->number);
        head = cur->next;
        free(cur);

        // 释放链表缓冲区锁
        pthread_mutex_unlock(&mutex_listnode);

        // 模拟消费耗时（500ms）
        usleep(500000);
    }
    return NULL;
}

//---------------------------
// 主函数：创建线程 & 清理
//---------------------------
int main()
{
    srand((unsigned)time(NULL));  // 初始化随机数种子

    pthread_t ppid_array[2], ctid_array[5];
    pthread_t ppid_list[2], ctid_list[5];

    int producer_ids[2], customer_ids[5];

    // 创建环境：2 个数组生产者 + 5 个数组消费者
    for (int i = 0; i < 2; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&ppid_array[i], NULL, producer_array, &producer_ids[i]);
    }
    for (int i = 0; i < 5; i++) {
        customer_ids[i] = i + 1;
        pthread_create(&ctid_array[i], NULL, customer_array, &customer_ids[i]);
    }

    // // 创建环境：2 个链表生产者 + 5 个链表消费者
    // for (int i = 0; i < 2; i++) {
    //     pthread_create(&ppid_list[i], NULL, producer_listnode, &producer_ids[i]);
    // }
    // for (int i = 0; i < 5; i++) {
    //     pthread_create(&ctid_list[i], NULL, customer_listnode, &customer_ids[i]);
    // }

    // 等待所有线程（示例中实际永不返回，因为线程循环不退出）
    for (int i = 0; i < 2; i++) {
        pthread_join(ppid_array[i], NULL);
        // pthread_join(ppid_list[i], NULL);
    }
    for (int i = 0; i < 5; i++) {
        pthread_join(ctid_array[i], NULL);
        // pthread_join(ctid_list[i], NULL);
    }

    // 销毁所有互斥锁和条件变量
    pthread_cond_destroy(&cond_array_full);
    pthread_cond_destroy(&cond_array_empty);
    pthread_mutex_destroy(&mutex_array);

    pthread_cond_destroy(&cond_listnode);
    pthread_mutex_destroy(&mutex_listnode);

    return 0;
}
