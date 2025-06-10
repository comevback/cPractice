//
// Created by 吨吨 on 2025/6/5.
//
// 文件功能：实现目录扫描器，可以递归搜索指定后缀的文件
// 主要函数：scanDir用于目录扫描，checkSuffix用于检查文件后缀

#include <stdio.h>    // 标准输入输出库
#include <dirent.h>   // 目录操作库，提供DIR, dirent等结构和函数
#include <stdlib.h>   // 标准库函数，提供内存分配等功能
#include <string.h>   // 字符串处理库

// 函数声明
int scanDir(const char *path, int recursive);  // 扫描指定路径下的文件
int checkSuffix(const char *filename, const char *suffix);  // 检查文件后缀

/**
 * 主函数：程序入口点
 * 调用scanDir函数扫描当前目录("."),递归深度为1
 */
int main()
{
    const char *path = ".";   // 设置扫描路径为当前目录
    scanDir(path, 1);         // 从当前目录开始，递归深度为1进行扫描
}

/**
 * 扫描目录函数
 *
 * @param path 要扫描的目录路径
 * @param recursive 递归深度，表示还能深入几层子目录
 * @return 成功返回0，失败返回1
 *
 * 功能：扫描指定目录，查找所有.txt后缀文件，并可递归查找子目录
 */
int scanDir(const char *path, int recursive)
{
    // 递归深度检查，小于0时停止递归
    if (recursive < 0)
    {
        return 0;
    }

    // 打开目录
    DIR *d = opendir(path);
    if (!d)
    {
        perror("can not open the path");  // 目录打开失败，输出错误信息
        return 1;
    }

    struct dirent *entry;  // 目录项结构指针

    // 循环读取目录中的每一项
    while ((entry = readdir(d)) != NULL)
    {
        // 处理普通文件
        if (entry->d_type == DT_REG)  // DT_REG表示常规文件
        {
            // 检查文件是否以txt后缀结尾
            if (checkSuffix(entry->d_name, "txt"))
            {
                printf("found file: %s%s\n", path, entry->d_name);  // 打印找到的文件
            }
        }

        // 处理目录
        if (entry->d_type == DT_DIR)  // DT_DIR表示目录
        {
            // 跳过当前目录(.)和父目录(..)
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            // 下面注释掉的代码是使用动态内存分配构建子目录路径的另一种方式
            // char *newPath = malloc(1024 * sizeof(char));
            // snprintf(newPath, 1024, "%s/%s", path, entry->d_name);
            // scanDir(newPath);
            // free(newPath);

            // 构造子目录的完整路径
            char newPath[1024];  // 存储子目录路径的缓冲区
            int len = snprintf(newPath, sizeof(newPath), "%s/%s", path, entry->d_name);

            // 检查路径构造是否成功
            if (len < 0)
            {
                printf("encoding error\n");  // 编码错误
            }
            else if (len > sizeof(newPath))
            {
                printf("path is too long\n");  // 路径过长
                closedir(d);
                return 0;
            }

            // 递归扫描子目录，递归深度减1
            scanDir(newPath, recursive-1);
        }
    }

    // 关闭目录
    closedir(d);
    return 0;
}

/**
 * 检查文件后缀函数
 *
 * @param filename 文件名
 * @param suffix 要检查的后缀
 * @return 如果文件名以指定后缀结尾返回1，否则返回0
 *
 * 功能：判断文件名是否以指定后缀结尾
 */
int checkSuffix(const char *filename, const char *suffix)
{
    size_t fileLen = strlen(filename);   // 文件名长度
    size_t fixLen = strlen(suffix);      // 后缀长度

    // 如果文件名比后缀短，肯定不匹配
    if (fileLen < fixLen)
    {
        return 0;
    }

    // 空后缀视为匹配所有文件
    if (fixLen == 0)
    {
        return 1;
    }

    // 计算需要比较的起始位置，并进行字符串比较
    const size_t index = fileLen - fixLen;
    const int res = strcmp(filename + index, suffix) == 0;
    return res;
}