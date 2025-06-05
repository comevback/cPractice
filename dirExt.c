//
// Created by 吨吨 on 2025/6/5.
//
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int scanDir(const char *path, int recursive);

int main()
{
    const char *path = ".";
    scanDir(path, 1);
}

int scanDir(const char *path, int recursive)
{
    if (recursive < 0)
    {
        return 0;
    }
    DIR *d = opendir(path);
    if (!d)
    {
        perror("can not open the path");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        if (entry -> d_type == DT_REG)
        {
            printf("found file: %s%s\n", path, entry->d_name);
        }

        if (entry -> d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            printf("found dir: %s/%s\n", path, entry->d_name);

            // char *newPath = malloc(1024 * sizeof(char));
            // snprintf(newPath, 1024, "%s/%s", path, entry->d_name);
            // scanDir(newPath);
            // free(newPath);

            char newPath[1024];
            int len = snprintf(newPath, sizeof(newPath), "%s/%s", path, entry->d_name);
            if (len < 0)
            {
                printf("encoding error\n");
            }
            else if (len > sizeof(newPath))
            {
                printf("path is too long\n");
                closedir(d);
                return 0;
            }
            scanDir(newPath, recursive-1);
        }
    }

    closedir(d);
    return 0;
}
