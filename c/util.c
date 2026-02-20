#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

#define BUFFER_SIZE 64

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

bool is_number(const char* s) {
    const char* i = s;
    while (*i != '\0') {
        if (!isdigit((unsigned char)*i)) {
            return false;
        }
        i++;
    }
    return true;
}

bool ends_with(const char* str, const char* suffix) {
    // If the suffix is longer than the string, it cannot be a suffix.
    int len_suffix = strlen(suffix);
    int len_str = strlen(str);
    int delta = len_str - len_suffix;

    if (delta < 0) {
        return 0;
    }

    return (strcmp(str + delta, suffix) == 0);
}

size_t get_file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
	return -1;
    }

    return st.st_size;
}

int read_file(const char* path, char** out) {
    /*
     * @out must be an uninitialized pointer;
     */
    int fd = open(path, O_RDONLY);
    if (fd == -1) {  
        perror("open");
        return -1;
    }

    size_t size = get_file_size(path);
    if (size == -1) {
	    fprintf(stderr, "=== Could not get file size: %s\n", path);
        return -1;
    }
    
    *out = (char *)malloc(size + 1);
    int offset = 0;
    int bytes_read;
    while ((bytes_read = read(fd, *out + offset, BUFFER_SIZE)) != 0) {
        offset += bytes_read;
        // printf("--- Read bytes: %d\n", bytes_read);
        // printf("--- string: %d %d %d %d %d\n", (*out)[0], (*out)[1], (*out)[2], (*out)[3], (*out)[4]);
        // printf("--- size: %d\n", size);
    }

    (*out)[size] = '\0';

    close(fd);
    return 0;
}

int write_file(const char* path, const char* buff, const size_t buff_size) {
    int fd = open(path, O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {  
        perror("open");
        return -1;
    }

    size_t written = write(fd, buff, buff_size);
    if (written != buff_size) {
        perror("write");
        return -1;
    }
    return 0;
}

void debug_string(const char *str, const char *name) {
    printf("%s: '", name);
    for (size_t i = 0; str[i]; i++) {
        if (str[i] >= 32 && str[i] <= 126) {
            printf("%c", str[i]);
        } else {
            printf("\\x%02X", (unsigned char)str[i]);
        }
    }
    printf("' (length: %zu)\n", strlen(str));
}

size_t strlen_with_delims(const char *s) {
    const char* i = s;
    while (*i != '\0' && *i != '\n' && *i != '\r' && *i != ' ') {
        i++;
    }
    return i - s;
}


const char * strstr_with_delims(const char *s1, const char *s2) {
    const size_t len = strlen(s2);

    if (!len) {
        return s1;
    }

    for (const char* i = s1; *i != '\0' && *i != '\n' && *i != '\r' && *i != ' '; i++) {
        if (strncmp(i, s2, len) == 0) {
            return (char *)i;
        }
    }
    return NULL;
}

const char * strchr_with_delims(const char *s1, const char ch) {
    for (const char* i = s1; *i != '\0' && *i != '\n' && *i != '\r' && *i != ' '; i++) {
        // printf("=== strchr: %c\n", *i);
        if (*i == ch) {
            return (const char *)i;
        }
    }
    return NULL;
}

