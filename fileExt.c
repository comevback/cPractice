//
// Created by 吨吨 on 2025/6/3.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* replace(const char *str, const char *origin, const char *replace);
int upperPrint(FILE *in, FILE *upper);

int main()
{
    FILE *in = fopen("sample_input.txt", "r");
    FILE *out = fopen("copy.txt", "w");
    FILE *rep = fopen("replace.txt" , "w");
    FILE *upper = fopen("uppercase.txt" , "w");

    if (in == NULL || out == NULL || rep == NULL || upper == NULL)
    {
        printf("can not open files");
        return 1;
    }

    upperPrint(in, upper);
    rewind(in);

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
            fprintf(out, "%s", wholeLine);

            if (strstr(wholeLine,"line") != NULL)
            {
                char *newLine = replace(wholeLine, "line", "sentence");
                fprintf(rep, "%s", newLine);
            } else
            {
                fprintf(rep, "%s", wholeLine);
            }

            rev = realloc(rev, (size + 1) * sizeof(char*));
            rev[size] = malloc(strlen(wholeLine) + 1);
            strcpy(rev[size], wholeLine);
            size += 1;

            // 两种清空字符串的方式
            wholeLine[0] = '\0';
            // strcpy(wholeLine, "");
        }
    }

    if (line[strlen(line) -1] != '\n')
    {
        size_t len = strlen(line);
        if (len + 1 < sizeof(wholeLine)) { // 预留 '\0'
            strcat(wholeLine, "\n");
        }
        fprintf(out, "%s", wholeLine);
        lineNum += 1;

        if (strstr(wholeLine,"line") != NULL)
        {
            char *newLine = replace(wholeLine, "line", "sentence");
            fprintf(rep, "%s", newLine);
        } else
        {
            fprintf(rep, "%s", wholeLine);
        }

        rev = realloc(rev, (size + 1) * sizeof(char*));
        rev[size] = malloc(strlen(wholeLine) + 1);
        strcpy(rev[size], wholeLine);
        size += 1;

        // 两种清空字符串的方式
        wholeLine[0] = '\0';
    }

    printf("\nnum of lines is %d\n", lineNum);

    for (int i = size-1; i>=0; i--)
    {
        printf("%s", rev[i]);
        free(rev[i]);
    }

    free(rev);
    fclose(in);
    fclose(out);
    fclose(rep);
    fclose(upper);
}

char* replace(const char *str, const char *origin, const char *replace)
{
    if (str == NULL || origin == NULL || replace == NULL) {
        return NULL;
    }
    char *pos = strstr(str, origin);
    if (pos == NULL) {
        // 如果找不到，返回原字符串的拷贝
        char *copy = malloc(strlen(str) + 1);
        strcpy(copy, str);
        return copy;
    }

    const size_t newLen = strlen(str) - strlen(origin) + strlen(replace);
    char *rep = malloc(newLen + 1);
    size_t index = pos - str;

    strncpy(rep, str, index);
    rep[index] = '\0';

    strcat(rep, replace);

    strcat(rep, pos + strlen(origin));

    return rep;
}

int upperPrint(FILE *in, FILE *upper)
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
}