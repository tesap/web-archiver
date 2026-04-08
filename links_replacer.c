#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>
#include <unistd.h>

#include "./util.h"
#include "./url_parser.h"

struct cmd_args {
    const char* file_path;
    bool in_place;
};

struct cmd_args cmd_args = {
    .file_path = "",
    .in_place = false,
};

void exit_args_error() {
    fprintf(stderr, "Usage: ./links_replacer -f <path> [-i]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-f --file <path>\tInput file path\n");
    fprintf(stderr, "\t-i --in-place\t\tPerform replacement in-place, do not output edited file\n");
    exit(1);
}

void parse_cmd_args(int argc, char* argv[], struct cmd_args* args) {
    bool f_flag = false;
    bool i_flag = false;
    bool b_flag = false;

    while (1) {
        static struct option long_options[] = {
            {"file",                 required_argument, 0, 'f'},
            {"in-place",             no_argument, 0, 'i'},
            {0, 0, 0, 0}
        };

        int longindex;
        int c = getopt_long(argc, argv, "f:i", long_options, &longindex);
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
            case 'i':
                if (i_flag) {
                    exit_args_error();
                }
                i_flag = true;
                args->in_place = true;
                break;
            case '?':
                exit_args_error();
            default:
                printf("==== default\n");
        }
    }

    // Assert required arguments
    if (!f_flag) {
        fprintf(stderr, ANSI_COLOR_RED "File argument is required\n" ANSI_COLOR_RESET);
        exit(1);
    }
}

// TODO can we make virtual file descriptor to test this func?
// void write_replaced(const char* buff, int size, ) {
//     char* el_ptr = (char*)data;
//     while ((el_ptr - data) < size) {
//
// }

// void printf_consume(char** ptr, int size) {
//     printf("%.*s", size, *ptr);
//     (*ptr) += size;
// }

void printf_consume(const char** ptr, const char* until) {
    assert(until > *ptr);
    int size = until - *ptr;
    printf("%.*s", size, *ptr);
    (*ptr) += size;
}

int main(int argc, char* argv[]) {
    parse_cmd_args(argc, argv, &cmd_args);

    char* buff = NULL;
    int size = read_file(cmd_args.file_path, &buff);
    assert(size > 0);
    assert(buff != NULL);

    const char* ptr = buff;
    int left = size;
    while(left > 0) {
        left = size - (ptr - buff);
        if (*ptr != '<') {
            printf_consume(&ptr, ptr + 1);
            continue;
        }

        struct HtmlTag t = parse_html_tag(ptr);
        if (t.link.ptr == NULL) {
            printf_consume(&ptr, ptr + 1);
            continue;
        }

        printf_consume(&ptr, t.link.ptr);

        // fprintf(stderr, ANSI_COLOR_GREEN "  F %s\n" ANSI_COLOR_RESET, cmd_args.file_path);
        // fprintf(stderr, ANSI_COLOR_YELLOW "  ? %.*s\n" ANSI_COLOR_RESET, t.link.size, t.link.ptr);

        char _file_path[MAX_URL_LENGTH];
        struct vec file_path = {_file_path, 0};
        if (is_url_relative(t.link.ptr)) {
            // printf_consume(&ptr, t.tag_end);
            // ptr = t.tag_end;

            int domain_len = first_dir_len(cmd_args.file_path);

            // TODO make join_paths last arg without *
            // NO! we need pass size back
            join_paths({(char*)cmd_args.file_path, domain_len}, t.link, &file_path);
        } else {
            LinkType lt = tag_link_type(&t);
            bool is_file = detect_is_file(t.link, lt);

            url_to_filepath(t.link, is_file, &file_path);
        }

        // fprintf(stderr, ANSI_COLOR_YELLOW "1-> %.*s\n" ANSI_COLOR_RESET, file_path.size, file_path.ptr);

        char _relpath[MAX_URL_LENGTH];
        struct vec relpath = {_relpath, 0};
        get_relpath(
            { (char*)cmd_args.file_path, dirname_len(cmd_args.file_path) },
            file_path,
            &relpath
        );

        printf("%.*s", relpath.size, relpath.ptr);
        // fprintf(stderr, ANSI_COLOR_GREEN "2-> %.*s\n" ANSI_COLOR_RESET, relpath.size, relpath.ptr);

        ptr = t.link.ptr + t.link.size;
    }
}
