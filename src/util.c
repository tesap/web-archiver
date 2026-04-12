#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

#include "util.h"

#define BUFFER_SIZE 64

struct vec vec_wrap(const char* s) {
    return (struct vec){ (char*)s, strlen(s) };
}

struct vec vec_alloc(size_t size) {
    return (struct vec){ (char*)malloc(sizeof(char) * size), size };
}

struct vec vec_init() {
    return (struct vec) { NULL, 0 };
}

struct vec vec_tails(const char* left, const char* right) {
    return (struct vec){left, right - left};
}

bool vec_eq(struct vec v, const char* cstr) {
    return (strncmp(v.ptr, cstr, v.size) == 0);
}

bool vec_eq2(struct vec v1, struct vec v2) {
    return (v1.size == v2.size) && (strncmp(v1.ptr, v2.ptr, v1.size) == 0);
}

struct UrlParts UrlParts_init() {
    return (struct UrlParts) {
        vec_init(),
        vec_init(),
        vec_init()
    };
}

struct HttpPage HttpPage_init(char* stack_buff) {
    struct HttpPage s = {
        vec_init(),
        vec_init(),
        vec_init()
    };
    s.effective_url.ptr = stack_buff;
    return s;
}

void HttpPage_dealloc(struct HttpPage p) {
    vec_dealloc(p.headers_vec);
    vec_dealloc(p.content_vec);
}

void vec_dealloc(struct vec v) {
    if (v.ptr) {
        free(v.ptr);
    }
}

void vec_terminate(struct vec* v) {
    v->ptr[v->size] = '\0';
}

void vec_to_cstr(struct vec v, char* out) {
    memcpy(out, v.ptr, v.size);
    out[v.size] = '\0';
}

void vec_append(struct vec* v, bool is_dynamic, struct vec add) {
    if (add.size <= 0) {
        return;
    }
    if (!v || !v->ptr) {
        fprintf(stderr, "v->ptr is NULL\n");
        exit(1);
    }

    if (is_dynamic) {
        void* ptr = realloc(v->ptr, v->size + add.size);
        if(!ptr) {
            /* out of memory */
            fprintf(stderr, "Not enough memory (realloc returned NULL): %s\n", strerror(errno));
            exit(1);
            return;
        }
        v->ptr = (char *)ptr;
    }

    memcpy((v->ptr + v->size), add.ptr, add.size);
    v->size += add.size;
}

void vec_append_cstring(struct vec* v, bool is_dynamic, const char* recv_buff) {
    vec_append(v, is_dynamic, vec_wrap(recv_buff));
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

bool is_alphabet(char c) {
    return strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", c) != NULL;
}

bool starts_with(struct vec s, struct vec suffix) {
    return (strncmp(s.ptr, suffix.ptr, suffix.size) == 0);
}
bool ends_with(struct vec s, struct vec suffix) {
    // If the suffix is longer than the string, it cannot be a suffix.
    int delta = s.size - suffix.size;

    if (delta < 0) {
        return 0;
    }

    return (strncmp(s.ptr + delta, suffix.ptr, suffix.size) == 0);
}

size_t file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }

    return st.st_size;
}

time_t file_mtime(struct vec path) {
    vec_terminate(&path);

    struct stat st;
    if (stat(path.ptr, &st) != 0) {
        return -1;
    }

    return st.st_mtime;
}

// LCOV_EXCL_START
void debug_string(const char *ptr, int size, const char *name) {
    printf("%s: '", name);
    for (int i = 0; i < size; i++) {
        if (ptr[i] >= 32 && ptr[i] <= 126) {
            printf("%c", ptr[i]);
        } else {
            printf("\\x%02X", (unsigned char)ptr[i]);
        }
    }
    printf("' (length: %zu)\n", strlen(ptr));
}
// LCOV_EXCL_STOP

size_t strlen_with_delims(const char *s) {
    const char* i = s;
    while (*i != '\0' && *i != '\n' && *i != '\r' && *i != ' ') {
        i++;
    }
    return i - s;
}

bool file_exists(struct vec path) {
    vec_terminate(&path);
    return access(path.ptr, F_OK) == 0;
}

int read_file(const char* path, char** out) {
    /*
     * @out must be an uninitialized pointer;
     */
    int fd = open(path, O_RDONLY);
    if (fd == -1) {  
        fprintf(stderr, ANSI_COLOR_RED "=== Error reading file: %s\n" ANSI_COLOR_RESET, path);
        perror("open");
        return -1;
    }

    size_t size = file_size(path);
    if (size == -1) {
        fprintf(stderr, ANSI_COLOR_RED "=== Could not get file size: %s\n" ANSI_COLOR_RESET, path);
        return -1;
    }
    
    *out = (char *)malloc(size + 1);
    int offset = 0;
    int bytes_read;
    while ((bytes_read = read(fd, *out + offset, BUFFER_SIZE)) > 0) {
        offset += bytes_read;
    }
    if (bytes_read < 0) {
        fprintf(stderr, ANSI_COLOR_RED "=== Error reading file: %s\n" ANSI_COLOR_RESET, path);
        perror("read");
        return -1;
    }

    (*out)[size] = '\0';

    close(fd);
    return offset;
}

int write_file(const char* path, const char* buff, const size_t buff_size) {
    int fd = open(path, O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {  
        fprintf(stderr, ANSI_COLOR_RED "=== Error writing file: %s\n" ANSI_COLOR_RESET, path);
        perror("open");
        return -1;
    }

    size_t written = write(fd, buff, buff_size);
    if (written != buff_size) {
        fprintf(stderr, ANSI_COLOR_RED "=== Error writing file: %s\n" ANSI_COLOR_RESET, path);
        perror("write");
        return -1;
    }
    return written;
}

bool mkdir_p(const char* dir) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return false;
    }

    if (pid == 0) {
        // Child process
        execlp("mkdir", "mkdir", "-p", dir, NULL);
        perror("exec failed");
        exit(1);
    } else {
        // Parent process
        wait(NULL);
        return true;
    }
}

void rstrip(struct vec* s, char c) {
    if (s->ptr[s->size - 1] == c) {
        s->size--;
    }
}

void lstrip(struct vec* s, struct vec prefix) {
    if (strncmp(s->ptr, prefix.ptr, prefix.size) == 0) {
        s->ptr += prefix.size;
        s->size -= prefix.size;
    }
}

// -- url_parser.c
bool is_abs_path(const char* s) {
    /*
     * Tells whether a given path is absolute
     */
    return s[0] == '/';
}

bool is_url_http(const char* url_start) {
    /*
     * Tells whether URL is http/https.
     */
    return strncmp(url_start, "http", 4) == 0;
}

struct UrlPtrs url_pointers(struct vec url) {
    struct UrlPtrs res = { NULL, NULL, NULL, NULL, NULL, NULL };

    assert(url.ptr != NULL);
    assert(url.size > 0);

    bool host_found = false;

    const char* i = url.ptr;
    for (i = url.ptr; i < url.ptr + url.size && (strchr("$?#\n\r ", *i) == NULL); i++) {
        if (!host_found && strncmp(i, "://", 3) == 0) {
            res.protocol_start = url.ptr;
            i += 3;
            res.protocol_end = i;
            res.host_start = i;
            host_found = true;
        } else if (*i == ':') {
            res.host_end = i;
        // Have not encountered '/' before
        } else if (*i == '/' && !res.path_start) {
            if (!res.host_start && i > url.ptr) {
                res.host_start = url.ptr;
            }
            if (!res.host_end && res.host_start) {
                res.host_end = i;
            }
            res.path_start = i;
        }
    }
    if (res.host_start && !res.host_end) {
        res.host_end = i;
    } else if (res.path_start) {
        res.path_end = i;
    }

    if (!res.protocol_start && !res.path_start) {
        res.host_start = url.ptr;
        res.host_end = i;
    }

    return res;
}


void url_parts(struct vec url, struct UrlParts* parts) {
    struct UrlPtrs ptrs = url_pointers(url);

    if (ptrs.protocol_start && ptrs.protocol_end) {
        int len = ptrs.protocol_end - ptrs.protocol_start;
        parts->protocol = (struct vec){ptrs.protocol_start, len};
    }

    if (ptrs.host_start && ptrs.host_end) {
        int len = ptrs.host_end - ptrs.host_start;
        parts->host = (struct vec){ptrs.host_start, len};
    }

    if (ptrs.path_start != NULL && ptrs.path_end != NULL) {
        int len = ptrs.path_end - ptrs.path_start;
        parts->path = (struct vec){ptrs.path_start, len};
    }
}

bool is_url_represents_file(struct vec url, LinkType type_hint) {
    if (ends_with(url, vec_wrap("/"))) {
        return false;
    }
    
    bool is_file = false;
    switch (type_hint) {
        case LINK_TYPE_HTML:
        case LINK_TYPE_UNKNOWN: 
            is_file = false;
            break;
        case LINK_TYPE_IMG:
        case LINK_TYPE_STYLE:
        case LINK_TYPE_SCRIPT:
            is_file = true;
            break;
    }

    // Find last '/' occurence
    struct UrlPtrs ptrs = url_pointers(url);
    int i = url.size;
    while (url.ptr + i > ptrs.path_start) {
        if (url.ptr[i] == '.' && is_alphabet(url.ptr[i + 1])) {
            is_file = true;
        }
        else if (url.ptr[i] == '/') {
            break;
        }
        i--;
    }

    return is_file;
}

void only_host_path(struct vec url, struct vec* out) {
    /*
     * 
     * For example:
     *
     * INPUT:
     *     - url = "https://www.archlinux.org/path/to/smth/?a=123"
     * OUTPUT:
     *     - out_path = "www.archlinux.org/path/to/smth"
     */
    struct UrlPtrs ptrs = url_pointers(url);

    char* substr_start = ptrs.host_start ? ptrs.host_start : ptrs.path_start;
    char* substr_end = ptrs.path_end ? ptrs.path_end : ptrs.host_end;
    vec_append(out, false, vec_tails(substr_start, substr_end));
    rstrip(out, '/');
}

int dir_count(struct vec path) {
        int i = 1;
        int cnt = 0;
        assert(path.ptr[0] != '/');
        while (i < path.size) {
            if (strncmp(path.ptr + i - 2, "/./", 3) == 0) {
            }
            else if (i == 1 && strncmp(path.ptr, "./", 2) == 0) {
            }
            else if (strncmp(path.ptr + i - 2, "../", 3) == 0) {
                cnt--;
            }
            else if (path.ptr[i] == '/') {
                cnt++;
            }
            i++;
        }
        if (path.ptr[path.size - 1] != '/') {
            cnt++;
        }
        return cnt;
}

void relpath(
    struct vec path_from,
    struct vec path_to,
    struct vec* result
) {
    /*
     * Calculate relative path from @path_from to @path_to.
     * Both @path_from and @path_to are treated as directories.
     *
     * For example:
     * IN:
     *     path_from = dir1/dir2/one
     *     path_to   = dir1/dir2/second
     * OUT:
     *     res = ../second
     */

    int i = 0;
    int last_saved_slash = 0;

    char from_path[path_from.size];
    char to_path[path_to.size];

    // n1 = Count number of directories in @from
    int dirs;
    if (path_from.size == 0) {
        dirs = 0;
    } else {
        dirs = dir_count(path_from);
    }
    
    // res = '../' * n1 + @to
    for(int i = 0; i < dirs; i++) {
        vec_append(result, false, vec_wrap("../"));
    }
    vec_append(result, false, path_to);
}


int dirname_len(struct vec path) {
    /*
     * Given @path (c-string) calculates directory name prefix length
     */
    if (path.ptr[path.size - 1] == '/') {
        return path.size;
    } else {
        // Find last '/' occurence
        int i = path.size;
        while (i > 0) {
            if (path.ptr[i] == '/') {
                return i + 1;
            }
            i--;
        }

    }
    return 0;
}

int url_dirname_len(struct vec url) {
    struct UrlParts parts = UrlParts_init();
    url_parts(url, &parts);
    if (parts.path.size > 0) {
        return dirname_len(url);
    } else {
        return url.size;
    }
}

int first_dir_len(struct vec path) {
    const char* init = path.ptr;
    assert(strncmp(path.ptr, "../", 3) != 0);

    while (strncmp(path.ptr, "./", 2) == 0) {
        path.ptr += 2;
    }
    const char* slash_occ = strchr(path.ptr, '/');
    assert(slash_occ != NULL);
    return slash_occ - init;
}

// -- network.c

// LCOV_EXCL_START
void print_sockaddr(struct sockaddr* sa) {
    char ip_string[INET6_ADDRSTRLEN];
    void *addr_ptr;
    in_port_t port;

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
        addr_ptr = &(ipv4->sin_addr);
        port = ntohs(ipv4->sin_port);

        if (inet_ntop(AF_INET, addr_ptr, ip_string, INET6_ADDRSTRLEN) == NULL) {
            perror("inet_ntop");
            return;
        }
        printf("IPv4 Address: %s, Port: %d\n", ip_string, port);

    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
        addr_ptr = &(ipv6->sin6_addr);
        port = ntohs(ipv6->sin6_port);

        if (inet_ntop(AF_INET6, addr_ptr, ip_string, INET6_ADDRSTRLEN) == NULL) {
            perror("inet_ntop");
            return;
        }
        printf("IPv6 Address: %s, Port: %d\n", ip_string, port);

    } else {
        printf("Unknown address family: %d\n", sa->sa_family);
    }
}
// LCOV_EXCL_STOP

int create_tcp_socket(struct vec hostname, const char* service) {
    struct addrinfo hints, *addrinfo_result;
    int status;
    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    char _hostname[256];
    vec_to_cstr(hostname, _hostname);
    int rv = getaddrinfo(_hostname, service, &hints, &addrinfo_result);
    if (rv != 0) {
        fprintf(stderr, "=== getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    int sockfd = -1;
    for (struct addrinfo* p = addrinfo_result; p != NULL; p = p->ai_next) {
        // print_sockaddr(p->ai_addr);

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
			perror("socket");
			continue;
		}

        int ok = connect(sockfd, p->ai_addr, p->ai_addrlen);
		if (ok == -1) {
			close(sockfd);
            sockfd = -1;
			perror("connect");
			continue;
		}
		break;
    }
    freeaddrinfo(addrinfo_result);

    return sockfd;
}

int download_http(struct vec url, int timeout_sec, struct HttpPage* out) {
    /*
     * The function inits @out with heap-located attributes, a caller should do HttpPage_dealloc()
     */
    struct UrlParts up = UrlParts_init();
    url_parts(url, &up);

    assert(up.host.ptr != NULL);
    assert(up.host.size > 0);

    char request[1024];

    if (up.path.ptr == NULL) {
        up.path = vec_wrap("/");
    }

    // Mimic to cURL
    const char* request_template = ""
        "GET %.*s HTTP/1.1\r\n"
        "Host: %.*s\r\n"
        "User-Agent: curl/8.18.0\r\n"
        "Connection: close\r\n\r\n";
    snprintf(request, sizeof(request), request_template, up.path.size, up.path.ptr, up.host.size, up.host.ptr);
    // printf("REQUEST: %s\n", request);
    // fflush(stdout);

    int sockfd;
    char recv_buff[BUFFER_SIZE];
    int bytes_received;
    bool content_started = false;
    struct vec headers_vec = vec_alloc(0);
    struct vec content_vec = vec_alloc(0);

    if (vec_eq(up.protocol, "http://") || up.protocol.size == 0) {
        sockfd = create_tcp_socket(up.host, "80");

        // --- Send request ---
        int bytes_sent = send(sockfd, request, strlen(request), 0);

        // --- Receive response ---
        while ((bytes_received = recv(sockfd, recv_buff, BUFFER_SIZE - 1, 0)) > 0) {
            parse_http_stream_chunk(recv_buff, bytes_received, &headers_vec, &content_vec, &content_started);
        }
    } else if (vec_eq(up.protocol, "https://")) {
        sockfd = create_tcp_socket(up.host, "443");

        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            fprintf(stderr, "SSL_CTX_new() failed.\n");
            return -1;
        }
        SSL* ssl = SSL_new(ctx);
        if (!ssl)
        {
            fprintf(stderr, "SSL_new() failed.\n");
            close(sockfd);
            SSL_CTX_free(ctx);
            return -1;
        }

        SSL_set_fd(ssl, sockfd);

        // IMPORTANT: Set SNI hostname
        char _host[256];
        vec_to_cstr(up.host, _host);
        SSL_set_tlsext_host_name(ssl, _host);

        if (SSL_connect(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(sockfd);
            SSL_CTX_free(ctx);
            return -1;
        }

        // --- Send request ---
        SSL_write(ssl, request, strlen(request));

        // --- Receive response ---
        struct timeval tv;
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        while ((bytes_received = SSL_read(ssl, recv_buff, BUFFER_SIZE - 1)) > 0) {
            parse_http_stream_chunk(recv_buff, bytes_received, &headers_vec, &content_vec, &content_started);
        }
    } else {
        fprintf(stderr, "=== Protocol not supported: %.*s\n", url.size, url.ptr);
        return -1;
    }

    if (bytes_received < 0) {
        fprintf(stderr, "=== Error getting response: %.*s\n", url.size, url.ptr);
        return -1;
    }

    // --- Parsing HTTP response ---
    int status_code = 0;
    sscanf(headers_vec.ptr, "HTTP/%*d.%*d %d", &status_code);
    if (status_code == 0) {
        fprintf(stderr, "=== Error parsing status_code: %.*s...\n", 20, headers_vec.ptr);
        return -1;
    } else if (status_code == 301 || status_code == 302) {
        char _mem1[256];
        struct vec redirect_url = {_mem1, 0};
        if (get_location_header(headers_vec, url, &redirect_url) != 0) {
            return -1;
        }
        // printf("--- FOUND REDIRECT Location: %s\n", redirect_url);
        return download_http(redirect_url, timeout_sec, out);
    } else if (status_code == 200) {
        if (out) {
            out->headers_vec = headers_vec;
            out->content_vec = content_vec;
            // We should not assign directly, as it is a struct with stack-pointing data
            // out->effective_url = url;
            vec_append(&out->effective_url, false, url);
        }
        return 0;
    } else {
        fprintf(stderr, "> %.*s\n", headers_vec.size, headers_vec.ptr);
        fprintf(stderr, "=== Bad status_code: %d\n", status_code);
        write_file("err.http", headers_vec.ptr, headers_vec.size);
        return -1;
    }
}

// -- http_parser.c

void parse_http_stream_chunk(const char* recv_buff, int size, struct vec* headers_vec, struct vec* content_vec, bool* content_started) {
    char border_window[6];
    memcpy(border_window, headers_vec->ptr + headers_vec->size - 3, 3);
    memcpy(border_window + 3, recv_buff, 3);
    int i = 0;
    bool found = false;
    while (i < 6) {
        if (strncmp(border_window + i, "\r\n\r\n", 4) == 0) {
            found = true;
            break;
        }
        i++;
    }

    if (found) {
        headers_vec->size -= 3 - i;

        int content_offset_in_buff = i + 4 - 3;
        *content_started = true;
        vec_append(
            content_vec,
            true,
            (struct vec){ (char*)recv_buff + content_offset_in_buff, size - content_offset_in_buff }
        );
        return;
    }
    
    char* pattern_ptr = (char*)memmem(recv_buff, size, "\r\n\r\n", 4);
    if (pattern_ptr != NULL) {
        size_t header_size_in_buff = pattern_ptr - recv_buff;
        vec_append(
            headers_vec,
            true,
            (struct vec){ (char*)recv_buff, header_size_in_buff }
        );

        size_t content_offset_in_buff = header_size_in_buff + 4;
        *content_started = true;
        vec_append(
            content_vec,
            true,
            (struct vec){(char*)recv_buff + content_offset_in_buff, size - content_offset_in_buff}
        );
        return;
    }

    if (*content_started) {
        vec_append(content_vec, true, (struct vec){ (char*)recv_buff, size });
    } else {
        vec_append(headers_vec, true, (struct vec){ (char*)recv_buff, size });
    }
}

int get_location_header(struct vec headers_data, struct vec request_url, struct vec* out) {
    const char* header_start = strstr(headers_data.ptr, "Location: ");
    if (!header_start) {
        header_start = strstr(headers_data.ptr, "location: ");
        if (!header_start) {
            fprintf(stderr, "=== Error searching for 'Location: ' pattern\n");
            return -1;
        }
    }
    const char* value_start = header_start + 10;
    size_t value_len = strlen_with_delims(value_start);
    if (value_start + value_len > headers_data.ptr + headers_data.size) {
        value_len = headers_data.ptr + headers_data.size - value_start;
    }

    link_to_full_url((struct vec){value_start, value_len}, request_url, out);
    return 0;
}

// --- html_parser.c

// LCOV_EXCL_START
static void print_link_type(LinkType ht) {
    switch (ht) {
        case LINK_TYPE_UNKNOWN: printf("LINK_TYPE_UNKNOWN"); break;
        case LINK_TYPE_IMG: printf("LINK_TYPE_IMG"); break;
        case LINK_TYPE_STYLE: printf("LINK_TYPE_STYLE"); break;
        case LINK_TYPE_SCRIPT: printf("LINK_TYPE_SCRIPT"); break;
        case LINK_TYPE_HTML: printf("LINK_TYPE_HTML"); break;
    }
}
// LCOV_EXCL_STOP

LinkType tag_link_type(struct HtmlTag* t) {
    static int i = 0;
    // fprintf(stderr, "(%d, \"%s\", \"%.*s\", \"%.*s\", LINK_TYPE_UNKNOWN);\n", i, t->tag_name, t->link.size, t->link.ptr, t->type.size, t->type.ptr);
    i++;
    // { "link", "/static/archlinux_common_style/navbar.css", "text/css" },
    if (strncmp(t->tag_name, "a", 1) == 0) {
        return LINK_TYPE_HTML;
    }
    if (strncmp(t->tag_name, "img", 3) == 0) {
        return LINK_TYPE_IMG;
    }
    if (strncmp(t->tag_name, "script", 6) == 0) {
        return LINK_TYPE_SCRIPT;
    }

    if (t->type.ptr) {
        if (strncmp(t->type.ptr, "text/css", 8) == 0) {
            return LINK_TYPE_STYLE;
        }
        if (strncmp(t->type.ptr, "image/", 6) == 0) {
            return LINK_TYPE_IMG;
        }
        if (strncmp(t->type.ptr, "text/javascript", 15) == 0) {
            return LINK_TYPE_SCRIPT;
        }
    }

    assert(t->link.ptr != NULL);

    struct UrlPtrs ptrs = url_pointers(t->link);
    assert(ptrs.host_end || ptrs.path_end);

    char* main_part_end = (ptrs.path_end) ? ptrs.path_end : ptrs.host_end;
    if (strncmp(main_part_end - 4, ".png", 4) == 0) {
        return LINK_TYPE_IMG;
    }
    if (strncmp(main_part_end - 4, ".jpg", 4) == 0) {
        return LINK_TYPE_IMG;
    }
    if (strncmp(main_part_end - 4, ".svg", 4) == 0) {
        return LINK_TYPE_IMG;
    }
    if (strncmp(main_part_end - 4, ".css", 4) == 0) {
        return LINK_TYPE_STYLE;
    }
    if (strncmp(main_part_end - 3, ".js", 3) == 0) {
        return LINK_TYPE_SCRIPT;
    }

    return LINK_TYPE_UNKNOWN;
}

enum CaptureAttr {
    CAPTURE_LINK,
    CAPTURE_TYPE,
    CAPTURE_NO,
};

struct HtmlTag parse_html_tag(const char* buff) {
    /*
     * The function effectively searches for a,link,img,script tags in @buff.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a .* type="<CAPTURE_1>" .* href="<CAPTURE_2>">.*</a>"'
     * 
     * A caller does not pass ownership of @page_url, he should handle it on its own.
     *
     * @buff is not expected to have '>' occurence
     */

    struct HtmlTag out = {
        { '\0' },
        NULL,
        (struct vec){NULL, 0},
        (struct vec){NULL, 0},
    };

    const char* ptr = buff;
    ptr++; // Skip '<'

    char* link_attr_prefix;
    if (strncmp(ptr, "a", 1) == 0) {
        strcpy(out.tag_name, "a");
        link_attr_prefix = "href=\"";
    }
    else if (strncmp(ptr, "link", 4) == 0) {
        strcpy(out.tag_name, "link");
        link_attr_prefix = "href=\"";
    }
    else if (strncmp(ptr, "img", 3) == 0) {
        strcpy(out.tag_name, "img");
        link_attr_prefix = "src=\"";
    }
    else if (strncmp(ptr, "script", 6) == 0) {
        strcpy(out.tag_name, "script");
        link_attr_prefix = "src=\"";
    } else {
        return out;
    }

    int progress_step = 0;
    enum CaptureAttr capture_attr = CAPTURE_NO;
    const char* capture_start = NULL;

    while ((ptr - buff) < MAX_HTML_TAG_LENGTH) {
        switch (progress_step) {
            case 0:
                // printf("---> \t2: (%.*s)\n", 10, ptr);
                if (*ptr == '>') {
                    goto end;
                } else if (strncmp(ptr, link_attr_prefix, strlen(link_attr_prefix)) == 0) {
                    progress_step++;
                    ptr += strlen(link_attr_prefix) - 1;
                    capture_start = ptr + 1;
                    capture_attr = CAPTURE_LINK;
                } else if (strncmp(ptr, "type=\"", 6) == 0) {
                    progress_step++;
                    ptr += 5;
                    capture_start = ptr + 1;
                    capture_attr = CAPTURE_TYPE;
                }
                break;
            case 1: // STATE_CAPTURE
                if (*ptr == '\"') { // End of arg capture
                    int captured_size = ptr - capture_start;
                    if (captured_size > 0) {
                        if (capture_attr == CAPTURE_LINK) {
                            // out.link = { capture_start, captured_size };
                            out.link.ptr = (char*)capture_start;
                            out.link.size = captured_size;
                        } else if (capture_attr == CAPTURE_TYPE) {
                            // out.type = { capture_start, captured_size };
                            out.type.ptr = (char*)capture_start;
                            out.type.size = captured_size;
                        }
                    }
                    progress_step = 0;
                    capture_attr = CAPTURE_NO;
                    capture_start = NULL;
                }
                break;
            default:
                fprintf(stderr, "=== Unknown progress_step: %d\n", progress_step);

        }
        ptr++;
    }

end:
    out.tag_end = ptr;
    return out;
}


void iter_html_tags(
    struct vec data,
    void(*callback)(struct HtmlTag* t, const void* ctx, FILE* fout),
    void* ctx
) {
    /*
     * Takes: ptr to string
     * Calls @callback on each occurence of URL
     */
    const char* init = data.ptr;
    const char* ptr = data.ptr;
    int left = data.size;
    while (left > 0 && (ptr = (const char*)memchr(ptr, '<', left))) {
        left = data.size - (ptr - init);
        
        struct HtmlTag t = parse_html_tag(ptr);
        if (t.link.ptr == NULL) {
            ptr++;
            continue;
        }

        callback(&t, ctx, stdout);
        ptr = t.tag_end + 1;
    }
}

// -- cached_network.c

int cached_download_http(struct vec url, int request_timeout, int is_save, int cache_ttl, LinkType type_hint, struct HttpPage* out) {
    char _mem1[MAX_URL_LENGTH];
    struct vec url_path = {_mem1, 0};

    url_save_path(url, type_hint, &url_path);

    time_t cur_time = time(NULL);
    time_t mtime = file_mtime(url_path);

    if (file_exists(url_path) && cache_ttl * 60 >= (cur_time - mtime)) {
        fprintf(stderr, ANSI_COLOR_GREEN "==== Cache hit: %.*s (file: %.*s)\n" ANSI_COLOR_RESET, url.size, url.ptr, url_path.size, url_path.ptr);
        char* buff;
        vec_terminate(&url_path);
        int size = read_file(url_path.ptr, &buff);

        if (out) {
            out->content_vec = (struct vec){ buff, size };
            vec_append(&out->effective_url, false, url);
        }
        return 1;
    }

    fprintf(stderr, ANSI_COLOR_YELLOW "==== Cache miss: %.*s\n\t(file: %s)\n" ANSI_COLOR_RESET, url.size, url.ptr, url_path);

    int res = download_http(url, request_timeout / 1000, out);
    if (res == 0) {
        assert(out->content_vec.size > 0);
        assert(out->headers_vec.size > 0);

        if (is_save) {
            // Save page to FS
            char _mem2[MAX_URL_LENGTH];
            struct vec path = {_mem2, 0};
            url_save_path(out->effective_url, type_hint, &path);

            // Create needed directory
            char dir_path[MAX_URL_LENGTH];
            struct vec v = {dir_path, 0};
            vec_append(&v, false, (struct vec){ path.ptr, dirname_len(path) });
            vec_terminate(&v);

            if (!mkdir_p(dir_path)) {
                fprintf(stderr, ANSI_COLOR_RED "==== Could not mkdir: %s\n" ANSI_COLOR_RESET, dir_path);
                goto end;
            }

            // Save file to FS
            vec_terminate(&path);
            bool ok = write_file(
                path.ptr,
                out->content_vec.ptr,
                out->content_vec.size
            );

            if (!ok) {
                fprintf(stderr, ANSI_COLOR_RED "==== Could not save file: %.*s\n" ANSI_COLOR_RESET, path.size, path.ptr);
                goto end;
            }
        }
    }

end:
    return res;
}

void printf_consume(FILE* fout, const char** ptr, const char* until) {
    assert(until > *ptr);
    int size = until - *ptr;
    fprintf(fout, "%.*s", size, *ptr);
    (*ptr) += size;
}

void link_repr_path(struct HtmlTag* t, struct vec page_hint, struct vec* out) {
    char _mem1[MAX_URL_LENGTH];
    struct vec full_url = {_mem1, 0};
    link_to_full_url(t->link, page_hint, &full_url);

    only_host_path(full_url, out);
        
    /* We do not want to add anything additional to representation link. */
    // LinkType lt = tag_link_type(t);
    // if (is_url_represents_file(t->link, lt)) {
    //     vec_append(out, false, vec_wrap("/index.html"));
    // }
}

void url_save_path(struct vec url, LinkType lt, struct vec* out) {
    only_host_path(url, out);

    // Add suffix for every html URL just for the case.
    // It is ok since this is a save path, we will not break anything.
    if ((lt == LINK_TYPE_HTML || lt == LINK_TYPE_UNKNOWN) && (!ends_with(*out, vec_wrap("/index.html")))) {
        vec_append(out, false, vec_wrap("/index.html"));
    }
}

void replace_links(struct vec data, struct vec page_hint, FILE* fout) {
    const char* ptr = data.ptr;
    int left = data.size;
    while(left > 0) {
        left = data.size - (ptr - data.ptr);
        if (*ptr != '<') {
            printf_consume(fout, &ptr, ptr + 1);
            continue;
        }

        struct HtmlTag t = parse_html_tag(ptr);
        if (t.link.ptr == NULL) {
            printf_consume(fout, &ptr, ptr + 1);
            continue;
        }

        printf_consume(fout, &ptr, t.link.ptr);

        // fprintf(stderr, ANSI_COLOR_GREEN "  F %s\n" ANSI_COLOR_RESET, page_hint);
        // fprintf(stderr, ANSI_COLOR_YELLOW "  ? %.*s\n" ANSI_COLOR_RESET, t.link.size, t.link.ptr);

        if (strncmp(t.link.ptr, "../", 3) == 0) {
            // fprintf(stderr, ANSI_COLOR_YELLOW "S-> %.*s\n" ANSI_COLOR_RESET, t.link.size, t.link.ptr);
            printf_consume(fout, &ptr, t.tag_end);
            ptr = t.tag_end;
            continue;
        }

        char _mem1[MAX_URL_LENGTH];
        struct vec fp = {_mem1, 0};
        link_repr_path(&t, page_hint, &fp);

        char _mem2[MAX_URL_LENGTH];
        struct vec relpath_ = {_mem2, 0};
        relpath(
            // Cut filename
            (struct vec){ page_hint.ptr, dirname_len(page_hint) },
            fp,
            &relpath_
        );

        fprintf(fout, "%.*s", relpath_.size, relpath_.ptr);
        fprintf(stderr, ANSI_COLOR_GREEN "2-> %.*s\n" ANSI_COLOR_RESET, relpath_.size, relpath_.ptr);

        ptr = t.link.ptr + t.link.size;
    }
}

// -- hrefs_crawler.c

bool should_crawl_url(struct vec url, struct vec page_url, DomainFilterType filter_type) {
    struct UrlParts url_p = UrlParts_init();
    url_parts(url, &url_p);

    struct UrlParts page_url_p = UrlParts_init();
    url_parts(page_url, &page_url_p);

    switch (filter_type) {
        case DOMAIN_FILTER_NO:
            return true;
        case DOMAIN_FILTER_SAME:
            return vec_eq2(url_p.host, page_url_p.host);
        case DOMAIN_FILTER_SUBDOMAIN:
            // Check whether url is a subdomain of page_url, f.e.:
            // url_p.host        =  terms.archlinux.org
            // page_url_p.host   =        archlinux.org
            // Then first is a subdomain of the second
            return ends_with(url_p.host, page_url_p.host);
        default:
            assert(false);
    }
}

void link_to_full_url(struct vec link, struct vec page_url, struct vec* out) {
    /*
     * This function build a full URL which a given href link tries to represent.
     * For example:
     *
     * IN:
     *     link = static/image.png
     *     page_url = archlinux.org/index.html
     * OUT:
     *     out = archlinux.org/static/image.png
     */

    lstrip(&page_url, vec_wrap("./"));

    if (is_url_http(link.ptr)) {
        vec_append(out, false, link);
        return;
    }
    if (is_abs_path(link.ptr)) {
        struct UrlPtrs ptrs = url_pointers(page_url);
        page_url.size = ptrs.host_end - page_url.ptr;
    } else {
        // Otherwise we are dealing with relative link
        if (is_url_represents_file(page_url, LINK_TYPE_HTML)) {
            page_url.size = url_dirname_len(page_url);
        }
    }

    struct UrlParts p_page_url = UrlParts_init();
    url_parts(page_url, &p_page_url);

    vec_append(out, false, p_page_url.protocol);
    vec_append(out, false, p_page_url.host);
    vec_append(out, false, p_page_url.path);
    rstrip(out, '/');
    if (!ends_with(*out, vec_wrap("/")) && link.ptr[0] != '/') {
        vec_append(out, false, vec_wrap("/"));
    }
    vec_append(out, false, link);
}
