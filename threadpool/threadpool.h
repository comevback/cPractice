//
// Created by 吨吨 on 2025/6/9.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H
struct Task; // 前置声明
struct ThreadPool; // 前置声明

void *worker(void *arg);
void *manager(void *arg);
void threadDestroy(struct ThreadPool *pool);
struct ThreadPool* ThreadPoolCreate(int max, int min, int cap);
void ThreadPoolAdd(struct ThreadPool *pool, void (*func)(void *arg), void *arg);
int getThreadLiveNum(struct ThreadPool *pool);
int getThreadBusyNum(struct ThreadPool *pool);
int ThreadPoolDestroy (struct ThreadPool *pool);

#endif //THREADPOOL_H
