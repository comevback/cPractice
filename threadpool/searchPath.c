//
// Created by 吨吨 on 2025/6/10.
//
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include "threadpool.h"

void search(const char *path, const char *target, struct ThreadPool *pool);
void regex(void *arg);

struct taskBody
{
    char *path;
    char *target;
};

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("should have 3 argument");
        return 1;
    }

    const char *path = (char*) argv[1];
    const char *target = (char*) argv[2];

    struct ThreadPool *pool = ThreadPoolCreate(20, 5, 100);
    search(path, target, pool);

    sleep(30);
    ThreadPoolDestroy(pool);
}

void search(const char *path, const char *target, struct ThreadPool *pool)
{

    struct taskBody *task_body = malloc(sizeof(struct taskBody));
    task_body->target = malloc(sizeof(char) * 1024);
    task_body->path = malloc(sizeof(char) * 1024);
    strcpy(task_body->path, path);
    strcpy(task_body->target, target);
    ThreadPoolAdd(pool, regex, task_body);

    DIR *dir = opendir(path);
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
            search(newPath, target, pool);
        }
    }

    closedir(dir);
}

void regex(void *arg)
{
    struct taskBody *task = (struct taskBody*)arg;
    char *path = task->path;
    char *target = task->target;

    regex_t reg;

    if (regcomp(&reg, target, REG_EXTENDED) != 0)
    {
        printf("Fail to compile the regex %s\n", target);
        regfree(&reg);
        return;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        printf("无法打开目录: %s\n", path);
        regfree(&reg);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            if (regexec(&reg, entry->d_name, 0, NULL, 0) == 0)
            {
                printf("Successfully matched the file:\n %s-%s\n", path, entry->d_name);
            }
        }

    }

    closedir(dir);
    regfree(&reg);
    return;
}