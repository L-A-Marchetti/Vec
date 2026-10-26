#include "vec.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Request {
    unsigned status;
    uint64_t bytes;
} Request;

/* Input: one "HTTP_STATUS RESPONSE_BYTES" pair per line, for example:
 * printf '200 512\n404 128\n200 1024\n' | ./build/example
 * Blank lines are ignored; malformed or oversized lines are rejected.
 */
static int parse_number(char **cursor, uint64_t *value)
{
    while (isspace((unsigned char)**cursor)) ++*cursor;
    if (!isdigit((unsigned char)**cursor)) return 0;
    char *end;
    errno = 0;
    uintmax_t parsed = strtoumax(*cursor, &end, 10);
    if (errno == ERANGE || parsed > UINT64_MAX) return 0;
    *cursor = end;
    *value = (uint64_t)parsed;
    return 1;
}

int main(void)
{
    Vec requests = vec_new(sizeof(Request));
    char line[256];
    size_t line_number = 0;
    while (fgets(line, sizeof line, stdin) != NULL) {
        ++line_number;
        char *cursor = line;
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (*cursor == '\0') continue;
        uint64_t status, bytes;
        if ((strchr(line, '\n') == NULL && !feof(stdin)) ||
            !parse_number(&cursor, &status) ||
            !isspace((unsigned char)*cursor) ||
            !parse_number(&cursor, &bytes)) goto invalid;
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (*cursor != '\0' || status < 100 || status > 599) goto invalid;
        Request request = { (unsigned)status, bytes };
        if (vec_push(&requests, &request) == NULL) {
            perror("vec_push");
            vec_destroy(&requests);
            return 1;
        }
        continue;
invalid:
        fprintf(stderr, "Invalid request on line %zu (expected status 100..599 and bytes)\n",
                line_number);
        vec_destroy(&requests);
        return 1;
    }
    if (ferror(stdin)) {
        fputs("Could not read stdin\n", stderr);
        vec_destroy(&requests);
        return 1;
    }

    /* Retain parsed records, then group them by HTTP status. */
    size_t counts[600] = {0};
    uint64_t totals[600] = {0};
    for (Request *it = vec_begin(&requests); it != vec_end(&requests); ++it) {
        if (totals[it->status] > UINT64_MAX - it->bytes) {
            fputs("Response byte total overflow\n", stderr);
            vec_destroy(&requests);
            return 1;
        }
        counts[it->status]++;
        totals[it->status] += it->bytes;
    }
    puts("status requests bytes");
    for (unsigned status = 100; status <= 599; ++status) {
        if (counts[status] != 0)
            printf("%u %zu %" PRIu64 "\n", status, counts[status], totals[status]);
    }
    vec_destroy(&requests);
    return 0;
}
