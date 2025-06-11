//
// Created by 吨吨 on 2025/6/10.
//
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <sys/time.h>
#include <pthread.h>
#include "threadpool.h"

// 全局文件写入互斥锁，防止多线程同时写入同一文件导致数据混乱
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void traverseAndScheduleSearch(const char *path, char *namePattern, regex_t *reg, FILE *write, struct ThreadPool *pool);
void findWithPattern(void *arg);
void findWithRegex(void *arg);
int matchPattern(const char *filename, const char *pattern);
static struct option long_options[];

// 任务体结构体
struct taskBody
{
    char *path;
    char *namePattern;
    regex_t *reg;
    FILE *write;
};

/* * 主函数
 * 解析命令行参数，编译正则表达式，创建线程池并开始搜索指定路径下的文件
 * 如果匹配正则表达式，则将结果写入到指定文件或标准输出
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 0 成功，非0 失败
 */
int main(const int argc, char *argv[])
{
    char *path   = ".";
    char *nameRegex = NULL;
    char *namePattern = NULL;
    char *outfile= NULL;

    // 解析命令行参数
    opterr = 0;
    const char *shortOpts = "p:r:n:o:h";
    int ch;
    while ((ch = getopt_long(argc, argv, shortOpts, long_options, NULL)) != -1)
    {
        switch (ch)
        {
            case 'p':
                path    = optarg;
                break;
            case 'r':
                nameRegex  = optarg;
                break;
            case 'n':
                namePattern = optarg;
                break;
            case 'o':
                outfile = optarg;
                break;

            case 'h':
                printf("Usage: %s [options] <path> <regex>\n", argv[0]);
                printf("Options:\n");
                printf("  -h, --help          Show this help message\n");
                printf("  -p, --path <path>   Specify the path to search\n");
                printf("  -n, --name <name>   Specify the file name pattern to match (supports * and ?)\n");
                printf("  -r, --regex <regex> Specify the regex pattern to match\n");
                printf("  -o, --output <file> Specify the output file (default: searchResult.txt)\n");
                exit(EXIT_SUCCESS);

            case '?':
                if (optopt == 'p' || optopt == 'r' || optopt == 'o')
                {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                }
                else
                {
                    fprintf(stderr, "Unknown option: %s\n", argv[optind - 1]);
                }
                return 1;

            default:
                fprintf(stderr, "Unexpected option: %c\n", ch);
                return 1;
        }
    }

    // 如果没有指定路径，使用当前目录
    if (!nameRegex && !namePattern)
    {
        printf("[Warning] Must specify either a regex or a name pattern.\n");
        return 1;
    }

    // 如果指定了正则表达式，则编译正则表达式，否则正则表达式的参数为NULL
    regex_t real_reg;
    regex_t *reg = NULL;
    if (nameRegex)
    {
        if (regcomp(&real_reg, nameRegex, REG_EXTENDED) != 0)
        {
            printf("Fail to compile the regex %s\n", nameRegex);
            regfree(&real_reg);
            return 1;
        }
        reg = &real_reg;
    }

    // 如果没有指定输出文件，默认输出到标准输出
    FILE *write = outfile ? fopen(outfile,"a") : stdout;
    if (!write)
    {
        printf("Fail to open the file for writing\n");
        return 1;
    }

    struct ThreadPool *pool = ThreadPoolCreate(30, 3, 100);
    traverseAndScheduleSearch(path, namePattern, reg, write, pool);
    sleep(1); // 等待所有任务入队，不然立即检查时，如果busyNum为0，可能还没开始就退出了
    ThreadPoolWaitAndDestroy(pool);

    // 释放资源
    if (reg) {regfree(reg);}
    fclose(write);
    return 0;
}

/* * 递归搜索指定路径下的所有子目录
 * 如果匹配正则表达式，则将结果写入到指定文件或标准输出
 *
 * @param path 需要搜索的路径
 * @param reg 正则表达式
 * @param namePattern 文件名模式字符串
 * @param write 输出文件指针，如果为NULL则输出到标准输出
 * @param pool 线程池指针
 */
void traverseAndScheduleSearch(const char *path, char* namePattern, regex_t *reg, FILE *write, struct ThreadPool *pool)
{
    // 在堆中构造任务体，释放是在regex函数中完成的
    struct taskBody *task_body = malloc(sizeof(struct taskBody));
    task_body->path = malloc(sizeof(char) * 1024);
    task_body->namePattern = namePattern;
    task_body->write = write;
    strcpy(task_body->path, path);

    // 如果有正则表达式，则使用正则表达式匹配函数，否则使用模式匹配函数
    if (reg != NULL)
    {
        task_body->reg = reg;
        // 如果有正则表达式，则添加到线程池中执行
        int ret = ThreadPoolAdd(pool, findWithRegex, task_body);
        if (ret != 0) {
            printf("[Error] Fail to add task to thread pool: %d\n", ret);
            free(task_body->path);
            free(task_body);
            return;
        }
    } else
    {
        int ret = ThreadPoolAdd(pool, findWithPattern, task_body);
        if (ret != 0) {
            printf("[Error] Fail to add task to thread pool: %d\n", ret);
            free(task_body->path);
            free(task_body);
            return;
        }
    }

    DIR *dir = opendir(path);
    if (!dir) {
        printf("[warning] Can not open dir:  %s\n", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            size_t len = strlen(path) + strlen(entry->d_name) + 2; // +2 for '/' and '\0'
            char *newPath = malloc(len);
            snprintf(newPath, len, "%s/%s", path, entry->d_name);
            traverseAndScheduleSearch(newPath, namePattern, reg, write, pool);
            free(newPath);
        }
    }
    closedir(dir);
}

/* * 模式匹配函数
 * 利用自定义的模式匹配函数来查找指定路径下的文件
 * 如果文件名匹配指定的模式，则将结果写入到指定文件或标准输出
 *
 * @param arg 任务体指针，包含路径、模式字符串和输出文件指针
 */
void findWithPattern(void *arg)
{
    struct taskBody *task = (struct taskBody*)arg;
    char *path = task->path;
    char *namePattern = task->namePattern;
    FILE *write = task->write;

    DIR *dir = opendir(path);
    if (!dir) {
        printf("[warning] Can not open dir:  %s\n", path);
        free(task->path);
        free(task);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            if (matchPattern(entry->d_name, namePattern))
            {
                pthread_mutex_lock(&file_mutex);
                fprintf(write ,"Successfully matched the file: %s : %s\n", path, entry->d_name);
                pthread_mutex_unlock(&file_mutex);
            }
        }
    }
    closedir(dir);
    free(task->path);
    free(task); // 释放任务体内存
    usleep(1000); // 模拟处理时间
    return;
}

/* * 正则表达式匹配函数
 * 该函数会在一个线程中执行，处理指定路径下的文件
 * 如果匹配成功，则打印或写入到指定文件
 *
 * @param arg 任务体指针，包含路径、正则表达式和输出文件指针
 */
void findWithRegex(void *arg)
{
    struct taskBody *task = (struct taskBody*)arg;
    char *path = task->path;
    const regex_t *reg = task->reg;
    FILE *write = task->write;

    DIR *dir = opendir(path);
    if (!dir) {
        printf("[warning] Can not open dir:  %s\n", path);
        free(task->path);
        free(task);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            if (regexec(reg, entry->d_name, 0, NULL, 0) == 0)
            {
                pthread_mutex_lock(&file_mutex);
                fprintf(write, "Successfully matched the file: %s : %s\n", path, entry->d_name);
                pthread_mutex_unlock(&file_mutex);
            }
        }
    }
    closedir(dir);
    free(task->path);
    free(task); // 释放任务体内存
    usleep(1000); // 模拟处理时间
    return;
}

/* * 模式匹配函数
 * 该函数使用递归方式实现通配符模式匹配
 * 支持 '*' 和 '?' 两种通配符
 *
 * @param filename 文件名
 * @param pattern 模式字符串
 * @return 1 如果匹配成功，0 如果匹配失败
 */
int matchPattern(const char *filename, const char *pattern)
{
    if (*pattern == '\0')
    {
        return *filename == '\0';
    }

    if (*pattern == '*')
    {
        while (*(pattern + 1) == '*')
        {
            pattern += 1; // 跳过连续的星号
        }

        if (*(pattern + 1) == '\0')
        {
            return 1; // 如果模式是以星号结尾，匹配成功
        }

        for (; *filename != '\0'; filename++)
        {
            if (matchPattern(filename, pattern + 1))
            {
                return 1; // 如果后续匹配成功，返回成功
            }
        }

        // 如果上面的循环没有找到匹配，那么filename此时已经是空字符串了，再匹配一下空字符看看
        return matchPattern(filename, pattern + 1);
    }

    // 如果两个字符相同，或者模式字符是问号（问号匹配一个任意字符），向下匹配
    if (*pattern == '?' || *pattern == *filename)
    {
        if (*filename == '\0')
        {
            return 0;
        }
        return matchPattern(filename + 1, pattern + 1);
    }

    return 0;
}

// 长选项定义
static struct option long_options[] =
{
    {"path", 1, NULL, 'p'},
    {"regex", 1, NULL, 'r'},
    {"name", 1, NULL, 'n'},
    {"output", 1, NULL, 'o'},
    {"help", 0, NULL, 'h'},
    {0,0,0,0}
};