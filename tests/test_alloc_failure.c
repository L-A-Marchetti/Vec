#include <stdlib.h>
#include "test.h"
#include "vec.h"

static int fail_allocation;
static void *test_realloc(void *ptr, size_t bytes)
{
    if (fail_allocation) return NULL;
    return realloc(ptr, bytes);
}

/* Override only this translation unit; no allocator hooks in the public API. */
#define realloc test_realloc
#include "../vec.c"
#undef realloc

static void test_allocation_failure(void)
{
    Vec v = vec_new(sizeof(int));
    int x = 42;
    fail_allocation = 1;
    EXPECT_TRUE(vec_push(&v, &x) == NULL);
    EXPECT_EQ_INT(errno, ENOMEM);
    EXPECT_TRUE(v.data == NULL);
    EXPECT_EQ_ULL(v.capacity, 0);
    EXPECT_EQ_ULL(v.len, 0);
    fail_allocation = 0;
    EXPECT_TRUE(vec_push(&v, &x) != NULL);
    unsigned char *original = v.data;
    fail_allocation = 1;
    EXPECT_TRUE(vec_push(&v, vec_at(&v, 0)) == NULL);
    EXPECT_EQ_INT(errno, ENOMEM);
    EXPECT_EQ_INT(vec_reserve(&v, 8), 0);
    EXPECT_EQ_INT(errno, ENOMEM);
    EXPECT_TRUE(v.data == original);
    EXPECT_EQ_ULL(v.capacity, 1);
    EXPECT_EQ_ULL(v.len, 1);
    EXPECT_EQ_INT(*(int*)vec_at(&v, 0), 42);
    fail_allocation = 0;
    EXPECT_TRUE(vec_reserve(&v, 8));
    original = v.data;
    fail_allocation = 1;
    EXPECT_EQ_INT(vec_shrink(&v), 0);
    EXPECT_EQ_INT(errno, ENOMEM);
    EXPECT_TRUE(v.data == original);
    EXPECT_EQ_ULL(v.capacity, 8);
    EXPECT_EQ_ULL(v.len, 1);
    EXPECT_EQ_INT(*(int*)vec_at(&v, 0), 42);
    fail_allocation = 0;
    EXPECT_EQ_INT(vec_shrink(&v), 1);
    EXPECT_EQ_ULL(v.capacity, 1);
    vec_destroy(&v);
}

int main(void)
{
    RUN_TEST(test_allocation_failure);
    return test_summary("allocation failure");
}
