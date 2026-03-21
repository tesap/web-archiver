#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "./util.h"

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
    fprintf(stderr, "\t-f <path>\tInput file path\n");
    fprintf(stderr, "\t-i\t\tPerform replacement in-place, do not output edited file\n");
    exit(1);
}

void parse_cmd_args(int argc, char* argv[], struct cmd_args* args) {
    bool f_flag = false;
    bool i_flag = false;

    while (1) {
        static struct option long_options[] = {
            {NULL,             required_argument, 0, 'f'},
            {NULL,             required_argument, 0, 'i'},
            {0, 0, 0, 0}
        };

        int longindex;
        int c = getopt_long(argc, argv, "f:i:", long_options, &longindex);
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
                i_flag = strlen(optarg) > 0;
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

int main(int argc, char* argv[]) {
    parse_cmd_args(argc, argv, &cmd_args);

    char* buff = NULL;
    int size = read_file(cmd_args.file_path, &buff);
    assert(size > 0);
    assert(buff != NULL);

    const char* ptr = buff;
    while(left > 0) {
    // int left = size;
    // while (left > 0 && (ptr = (const char*)memchr(ptr, '<', left))) {
        left = size - (ptr - buff);
        if (*ptr == '<') {
            struct HtmlTag t = parse_html_tag(ptr);
            if (t.link_start == NULL) {
                ptr++;
            }
            else if (is_relative_url(t.link_start)) {
                ptr = t.tag_end;
            }
            else {
                printf("%.*s", t.link_start - ptr, ptr);
                struct UrlPtrs ptrs = get_url_pointers(t.link_start, t.link_size);
                if (ptrs.path_start) {
                    printf("%s", ptrs.path_start);
                    printf("%.*s", t.tag_end - t.link_start - t.link_size, t.link_start + t.link_size);
                    ptr = t.tag_end;
                } else {
                    printf("/");
                    // TODO
                }
            }
        }
        
        ptr++;
    }
}
