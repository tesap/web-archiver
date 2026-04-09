#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>
#include <unistd.h>

#include "util.h"

struct cmd_args {
    const char* file_path;
    const char* path_hint;
    bool in_place;
};

struct cmd_args cmd_args = {
    .file_path = "",
    .path_hint = NULL,
    .in_place = false,
};

void exit_args_error() {
    fprintf(stderr, "Usage: ./links_replacer -f <path> [-i]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-f --file <path>\tInput file path\n");
    fprintf(stderr, "\t-h --path-hint <path>\tA hint of where a given file should be located in the root of archive\n");
    fprintf(stderr, "\t-i --in-place\t\tPerform replacement in-place, do not output edited file\n");
    exit(1);
}

void parse_cmd_args(int argc, char* argv[], struct cmd_args* args) {
    bool f_flag = false;
    bool h_flag = false;
    bool i_flag = false;

    while (1) {
        static struct option long_options[] = {
            {"file",                 required_argument, 0, 'f'},
            {"path-hint",            required_argument, 0, 'h'},
            {"in-place",             no_argument, 0, 'i'},
            {0, 0, 0, 0}
        };

        int longindex;
        int c = getopt_long(argc, argv, "f:h:i", long_options, &longindex);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
            case 'f':
                if (f_flag) {
                    exit_args_error();
                }
                f_flag = strlen(optarg) > 0;
                args->file_path = optarg;
                break;
            case 'h':
                if (h_flag) {
                    exit_args_error();
                }
                h_flag = strlen(optarg) > 0;
                args->path_hint = optarg;
                break;
            case 'i':
                if (i_flag) {
                    exit_args_error();
                }
                i_flag = true;
                args->in_place = true;
                break;
            case '?':
            default:
                exit_args_error();
        }
    }

    // Assert required arguments
    if (!f_flag) {
        fprintf(stderr, ANSI_COLOR_RED "File argument is required\n" ANSI_COLOR_RESET);
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    parse_cmd_args(argc, argv, &cmd_args);

    char* buff = NULL;
    int size = read_file(cmd_args.file_path, &buff);
    assert(size > 0);
    assert(buff != NULL);

    char* _path_hint = (cmd_args.path_hint) ? cmd_args.path_hint : cmd_args.file_path;
    struct vec path_hint = vec_wrap(_path_hint);

    replace_links(
        (struct vec){ buff, size },
        path_hint,
        stdout
    );
}

