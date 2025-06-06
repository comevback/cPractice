//
// Created by 吨吨 on 2025/6/3.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* replace(const char *str, const char *origin, const char *replace);
int upperFile(FILE *in, FILE *upper);
int copyFile(FILE *in, FILE *out);
int replaceFile(FILE *in, FILE *rep);
int reverseFile(FILE *in, FILE *reverse);

int main()
{
    FILE *in = fopen("sample_input.txt", "r");
    FILE *out = fopen("copy.txt", "w");
    FILE *rep = fopen("replace.txt" , "w");
    FILE *upper = fopen("uppercase.txt" , "w");
    FILE *reverse = fopen("reversed.txt", "w");

    if (in == NULL || out == NULL || rep == NULL || upper == NULL || reverse == NULL)
    {
        printf("can not open files");
        // 先关闭任何已打开的
        if (in) fclose(in);
        if (out) fclose(out);
        if (rep) fclose(rep);
        if (upper) fclose(upper);
        if (reverse) fclose(reverse);
        return 1;
    }

    copyFile(in, out);
    rewind(in);

    upperFile(in, upper);
    rewind(in);

    reverseFile(in, reverse);
    rewind(in);

    replaceFile(in, rep);

    fclose(in);
    fclose(out);
    fclose(rep);
    fclose(upper);
    fclose(reverse);
}

// 替换字符串中的特定子串
char* replace(const char *str, const char *originStr, const char *replaceStr)
{
    if (str == NULL || originStr == NULL || replaceStr == NULL) {
        return NULL;
    }
    char *pos = strstr(str, originStr);
    if (pos == NULL) {
        // 如果找不到，返回原字符串的拷贝
        char *copy = malloc(strlen(str) + 1);
        strcpy(copy, str);
        return copy;
    }

    const size_t newLen = strlen(str) - strlen(originStr) + strlen(replaceStr);
    char *rep = malloc(newLen + 1);
    size_t index = pos - str;

    strncpy(rep, str, index);
    rep[index] = '\0';
    strcat(rep, replaceStr);
    strcat(rep, pos + strlen(originStr));

    return rep;
}

// 复制文件内容到另一个文件
int copyFile(FILE *in, FILE *out)
{
    char line[100] = "";
    while (fgets(line, sizeof(line), in))
    {
        fprintf(out, "%s", line);
    }
    return 0;
}

// 把文件中的小写字母转换为大写字母
int upperFile(FILE *in, FILE *upper)
{
    int ch;
    while ((ch = fgetc(in)) != EOF)
    {
        if (ch <= 'z' && ch >= 'a')
        {
            ch -= 32;
        }

        fputc(ch, upper);
    }
    return 0;
}

// 替换 "line" 为 "sentence"
int replaceFile(FILE *in, FILE *rep)
{
    char line[100] = "";
    while (fgets(line, sizeof(line), in))
    {
        if (strstr(line,"line") != NULL)
        {
            char *newLine = replace(line, "line", "sentence");
            fprintf(rep, "%s", newLine);
            free(newLine);
        } else
        {
            fprintf(rep, "%s", line);
        }
    }

    return 0;
}

// 按照行反转文件内容
int reverseFile(FILE *in, FILE *reverse)
{
    char line[100] = "";
    char **rev = NULL;
    char wholeLine[500] = "";
    size_t size = 0;
    int lineNum = 0;
    while (fgets(line, sizeof(line), in))
    {
        size_t len = strlen(line);
        strcat(wholeLine, line);
        if (len > 0 && line[len -1] == '\n')
        {
            lineNum += 1;
            rev = realloc(rev, (size + 1) * sizeof(char*));
            rev[size] = malloc(strlen(wholeLine) + 1);
            strcpy(rev[size], wholeLine);
            size += 1;

            // 两种清空字符串的方式
            wholeLine[0] = '\0';
            // strcpy(wholeLine, "");
        }
    }

    if (wholeLine[0] != '\0') {
        // 这就是最后一行，直接当一行 push 到 rev[]
        rev = realloc(rev, (size + 1) * sizeof(char*));
        rev[size] = malloc(strlen(wholeLine) + 1);
        strcpy(rev[size], wholeLine);
        size += 1;
        lineNum += 1;
    }

    printf("\nnum of lines is %d\n", lineNum);

    for (int i = size-1; i>=0; i--)
    {
        printf("%s", rev[i]);
        fprintf(reverse, "%s", rev[i]);
        free(rev[i]);
    }

    free(rev);
    return 0;
}