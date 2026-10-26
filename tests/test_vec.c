#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "test.h"

#include <vec.h>
#include <stdint.h>

static void test_shrink_on_empty(void) {
    Vec v = vec_new(sizeof(int));
    vec_shrink(&v);
    EXPECT_EQ_ULL(v.len, 0);
    EXPECT_EQ_ULL(v.capacity, 0);
    EXPECT_TRUE(v.data == NULL);
    vec_destroy(&v);
}

static void test_pop_on_empty(void) {
    Vec v = vec_new(sizeof(int));
    vec_pop(&v);
    vec_pop(&v);
    EXPECT_EQ_ULL(v.len, 0);
    vec_destroy(&v);
}

static void test_push_after_shrink(void) {
    Vec v = vec_new(sizeof(char));
    const char s[] = "abcdef";
    for (size_t i = 0; i < sizeof(s)-1; ++i) {
        EXPECT_TRUE(vec_push(&v, &s[i]) != NULL);
    }
    vec_pop(&v); // remove 'f'
    vec_pop(&v); // remove 'e'
    vec_shrink(&v);
    char g = 'g';
    EXPECT_TRUE(vec_push(&v, &g) != NULL);
    EXPECT_EQ_ULL(v.len, 5);
    EXPECT_EQ_ULL(*(char*)vec_at(&v, v.len-1), 'g');
    vec_destroy(&v);
}

static void test_alternating_push_pop(void) {
    Vec v = vec_new(sizeof(int));
    for (int i = 0; i < 10000; ++i) {
        int x = i;
        EXPECT_TRUE(vec_push(&v, &x) != NULL);
        EXPECT_EQ_ULL(v.len, 1);
        vec_pop(&v);
        EXPECT_EQ_ULL(v.len, 0);
    }
    vec_destroy(&v);
}

struct Big { unsigned char buf[128]; int tag; };

static void test_large_elem_size(void) {
    Vec v = vec_new(sizeof(struct Big));
    struct Big a = { {0}, 42 };
    memset(a.buf, 0xAA, sizeof a.buf);
    struct Big b = { {0}, 7 };
    memset(b.buf, 0xBB, sizeof b.buf);
    EXPECT_TRUE(vec_push(&v, &a) != NULL);
    EXPECT_TRUE(vec_push(&v, &b) != NULL);
    struct Big *pa = (struct Big*)vec_at(&v, 0);
    struct Big *pb = (struct Big*)vec_back(&v);
    EXPECT_TRUE(pa && pb);
    EXPECT_EQ_INT(pa->tag, 42);
    EXPECT_EQ_INT(pb->tag, 7);
    EXPECT_EQ_ULL(v.len, 2);
    vec_destroy(&v);
}

static void test_overflow_guard(void) {
    Vec v = vec_new(8);
    errno = 0;
    size_t huge = (SIZE_MAX / 8) + 1;
    int ok = vec_reserve(&v, huge);
    EXPECT_EQ_INT(ok, 0);
    EXPECT_EQ_INT(errno, ENOMEM);
    vec_destroy(&v);
}

static void test_bounds_at_back(void) {
    Vec v = vec_new(sizeof(int));
    EXPECT_TRUE(vec_at(&v, 0) == NULL);
    EXPECT_TRUE(vec_back(&v) == NULL);
    int x = 123;
    vec_push(&v, &x);
    EXPECT_TRUE(vec_at(&v, 1) == NULL);
    EXPECT_EQ_INT(*(int*)vec_back(&v), 123);
    vec_destroy(&v);
}

static void test_remove_and_clear(void) {
    Vec v = vec_new(sizeof(int));
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(vec_push(&v, &i) != NULL);
    }
    EXPECT_EQ_ULL(v.len, 5);
    EXPECT_EQ_INT(vec_remove(&v, 0), 1); // remove first
    EXPECT_EQ_INT(*(int*)vec_at(&v, 0), 1);
    EXPECT_EQ_INT(vec_remove(&v, 2), 1); // remove former 3 (now at index 2)
    EXPECT_EQ_ULL(v.len, 3);
    EXPECT_EQ_INT(*(int*)vec_at(&v, 0), 1);
    EXPECT_EQ_INT(*(int*)vec_at(&v, 1), 2);
    EXPECT_EQ_INT(*(int*)vec_at(&v, 2), 4);
    vec_clear(&v);
    EXPECT_EQ_ULL(v.len, 0);
    size_t cap_after = v.capacity;
    EXPECT_TRUE(cap_after > 0); // capacity remains
    vec_destroy(&v);
}

static void test_iterators(void) {
    // empty vector: begin == end (both may be NULL)
    {
        Vec v = vec_new(sizeof(int));
        EXPECT_TRUE(vec_begin(&v) == vec_end(&v));
        vec_destroy(&v);
    }

    // ints: iterate from begin to end and sum
    {
        Vec v = vec_new(sizeof(int));
        for (int i = 1; i <= 5; ++i) vec_push(&v, &i);
        int *b = (int*)vec_begin(&v);
        int *e = (int*)vec_end(&v);
        int sum = 0;
        for (int *p = b; p != e; ++p) sum += *p;
        EXPECT_EQ_INT(sum, 1+2+3+4+5);
        vec_destroy(&v);
    }

    // chars: verify last element via end-1
    {
        Vec v = vec_new(sizeof(char));
        const char s[] = "abcd";
        for (size_t i = 0; i < sizeof(s)-1; ++i) vec_push(&v, &s[i]);
        char *e = (char*)vec_end(&v);
        EXPECT_EQ_INT(*(e-1), 'd');
        vec_destroy(&v);
    }
}


static void test_reserve_success(void) {
    Vec v = vec_new(sizeof(int));
    EXPECT_EQ_INT(vec_reserve(&v, 100), 1);
    EXPECT_TRUE(v.capacity >= 100);
    EXPECT_EQ_ULL(v.len, 0);
    vec_destroy(&v);
}

static void test_push_overflow_guard_on_growth(void) {
    // Exhaust the maximum representable capacity for eight-byte elements.
    Vec v = vec_new(8);
    v.capacity = SIZE_MAX / 8; // no further element fits
    v.len = v.capacity;              // force growth path
    errno = 0;
    int x = 42;
    void *p = vec_push(&v, &x);
    EXPECT_TRUE(p == NULL);
    EXPECT_EQ_INT(errno, ENOMEM);
    EXPECT_EQ_ULL(v.len, v.capacity); // unchanged on failure
    vec_destroy(&v);
}

static void test_self_push(void) {
    Vec v = vec_new(sizeof(int));
    int x = 42;
    EXPECT_TRUE(vec_push(&v, &x) != NULL);
    EXPECT_TRUE(vec_push(&v, vec_at(&v, 0)) != NULL);
    EXPECT_EQ_INT(*(int*)vec_back(&v), 42);
    EXPECT_TRUE(vec_reserve(&v, 8));
    EXPECT_TRUE(vec_push(&v, vec_at(&v, 1)) != NULL);
    EXPECT_EQ_INT(*(int*)vec_back(&v), 42);
    vec_destroy(&v);
}

static void test_invalid_and_destroy(void) {
    errno = 0;
    Vec v = vec_new(0);
    EXPECT_EQ_INT(errno, EINVAL);
    int x = 1;
    EXPECT_TRUE(vec_push(&v, &x) == NULL);
    EXPECT_EQ_INT(errno, EINVAL);
    EXPECT_EQ_INT(vec_reserve(&v, 1), 0);
    vec_destroy(&v);
    v = vec_new(sizeof(int));
    EXPECT_TRUE(vec_push(&v, NULL) == NULL);
    EXPECT_EQ_INT(errno, EINVAL);
    EXPECT_EQ_ULL(v.len, 0);
    EXPECT_TRUE(vec_push(&v, &x) != NULL);
    vec_destroy(&v);
    vec_destroy(&v);
    EXPECT_TRUE(v.data == NULL);
    EXPECT_EQ_ULL(v.elem_size, 0);
    EXPECT_EQ_ULL(v.capacity, 0);
    EXPECT_EQ_ULL(v.len, 0);
}

int main(void) {
    RUN_TEST(test_self_push);
    RUN_TEST(test_invalid_and_destroy);
    RUN_TEST(test_shrink_on_empty);
    RUN_TEST(test_pop_on_empty);
    RUN_TEST(test_push_after_shrink);
    RUN_TEST(test_alternating_push_pop);
    RUN_TEST(test_large_elem_size);
    RUN_TEST(test_overflow_guard);
    RUN_TEST(test_bounds_at_back);
    RUN_TEST(test_remove_and_clear);
    RUN_TEST(test_reserve_success);
    RUN_TEST(test_iterators);
    RUN_TEST(test_push_overflow_guard_on_growth);

    return test_summary("vec");
}
