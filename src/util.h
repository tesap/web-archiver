
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <time.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

struct vec {
    char* ptr;
    size_t size;
    // bool allocated;
};

struct UrlPtrs {
    const char* protocol_start;
    const char* protocol_end;
    const char* host_start;
    const char* host_end;
    const char* path_start;
    const char* path_end;
};

struct UrlParts {
    struct vec protocol;
    struct vec host;
    struct vec path;
};

struct HttpPage {
    struct vec effective_url;
    struct vec headers_vec; // Heap-located
    struct vec content_vec; // Heap-located
};

typedef enum {
    LINK_TYPE_UNKNOWN,
    LINK_TYPE_HTML,
    LINK_TYPE_IMG,
    LINK_TYPE_STYLE,
    LINK_TYPE_SCRIPT,
} LinkType;

struct HtmlTag {
    char tag_name[16];
    const char* tag_end;
    struct vec link;
    struct vec type;
};

typedef enum {
    DOMAIN_FILTER_NO,
    DOMAIN_FILTER_SAME,
    DOMAIN_FILTER_SUBDOMAIN,
} DomainFilterType;

struct vec vec_wrap(const char* s);
void vec_terminate(struct vec* v);

struct vec vec_init();
struct vec vec_alloc(size_t newsize);
void vec_dealloc(struct vec v);
void vec_append(struct vec* v, bool is_dynamic, struct vec add);
void vec_append_cstring(struct vec* v, bool is_dynamic, const char* recv_buff);
bool vec_eq(struct vec v, const char* cstr);
bool vec_eq2(struct vec v1, struct vec v2);
void vec_to_cstr(struct vec v, char* out);

struct HttpPage HttpPage_init(char* stack_buff);
void HttpPage_dealloc(struct HttpPage p);

struct UrlParts UrlParts_init();

bool is_number(const char* s);
bool is_alphabet(char c);
bool starts_with(struct vec s, struct vec suffix);
bool ends_with(struct vec s, struct vec suffix);
void debug_string(const char *str, int size, const char *name);
size_t strlen_with_delims(const char *s);

size_t file_size(const char* path);
time_t file_mtime(struct vec path);
bool file_exists(struct vec path);

int read_file(const char* path, char** out);
int write_file(const char* path, const char* buff, size_t buff_size);

bool mkdir_p(const char* dir);
void rstrip(struct vec* s, char c);
void lstrip(struct vec* s, struct vec prefix);

// --- url_parser.h
bool is_abs_path(const char* s);
bool is_url_http(const char* url);

struct UrlPtrs url_pointers(struct vec url);
void url_parts(struct vec url, struct UrlParts* parts);
bool is_url_represents_file(struct vec url, LinkType type_hint);
void only_host_path(struct vec url, struct vec* out);
void relpath(
    struct vec path_from,
    struct vec path_to,
    struct vec* result
);
int dirname_len(struct vec path);
int url_dirname_len(struct vec url);
int first_dir_len(struct vec path);
int dir_count(struct vec path);
void join_paths(struct vec p1, struct vec p2, struct vec* out);
// ---

// --- network.c
int create_tcp_socket(struct vec hostname, const char* service);
int download_http(struct vec url, int timeout_sec, struct HttpPage* out);
void print_sockaddr(struct sockaddr* sa);
// ---

// --- http_parser.c
void parse_http_stream_chunk(const char* recv_buff, int size, struct vec* headers_vec, struct vec* content_vec, bool* content_started);
int get_location_header(struct vec headers_data, struct vec request_url, char* result);
// ---

// -- html_parser.c
// struct HtmlTag HtmlTag_init();
LinkType tag_link_type(struct HtmlTag* t);
// void print_link_type(LinkType lt);
struct HtmlTag parse_html_tag(const char* buff);
void iter_html_tags(
    struct vec data,
    void(*callback)(struct HtmlTag* t, const void* ctx, FILE* fout),
    void* ctx
);

// -- cached_network.c
void save_downloaded_page(const struct HttpPage* hp);
int cached_download_http(struct vec url, int request_timeout, int is_save, int cache_ttl, LinkType type_hint, struct HttpPage* out);

// -- links_replacer.c
// void printf_consume(int fd_out, const char** ptr, const char* until);
void replace_links(struct vec data, struct vec filepath_hint, FILE* fout);
void link_repr_path(struct HtmlTag* t, struct vec page_hint, struct vec* out);
void url_save_path(struct vec url, LinkType lt, struct vec* out);

// -- hrefs_crawler.c
bool should_crawl_url(struct vec url, struct vec page_url, DomainFilterType filter_type);
void link_to_full_url(struct vec link, struct vec page_url, struct vec* out);
#endif

