#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

#include "./util.h"
#include "./cached_network.h"

struct cmd_args {
    const char* url;
    int request_timeout;
    bool is_save;
    int cache_ttl;
    HrefType type_hint;
};

struct cmd_args cmd_args = {
    .url = "",
    .request_timeout = DEFAULT_REQUEST_TIMEOUT,
    .is_save = false,
    .cache_ttl = 0,
};

void exit_args_error() {
    fprintf(stderr, "Usage: ./cached_curl -a <addr> [-p <period>] [-t <timeout]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-a, --address <addr>\tURL to download\n");
    fprintf(stderr, "\t-t, --timeout <val>\tTimeout for TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_TIMEOUT);
    fprintf(stderr, "\t--save\t\t\tSave downloaded pages to fs cache\n");
    fprintf(stderr, "\t--cache-ttl <ttl>\tUse saved pages from fs to cache repeated requests, according to given TTL (Time to Live) of saved entries. 0 means each saved page is considered invalidated. TTL is in minutes (default: 0)\n");
    fprintf(stderr, "\t--type-hint <TYPE>\tSpecify type of the downloaded page. Possible values: \"HTML\", \"STYLE\", \"SCRIPT\", \"IMAGE\", \"NO\" (Default: \"NO\")");
    exit(1);
}

void parse_cmd_args(int argc, char* argv[], struct cmd_args* args) {
    bool a_flag = false;
    bool t_flag = false;
    bool type_hint_flag = false;

    while (1) {
        static struct option long_options[] = {
            {"address",             required_argument, 0, 'a'},
            {"request-timeout",     required_argument, 0, 't'},
            {"save",                no_argument, 0, 0},
            {"cache-ttl",           required_argument, 0, 0},
            {"type-hint",           required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int longindex;
        int c = getopt_long(argc, argv, "a:t:", long_options, &longindex);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                if (strcmp(long_options[longindex].name, "save") == 0) {
                    args->is_save = true;
                }
                else if (strcmp(long_options[longindex].name, "cache-ttl") == 0) {
                    if (!is_number(optarg)) {
                        fprintf(stderr, ANSI_COLOR_RED "Error: argument is not a number: %s\n" ANSI_COLOR_RESET, optarg);
                        exit(1);
                    }
                    args->cache_ttl = atoi(optarg);
                }
                else if (strcmp(long_options[longindex].name, "type-hint") == 0) {
                    type_hint_flag = true;
                    if (strcmp(optarg, "HTML") == 0) {
                        cmd_args.type_hint = HREF_TYPE_HTML;
                    }
                    else if (strcmp(optarg, "STYLE") == 0) {
                        cmd_args.type_hint = HREF_TYPE_STYLE;
                    }
                    else if (strcmp(optarg, "SCRIPT") == 0) {
                        cmd_args.type_hint = HREF_TYPE_SCRIPT;
                    }
                    else if (strcmp(optarg, "IMAGE") == 0) {
                        cmd_args.type_hint = HREF_TYPE_IMG;
                    }
                    else if (strcmp(optarg, "NO") == 0) {
                        cmd_args.type_hint = HREF_TYPE_UNKNOWN;
                    }
                    else {
                        fprintf(stderr, ANSI_COLOR_RED "Error: unknown option value: %s\n" ANSI_COLOR_RESET, optarg);
                        exit(1);
                    }
                }
                break;
            case 'a':
                if (a_flag) {
                    exit_args_error();
                }
                a_flag = strlen(optarg) > 0;
                args->url = optarg;
                break;
            case 't':
                if (t_flag) {
                    exit_args_error();
                }
                if (!is_number(optarg)) {
                    fprintf(stderr, ANSI_COLOR_RED "Error: argument is not a number: %s\n" ANSI_COLOR_RESET, optarg);
                    exit_args_error();
                }
                t_flag = true;
                args->request_timeout = atoi(optarg);
                break;
            case '?':
                exit_args_error();
            default:
                printf("==== default\n");
        }
    }

    // Assert required arguments
    if (!a_flag) {
        fprintf(stderr, ANSI_COLOR_RED "Address argument is required\n" ANSI_COLOR_RESET);
        exit(1);
    }

    if (!type_hint_flag) {
        fprintf(stderr, ANSI_COLOR_RED "type-hint argument is required\n" ANSI_COLOR_RESET);
        exit(1);
    }
}


int main(int argc, char* argv[]) {
    parse_cmd_args(argc, argv, &cmd_args);

    struct HttpPage downloaded_page;
    int res = cached_download_http(
        cmd_args.url,
        cmd_args.request_timeout,
        cmd_args.is_save,
        cmd_args.cache_ttl,
        cmd_args.type_hint,
        &downloaded_page
    );

    return res;
}
