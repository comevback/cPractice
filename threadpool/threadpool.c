//
// Created by 吨吨 on 2025/6/9.
//
#include "threadpool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define CHANGE_NUM 2

void *worker(void *arg);
void *manager(void *arg);
void threadDestroy(struct ThreadPool *pool);
struct ThreadPool* ThreadPoolCreate(int max, int min, int cap);
int ThreadPoolAdd(struct ThreadPool *pool, void (*func)(void *arg), void *arg);
int getThreadLiveNum(struct ThreadPool *pool);
int getThreadBusyNum(struct ThreadPool *pool);
int ThreadPoolDestroy (struct ThreadPool *pool);
void ThreadPoolWaitAndDestroy(struct ThreadPool *pool);
int getThreadQueueSize(struct ThreadPool *pool);

struct timeval start_time, end_time;

struct Task
{
    void (*func) (void *arg);
    void *arg;
};

struct ThreadPool
{
    // 消息队列
    struct Task *taskQueue;
    int QueueCapacity;
    int QueueSize;
    int QueueFront;
    int QueueRear;

    // 线程
    pthread_t managerTid;
    pthread_t *workers;

    // 工作线程（消费者）数组
    int max;
    int min;
    int busyNum;
    int liveNum;
    int quitNum;

    // 锁和条件变量
    pthread_mutex_t mutex_pool;
    pthread_mutex_t mutex_busy;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

    // 销毁
    int shutdown;
};

// 创建线程池函数
struct ThreadPool* ThreadPoolCreate(int max, int min, int cap)
{
    // 记录开始时间
    gettimeofday(&start_time, NULL);

    struct ThreadPool *pool = malloc(sizeof(struct ThreadPool));
    do
    {
        if (pool == NULL)
        {
            printf("Fail to create a threadpool\n");
            break;
        }

        pool->taskQueue = malloc(sizeof(struct Task) * cap);
        if (pool->taskQueue == NULL)
        {
            printf("Fail to create a taskQueue\n");
            break;
        }

        pool->QueueCapacity = cap;
        pool->QueueSize = 0;
        pool->QueueFront = 0;
        pool->QueueRear = 0;

        pool->max = max;
        pool->min = min;
        pool->liveNum = min; // 因为初始化时候先往workers里加入min个线程
        pool->busyNum = 0;
        pool->quitNum = 0;
        pool->shutdown = 0;

        if (pthread_mutex_init(&pool->mutex_pool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutex_busy, NULL) != 0 ||
            pthread_cond_init(&pool->not_empty, NULL) != 0 ||
            pthread_cond_init(&pool->not_full, NULL) != 0)
        {
            printf("lock can not be inited\n");
            break;
        }

        pthread_create(&pool->managerTid, NULL, &manager, pool);
        pool->workers = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if (pool->workers == NULL)
        {
            printf("Fail to create a worker queue\n");
            break;
        }

        memset(pool->workers, 0, sizeof(pthread_t) * max);
        for (int i = 0; i < min; i++)
        {
            pthread_create(&pool->workers[i], NULL, &worker, pool);
            pthread_detach(pool->workers[i]); // 分离线程
        }

        printf("total live threads: %d\n", pool->liveNum);
        return pool;
    } while (0);
    if (pool->workers)
    {
        free(pool->workers);
    }
    if (pool->taskQueue)
    {
        free(pool->taskQueue);
    }
    if (pool)
    {
        free(pool);
    }
    return NULL;
}

// 销毁线程池函数
int ThreadPoolDestroy (struct ThreadPool *pool)
{
    printf("[Main] Shutting down... broadcasting to all worker threads.\n");
    if (pool == NULL)
    {
        printf("pool not exist\n");
        return -1;
    }

    pthread_mutex_lock(&pool->mutex_pool);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex_pool);

    // 可以用pthread_cond_broadcast来唤醒所有线程
    pthread_cond_broadcast(&pool->not_empty);
    pthread_cond_broadcast(&pool->not_full);

    // 等待管理线程退出
    pthread_join(pool->managerTid, NULL);

    // 等所有 worker 退出
    while (1) {
        pthread_mutex_lock(&pool->mutex_pool);
        int live = pool->liveNum;
        pthread_mutex_unlock(&pool->mutex_pool);
        if (live == 0) break;
        usleep(1000);
    }

    pthread_mutex_destroy(&pool->mutex_pool);
    pthread_mutex_destroy(&pool->mutex_busy);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);

    if (pool->taskQueue)
    {
        free(pool->taskQueue);
    }

    if (pool->workers)
    {
        free(pool->workers);
    }

    // 记录结束时间并计算耗时
    gettimeofday(&end_time, NULL);
    double time_used = ((end_time.tv_sec - start_time.tv_sec) * 1000000u +
                       end_time.tv_usec - start_time.tv_usec) / 1.0e6;
    printf("[Time] Task Spent: %.2f s\n", time_used);

    free(pool);
    pool = NULL;
    return 0;
}

// 添加任务到线程池函数
int ThreadPoolAdd(struct ThreadPool *pool, void (*func)(void *arg), void *arg)
{
    if (pool == NULL || func == NULL)
    {
        printf("pool or func not exist\n");
        return -1; // 线程池或任务函数不存在
    }

    struct Task task;
    task.arg = arg;
    task.func = func;

    pthread_mutex_lock(&pool->mutex_pool);

    while (pool->QueueSize >= pool->QueueCapacity && !pool->shutdown)
    {
        pthread_cond_wait(&pool->not_full, &pool->mutex_pool);
    }

    if (pool->shutdown)
    {
        printf("pool already shutdown\n");
        pthread_mutex_unlock(&pool->mutex_pool);
        return -1; // 线程池已关闭，无法添加任务
    }

    if (pool->QueueSize >= pool->QueueCapacity)
    {
        printf("task queue full, can not add\n");
        pthread_mutex_unlock(&pool->mutex_pool);
        return -1; // 任务队列已满，无法添加任务
    }

    pool->taskQueue[pool->QueueRear] = task;
    pool->QueueRear = (pool->QueueRear + 1) % pool->QueueCapacity;
    pool->QueueSize += 1;

    pthread_mutex_unlock(&pool->mutex_pool);
    pthread_cond_signal(&pool->not_empty);

    return 0; // 成功添加任务
}

// 等待所有任务完成再销毁线程池函数
void ThreadPoolWaitAndDestroy(struct ThreadPool *pool)
{
    if (pool == NULL)
    {
        printf("pool not exist\n");
        return;
    }

    // 等待所有任务完成
    while (getThreadBusyNum(pool) > 0 || getThreadQueueSize(pool) > 0)
    {
        usleep(1000); // 等待1毫秒
    }

    // 等待所有工作线程退出后就可以销毁线程池
    ThreadPoolDestroy(pool);
}

// 销毁单个线程函数
void threadDestroy(struct ThreadPool *pool)
{
    pthread_t tid = pthread_self();
    pthread_mutex_lock(&pool->mutex_pool);
    for (int i = 0; i < pool->max; i++)
    {
        if (tid == pool->workers[i])
        {
            pool->workers[i] = 0;
            printf("[Action] Destroy the thread [%ld]\n", (long int)tid);
            printf("[Status] Total live threads: %d\n", pool->liveNum);
            break;
        }
    }
    pthread_mutex_unlock(&pool->mutex_pool);
    pthread_exit(NULL);
}

// 获取当前存活线程数
int getThreadLiveNum(struct ThreadPool *pool)
{
    pthread_mutex_lock(&pool->mutex_pool);
    int liveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutex_pool);
    return liveNum;
}

// 获取当前繁忙线程数
int getThreadBusyNum(struct ThreadPool *pool)
{
    pthread_mutex_lock(&pool->mutex_busy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutex_busy);
    return busyNum;
}

int getThreadQueueSize(struct ThreadPool *pool)
{
    pthread_mutex_lock(&pool->mutex_pool);
    int queueSize = pool->QueueSize;
    pthread_mutex_unlock(&pool->mutex_pool);
    return queueSize;
}

/** * 工作线程函数，循环从任务队列中取出任务并执行
 * 具体思路：
 * 1.不断循环，直到线程池被销毁
 * 2.每次循环中：先上锁，等待条件变量not_empty被唤醒，如果唤醒时发现需要减少线程或者线程池被销毁，用threadDestroy销毁线程
 * 3.如果是因为有任务被唤醒，则取出任务，解锁，执行任务
 *
 * @param arg 线程池指针
 * @return NULL
 */
void *worker(void *arg)
{
    struct ThreadPool *pool = (struct ThreadPool*) arg;
    while (1)
    {
        pthread_mutex_lock(&pool->mutex_pool);
        while (pool->QueueSize == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->not_empty, &pool->mutex_pool);
            if (pool->quitNum != 0)
            {
                pool->quitNum -= 1;
                if (pool->liveNum > pool->min)
                {
                    pool->liveNum -= 1;
                    pthread_mutex_unlock(&pool->mutex_pool);
                    threadDestroy(pool);
                }
            }
        }

        if (pool->shutdown)
        {
            pool->liveNum -= 1;
            pthread_mutex_unlock(&pool->mutex_pool);
            threadDestroy(pool);
        }

        struct Task task;
        task.func = pool->taskQueue[pool->QueueFront].func;
        task.arg = pool->taskQueue[pool->QueueFront].arg;
        pool->QueueFront = (pool->QueueFront + 1) % pool->QueueCapacity;
        pool->QueueSize -= 1;

        pthread_mutex_unlock(&pool->mutex_pool);
        pthread_cond_signal(&pool->not_full);

        pthread_mutex_lock(&pool->mutex_busy);
        pool->busyNum += 1;
        pthread_mutex_unlock(&pool->mutex_busy);

        task.func(task.arg);

        pthread_mutex_lock(&pool->mutex_busy);
        pool->busyNum -= 1;
        pthread_mutex_unlock(&pool->mutex_busy);
    }
    return NULL;
}

/** 管理线程函数，定期检查线程池状态
 * 具体思路：
 * 1.每隔3秒检查一次线程池状态
 * 2.如果存活线程数小于任务数且小于最大线程数，则增加2个线程
 * 3.如果存活线程数大于繁忙线程数的两倍且大于最小线程数，则减少2个线程
 * 4.打印当前存活线程数
 *
 * @param arg 线程池指针
 * @return NULL
 */
void *manager(void *arg)
{
    struct ThreadPool *pool = (struct ThreadPool*) arg;
    while (!pool->shutdown)
    {
        sleep(3);

        pthread_mutex_lock(&pool->mutex_pool);
        int liveNum = pool->liveNum;
        int taskSize = pool->QueueSize;
        int maxNum = pool->max;
        int minNUm = pool->min;
        pthread_mutex_unlock(&pool->mutex_pool);

        pthread_mutex_lock(&pool->mutex_busy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutex_busy);

        // 如果存活线程数量小于目前的任务数量，增加
        if (taskSize > liveNum && liveNum < maxNum)
        {
            printf("[Action] Threads + %d\n", CHANGE_NUM);
            pthread_mutex_lock(&pool->mutex_pool);
            int count = 0;
            for (int i = 0; i < pool->max && count < CHANGE_NUM && pool->liveNum < pool->max; i++)
            {
                if (pool->workers[i] == 0)
                {
                    pool->liveNum += 1;
                    pthread_create(&pool->workers[i], NULL, worker, pool);
                    pthread_detach(pool->workers[i]); // 分离线程
                    count += 1;
                }
            }
            pthread_mutex_unlock(&pool->mutex_pool);
        }

        // 如果存活线程数量是目前的繁忙线程数的两倍以上，减少
        if (liveNum > busyNum * 2 && liveNum > minNUm)
        {
            printf("[Action] Threads - %d\n", CHANGE_NUM);
            pthread_mutex_lock(&pool->mutex_pool);
            pool -> quitNum = CHANGE_NUM;
            pthread_mutex_unlock(&pool->mutex_pool);

            for (int i = 0; i < CHANGE_NUM; i++)
            {
                pthread_cond_signal(&pool->not_empty);
            }
        }

        pthread_mutex_lock(&pool->mutex_pool);
        printf("[Status] Total live threads: %d, Busy threads: %d, Queue size: %d\n", pool->liveNum, busyNum, pool->QueueSize);
        pthread_mutex_unlock(&pool->mutex_pool);
    }
    return NULL;
}