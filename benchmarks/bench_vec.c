#include "vec.h"
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { MAX_TRIALS = 99, SCAN_PASSES = 16 };
typedef struct Record { uint32_t value; unsigned char payload[60]; } Record;
_Static_assert(sizeof(Record) == 64, "benchmark requires 64-byte records");
static volatile uint64_t sink;

static void fail(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static clock_t now(void)
{
    clock_t value = clock();
    if (value == (clock_t)-1) fail("CPU clock unavailable");
    return value;
}

static double elapsed(clock_t start)
{
    clock_t end = now();
    if (end < start) fail("CPU clock wrapped");
    return (double)(end - start) / CLOCKS_PER_SEC;
}

static int compare_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void report(const char *type, const char *operation, const char *impl,
                   size_t n, size_t trials, double *times, uint64_t checksum)
{
    qsort(times, trials, sizeof *times, compare_double);
    double median = trials % 2 ? times[trials / 2] :
        (times[trials / 2 - 1] + times[trials / 2]) / 2;
    printf("%s,%s,%s,%zu,%zu,%.3f,%.3f,%.3f,%" PRIu64 "\n",
           type, operation, impl, n, trials,
           times[0] * 1000, median * 1000, times[trials - 1] * 1000, checksum);
}

static uint32_t make_u32(size_t i) { return (uint32_t)i; }
static uint64_t get_u32(uint32_t value) { return value; }
static Record make_record(size_t i)
{
    Record value = {0};
    value.value = (uint32_t)i;
    memset(value.payload, (int)(i % 251), sizeof value.payload);
    return value;
}
static uint64_t get_record(Record value) { return value.value; }

/* The typed baseline intentionally lets the compiler inline pushes and know
 * the element size. Vec is linked separately without LTO. Both use realloc,
 * capacity doubling, identical input, and identical validation.
 */
#define DEFINE_BENCH(T, NAME, MAKE, GET) \
static void bench_##NAME(size_t n, size_t trials) \
{ \
    uint64_t expected = (uint64_t)n * (n - 1) / 2; \
    for (int operation = 0; operation < 3; ++operation) { \
        double times[2][MAX_TRIALS]; \
        for (size_t trial = 0; trial <= trials; ++trial) { \
            /* Untimed warm-up trial; alternate implementation order. */ \
            for (int turn = 0; turn < 2; ++turn) { \
                int impl = (turn + (int)(trial % 2)) % 2; \
                Vec v = vec_new(sizeof(T)); \
                T *data = NULL; \
                size_t len = 0, capacity = 0; \
                clock_t start = now(); \
                if (operation != 0) { \
                    if (impl == 0) { \
                        if (!vec_reserve(&v, n)) fail("Vec reserve failed"); \
                    } else { \
                        data = malloc(n * sizeof(T)); \
                        if (!data) fail("typed reserve failed"); \
                        capacity = n; \
                    } \
                } \
                if (impl == 0) { \
                    for (size_t i = 0; i < n; ++i) { \
                        T item = MAKE(i); \
                        if (!vec_push(&v, &item)) fail("Vec push failed"); \
                    } \
                    data = (T *)vec_begin(&v); \
                    len = v.len; \
                } else { \
                    for (size_t i = 0; i < n; ++i) { \
                        T item = MAKE(i); \
                        if (len == capacity) { \
                            size_t next = capacity ? capacity * 2 : 1; \
                            T *tmp = realloc(data, next * sizeof(T)); \
                            if (!tmp) fail("typed growth failed"); \
                            data = tmp; \
                            capacity = next; \
                        } \
                        data[len++] = item; \
                    } \
                } \
                double seconds = elapsed(start); \
                uint64_t sum = 0; \
                if (operation == 2) { \
                    /* Scan starts after construction; direct typed traversal \
                     * for both buffers, as recommended by the API. */ \
                    start = now(); \
                    for (int pass = 0; pass < SCAN_PASSES; ++pass) { \
                        for (size_t i = 0; i < len; ++i) sum += GET(data[i]); \
                        sink = sum; \
                    } \
                    seconds = elapsed(start); \
                    if (sum != expected * SCAN_PASSES) fail("scan checksum mismatch"); \
                } \
                sum = 0; \
                if (len != n) fail("length mismatch"); \
                for (size_t i = 0; i < len; ++i) { \
                    T item = MAKE(i); \
                    if (memcmp(&data[i], &item, sizeof(T)) != 0) fail("data mismatch"); \
                    sum += GET(data[i]); \
                } \
                if (sum != expected) fail("checksum mismatch"); \
                sink = sum; \
                if (trial != 0) times[impl][trial - 1] = seconds; \
                if (impl == 0) vec_destroy(&v); else free(data); \
            } \
        } \
        const char *op = operation == 0 ? "push_growth" : \
            operation == 1 ? "push_reserved" : "scan_16_passes"; \
        report(#NAME, op, "vec", n, trials, times[0], expected); \
        report(#NAME, op, "typed", n, trials, times[1], expected); \
    } \
}

DEFINE_BENCH(uint32_t, u32, make_u32, get_u32)
DEFINE_BENCH(Record, record64, make_record, get_record)

static size_t argument(const char *text, size_t limit)
{
    if (*text < '0' || *text > '9') fail("arguments must be positive integers");
    char *end;
    errno = 0;
    uintmax_t value = strtoumax(text, &end, 10);
    if (errno == ERANGE || *end || value == 0 || value > limit)
        fail("argument out of range");
    return (size_t)value;
}

int main(int argc, char **argv)
{
    if (argc > 3) fail("usage: bench_vec [elements:1..10000000] [trials:1..99]");
    size_t n = argc > 1 ? argument(argv[1], 10000000) : 1000000;
    size_t trials = argc > 2 ? argument(argv[2], MAX_TRIALS) : 7;
    if (n > SIZE_MAX / sizeof(Record) / 2) fail("input too large for platform");
    puts("type,operation,implementation,elements,trials,min_ms,median_ms,max_ms,checksum");
    bench_u32(n, trials);
    bench_record64(n, trials);
    return 0;
}
