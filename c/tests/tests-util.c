#include <assert.h>

#include "./tests-common.h"
#include "../util.h"

// TODO fix relative paths
#define TEST_UTIL_READ_FILE(name, path, res_expected, out_expected) \
    TEST(util_##name) { \
        char* out; \
        int res = read_file(path, &out); \
        ASSERT_EQUAL_INT(res, res_expected); \
        if (res != -1) { \
            ASSERT_EQUAL_STR(out, out_expected); \
        } \
    } \

#define TEST_UTIL_WRITE_FILE(name, path, write_data, res_expected) \
    TEST(util_##name) { \
        int res = write_file(path, write_data, 6); \
        ASSERT_EQUAL_INT(res, res_expected); \
    } \

TEST_UTIL_READ_FILE(1, "non-existing-path", -1, "");
TEST_UTIL_READ_FILE(2, "./out/files/test_util_1.txt", 0, "123\n");
TEST_UTIL_WRITE_FILE(3, "./out/files/test_util_1_write.txt", "12345\n", 0);

int main() {
    RUN_TEST(util_1);
    RUN_TEST(util_2);
    RUN_TEST(util_3);
}
