
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

struct vec {
    size_t size;
    char* ptr;
    bool allocated;
};

struct vec* vec_init(size_t newsize);
void vec_deinit(struct vec* v);
void vec_append(struct vec* v, const char* recv_buff, size_t newsize);

bool is_number(const char* s);
bool is_alphabet(char c);
bool ends_with(const char* str, const char* suffix);
void debug_string(const char *str, int size, const char *name);
size_t strlen_with_delims(const char *s);

size_t get_file_size(const char* path);
time_t get_file_last_modified(const char* path);
bool file_exists(const char* path);

int read_file(const char* path, char** out);
int write_file(const char* path, const char* buff, size_t buff_size);

bool mkdir_p(const char* dir);
