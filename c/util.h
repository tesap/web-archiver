
struct vec {
    size_t size;
    char* ptr;
    bool allocated;
};

struct vec* vec_init(size_t newsize);
void vec_deinit(struct vec* v);
void vec_append(struct vec* v, const char* recv_buff, size_t newsize);
bool is_number(char* s);
bool ends_with(char* str, char* suffix);
