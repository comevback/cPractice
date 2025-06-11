# C语言线程池实现

---

## 目录

1. [接口说明](#接口说明)
2. [数据结构](#数据结构)
3. [创建与销毁线程池](#创建与销毁线程池)
4. [添加任务](#添加任务)
5. [工作线程逻辑](#工作线程逻辑)
6. [管理线程逻辑](#管理线程逻辑)
7. [使用示例](#使用示例)
8. [总结](#总结)

---

## 接口说明

| 接口                               | 功能                 |
| -------------------------------- | ------------------ |
| `ThreadPoolCreate(max,min,cap)`  | 创建线程池              |
| `ThreadPoolAdd(pool,func,arg)`   | 添加任务               |
| `ThreadPoolWaitAndDestroy(pool)` | 等待所有任务完成并销毁线程池     |
| `ThreadPoolDestroy(pool)`        | 立即销毁线程池（需先确保无任务运行） |
| `getThreadBusyNum(pool)`         | 获取当前忙碌线程数          |
| `getThreadQueueSize(pool)`       | 获取当前队列中等待任务数       |

---

## 数据结构

```c
// 任务结构体
struct Task {
    void (*func)(void *arg);
    void *arg;
};

// 线程池结构体
struct ThreadPool {
    // 消息队列
    struct Task  *taskQueue;
    int           QueueCapacity;
    int           QueueSize;
    int           QueueFront;
    int           QueueRear;

    // 线程管理
    pthread_t     managerTid;
    pthread_t    *workers;

    // 工作线程状态
    int           max;
    int           min;
    int           busyNum;
    int           liveNum;
    int           quitNum;

    // 同步原语
    pthread_mutex_t mutex_pool;
    pthread_mutex_t mutex_busy;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;

    // 关闭标志
    int           shutdown;
};
```

---

## 创建与销毁线程池

### `ThreadPoolCreate(int max, int min, int cap)`

* **作用**：初始化线程池，创建管理线程和 `min` 个工作线程
* **关键步骤**

    1. 分配并初始化 `ThreadPool` 结构
    2. 初始化互斥锁和条件变量
    3. 启动管理线程 `manager`
    4. 启动并 detach `min` 个工作线程 `worker`

```c
struct ThreadPool* ThreadPoolCreate(int max, int min, int cap) {
    struct ThreadPool *pool = malloc(sizeof(*pool));
    // …初始化队列、锁、变量…
    pthread_create(&pool->managerTid, NULL, manager, pool);
    pool->workers = malloc(sizeof(pthread_t) * max);
    for (int i = 0; i < min; i++) {
        pthread_create(&pool->workers[i], NULL, worker, pool);
        pthread_detach(pool->workers[i]);
    }
    return pool;
}
```

### `ThreadPoolDestroy(ThreadPool *pool)`

* **作用**：通知所有线程退出，回收管理线程并释放资源
* **关键步骤**

    1. 设置 `pool->shutdown = 1`
    2. `pthread_cond_broadcast` 唤醒所有等待线程
    3. `pthread_join` 管理线程
    4. 循环等待 `liveNum` 归零
    5. 销毁锁、条件变量并 `free`

```c
int ThreadPoolDestroy(struct ThreadPool *pool) {
    pthread_mutex_lock(&pool->mutex_pool);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex_pool);

    pthread_cond_broadcast(&pool->not_empty);
    pthread_cond_broadcast(&pool->not_full);
    pthread_join(pool->managerTid, NULL);

    while (pool->liveNum > 0) {
        usleep(1000);
    }

    // 销毁同步原语并释放内存
    pthread_mutex_destroy(&pool->mutex_pool);
    pthread_mutex_destroy(&pool->mutex_busy);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    free(pool->taskQueue);
    free(pool->workers);
    free(pool);
    return 0;
}
```

---

## 添加任务

### `ThreadPoolAdd(ThreadPool *pool, void (*func)(void*), void *arg)`

* **作用**：将用户任务加入队列，并唤醒一个工作线程
* **关键步骤**

    1. 检查 `pool` 与 `func` 非空
    2. 当队列满或正在关闭时，阻塞或返回错误
    3. 将任务写入 `taskQueue[rear]`，更新 `rear`、`size`
    4. `pthread_cond_signal(&pool->not_empty)`

```c
int ThreadPoolAdd(struct ThreadPool *pool, void (*func)(void *), void *arg) {
    if (!pool || !func) return -1;
    pthread_mutex_lock(&pool->mutex_pool);
    while (pool->QueueSize == pool->QueueCapacity && !pool->shutdown) {
        pthread_cond_wait(&pool->not_full, &pool->mutex_pool);
    }
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex_pool);
        return -1;
    }
    // 入队
    pool->taskQueue[pool->QueueRear] = (struct Task){func, arg};
    pool->QueueRear = (pool->QueueRear + 1) % pool->QueueCapacity;
    pool->QueueSize++;
    pthread_mutex_unlock(&pool->mutex_pool);
    pthread_cond_signal(&pool->not_empty);
    return 0;
}
```

---

## 工作线程逻辑

```c
void *worker(void *arg) {
    ThreadPool *pool = arg;
    while (1) {
        pthread_mutex_lock(&pool->mutex_pool);
        // 等待新任务或退出信号
        while (pool->QueueSize == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->mutex_pool);
            // 动态退出逻辑（quitNum）…
        }
        if (pool->shutdown) {
            pool->liveNum--;
            pthread_mutex_unlock(&pool->mutex_pool);
            threadDestroy(pool);
        }
        // 取出任务
        struct Task task = pool->taskQueue[pool->QueueFront];
        pool->QueueFront = (pool->QueueFront + 1) % pool->QueueCapacity;
        pool->QueueSize--;
        pthread_mutex_unlock(&pool->mutex_pool);
        pthread_cond_signal(&pool->not_full);

        // 标记忙碌
        pthread_mutex_lock(&pool->mutex_busy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutex_busy);

        // 执行用户任务
        task.func(task.arg);

        // 标记空闲
        pthread_mutex_lock(&pool->mutex_busy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutex_busy);
    }
    return NULL;
}
```

---

## 管理线程逻辑

周期性（每 3 秒）检查，动态增减线程：

```c
void *manager(void *arg) {
    ThreadPool *pool = arg;
    while (!pool->shutdown) {
        sleep(3);
        // 取 liveNum、busyNum、QueueSize
        if (pool->QueueSize > pool->liveNum && pool->liveNum < pool->max) {
            // 增加 CHANGE_NUM 个线程
        }
        if (pool->liveNum > pool->busyNum * 2 && pool->liveNum > pool->min) {
            // 减少 CHANGE_NUM 个线程
        }
    }
    return NULL;
}
```
---

## 使用示例

### `testPool.c`

```c
int main() {
    ThreadPool *pool = ThreadPoolCreate(10, 3, 50);
    for (int i = 0; i < 100; i++) {
        int *num = malloc(sizeof(int));
        *num = i;
        ThreadPoolAdd(pool, runTask, num);
    }
    sleep(25);
    ThreadPoolDestroy(pool);
    return 0;
}
```

### `searchPath.c`

```c
// 并行目录搜索示例
ThreadPool *pool = ThreadPoolCreate(30, 3, 100);
search(path, &reg, write, pool);    // 递归投递任务
sleep(1);
ThreadPoolWaitAndDestroy(pool);
```

---

## 总结

* **固定队列**：容量上限，满时等待或返回
* **动态伸缩**：管理线程按任务量增减工作线程
* **安全同步**：互斥锁 + 条件变量，保证队列和计数器一致性
* **简洁接口**：`Create`／`Add`／`WaitAndDestroy`／`Destroy`

以上即为对 `threadpool` 模块的整理与格式化。
