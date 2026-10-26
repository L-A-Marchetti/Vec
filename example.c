#include "vec.h"
#include <stdio.h>

/* Collect integers from stdin, remove the first, then print the remainder.
 * Example: printf '10 20 30\n' | ./build/example
 */
int main(void)
{
    Vec values = vec_new(sizeof(int));
    int value;
    int status;
    while ((status = scanf("%d", &value)) == 1) {
        if (vec_push(&values, &value) == NULL) {
            perror("vec_push");
            vec_destroy(&values);
            return 1;
        }
    }
    if (status != EOF || ferror(stdin)) {
        fputs("Could not read integers from stdin\n", stderr);
        vec_destroy(&values);
        return 1;
    }
    vec_remove(&values, 0);
    for (int *it = vec_begin(&values); it != vec_end(&values); ++it) {
        printf("%d\n", *it);
    }
    vec_destroy(&values);
    return 0;
}
