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

#include "threadpool.h"

void search(const char *path, regex_t *reg, FILE *write, struct ThreadPool *pool);
void regex(void *arg);
static struct option long_options[];

// 任务体结构体
struct taskBody
{
    char *path;
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
    // 记录开始时间
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);


    char *path   = ".";
    char *nameRegex = NULL;
    char *outfile= NULL;

    // 解析命令行参数
    opterr = 0;
    const char *shortOpts = "p:r:o:h";
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
            case 'o':
                outfile = optarg;
                break;

            case 'h':
                printf("Usage: %s [options] <path> <regex>\n", argv[0]);
                printf("Options:\n");
                printf("  -h, --help          Show this help message\n");
                printf("  -p, --path <path>   Specify the path to search\n");
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
    if (!nameRegex)
    {
        printf("[Warning] Regex pattern is required.\n");
        return 1;
    }

    // 编译正则表达式
    regex_t reg;
    if (regcomp(&reg, nameRegex, REG_EXTENDED) != 0)
    {
        printf("Fail to compile the regex %s\n", nameRegex);
        regfree(&reg);
        return 1;
    }

    // 如果没有指定输出文件，默认输出到标准输出
    FILE *write = outfile ? fopen(outfile,"a") : stdout;
    if (!write)
    {
        printf("Fail to open the file for writing\n");
        return 1;
    }

    struct ThreadPool *pool = ThreadPoolCreate(30, 3, 100);
    search(path, &reg, write, pool);
    sleep(1); // 等待所有任务入队

    // 等所有 worker 退出
    while (1) {
        int live = getThreadBusyNum(pool);
        if (live == 0)
        {
            // 销毁线程池
            ThreadPoolDestroy(pool);
            break;
        }
        sleep(2);
    }


    // 记录结束时间并计算耗时
    gettimeofday(&end_time, NULL);
    double time_used = ((end_time.tv_sec - start_time.tv_sec) * 1000000u +
                       end_time.tv_usec - start_time.tv_usec) / 1.0e6;
    printf("[Time] Spent: %.2f s\n", time_used);

    // 释放资源
    regfree(&reg);
    fclose(write);
    return 0;
}

/* * 递归搜索指定路径下的所有子目录和文件
 * 如果匹配正则表达式，则将结果写入到指定文件或标准输出
 *
 * @param path 需要搜索的路径
 * @param reg 正则表达式
 * @param write 输出文件指针，如果为NULL则输出到标准输出
 * @param pool 线程池指针
 */
void search(const char *path, regex_t *reg, FILE *write, struct ThreadPool *pool)
{
    struct taskBody *task_body = malloc(sizeof(struct taskBody));
    task_body->path = malloc(sizeof(char) * 1024);
    task_body->reg = reg;
    task_body->write = write;
    strcpy(task_body->path, path);
    ThreadPoolAdd(pool, regex, task_body);
    // regex(task_body); // 直接调用函数处理当前目录

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

            char newPath[1024];
            snprintf(newPath, sizeof(newPath), "%s/%s", path, entry->d_name);
            search(newPath, reg, write, pool);
        }
    }
    closedir(dir);
}

/* * 正则表达式匹配函数
 * 该函数会在一个线程中执行，处理指定路径下的文件
 * 如果匹配成功，则打印或写入到指定文件
 *
 * @param arg 任务体指针，包含路径、正则表达式和输出文件指针
 */
void regex(void *arg)
{
    const struct taskBody *task = (struct taskBody*)arg;
    char *path = task->path;
    const regex_t *reg = task->reg;
    FILE *write = task->write;

    DIR *dir = opendir(path);
    if (!dir) {
        printf("[warning] Can not open dir:  %s\n", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            if (regexec(reg, entry->d_name, 0, NULL, 0) == 0)
            {
                fprintf(write ,"Successfully matched the file: %s : %s\n", path, entry->d_name);
            }
        }
    }
    closedir(dir);
    usleep(1000); // 模拟处理时间
    return;
}

// 长选项定义
static struct option long_options[] =
{
    {"path", 1, NULL, 'p'},
    {"regex", 1, NULL, 'r'},
    {"output", 1, NULL, 'o'},
    {"help", 0, NULL, 'h'},
    {0,0,0,0}
};