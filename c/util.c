#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"

struct vec* vec_init(size_t newsize) {
    struct vec* v = (struct vec*)malloc(sizeof(struct vec));
    v->ptr = (char*)malloc(sizeof(char) * newsize);
    v->size = newsize;
    return v;
}

void vec_deinit(struct vec* v) {
    free(v->ptr);
    free(v);
}

void vec_append(struct vec* v, const char* recv_buff, size_t newsize) {
    if (newsize <= 0) {
        fprintf(stderr, "Error vec_append: newsize = %d\n", newsize);
        exit(1);
    }
    if (!v || !v->ptr) {
        fprintf(stderr, "v->ptr is NULL\n");
        exit(1);
    }

    void* ptr = realloc(v->ptr, v->size + newsize);
    if(!ptr) {
        /* out of memory */
        fprintf(stderr, "Not enough memory (realloc returned NULL): %s\n", strerror(errno));
        exit(1);
        return;
    }
    v->ptr = (char *)ptr;

    memcpy((v->ptr + v->size), recv_buff, newsize);
    v->size += newsize;
}

bool is_number(char* s) {
    char* i = s;
    while (*i != '\0') {
        if (!isdigit((unsigned char)*i)) {
            return false;
        }
        i++;
    }
    return true;
}

bool ends_with(char* str, char* suffix) {
    // If the suffix is longer than the string, it cannot be a suffix.
    int len_suffix = strlen(suffix);
    int len_str = strlen(str);
    int delta = len_str - len_suffix;

    if (delta < 0) {
        return 0;
    }

    return (strcmp(str + delta, suffix) == 0);
}

