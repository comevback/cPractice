//
// Created by 吨吨 on 2025/6/4.
//
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
static struct option long_options[];

int main(const int argc, char *argv[])
{
    int option_index = 0;
    int verbose_level = 0;
    opterr = 0;
    const char *shortOpts = "ho:v::";
    int ch;
    while ((ch = getopt_long(argc, argv, shortOpts, long_options, &option_index)) != -1)
    {
        switch (ch)
        {
        case 'h':
            printf("no document.\n");
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            if (optarg)
            {
                verbose_level = atoi(optarg);
                printf("verbose level is %d\n", verbose_level);
            }else
            {
                printf("lack of verbose_level\n");
                printf("verbose level is %d\n", 1);
            }
            break;
        case 'o':
            if (optarg)
            {
                printf("output file is %s\n", optarg);
            }else
            {
                printf("lack of output file\n");
                exit(EXIT_FAILURE);
            }
            break;
        case '?':
            if (optopt == 'o' || optopt == 'n')
            {
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            } else
            {
                fprintf(stderr, "Unknown option: %s\n", argv[optind - 1]);
            }
        default:
            // 不应该到达这里
            fprintf(stderr, "Unexpected option: %c\n", ch);
            exit(EXIT_FAILURE);
        }
    }
}

static struct option long_options[] =
{
    {"help", 0, NULL, 'h'},
    {"output", 1, NULL, 'o'},
    {"verbose", optional_argument, NULL, 'v'},
    {0,0,0,0}
};