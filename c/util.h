
struct vec {
    size_t size;
    char* ptr;
    bool allocated;
};

struct vec* vec_init(size_t newsize);
void vec_deinit(struct vec* v);
void vec_append(struct vec* v, const char* recv_buff, size_t newsize);
bool is_number(const char* s);
bool ends_with(const char* str, const char* suffix);
void debug_string(const char *str, const char *name);
size_t strlen_with_delims(const char *s);
const char * strstr_with_delims(const char *s1, const char *s2);
const char * strchr_with_delims(const char *s1, const char ch);

int read_file(const char* path, char** out);
int write_file(const char* path, const char* buff, size_t buff_size);
