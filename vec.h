#ifndef VEC_H
#define VEC_H

#include <stddef.h>

typedef struct Vec {
    unsigned char *data;
    size_t len;
    size_t capacity;
    size_t elem_size;
} Vec;

/* elem_size must be nonzero; otherwise returns a zero vector, errno=EINVAL.
 * Vec owns its buffer: do not copy an owning Vec by assignment.
 * Elements are shallow byte copies; their resources belong to the caller.
 */
Vec vec_new(size_t elem_size);
/* Returns the inserted element, or NULL on failure. An existing element of
 * this vector may be supplied, including when growth reallocates the buffer.
 * Allocation failure/overflow: ENOMEM. Invalid element size/source: EINVAL.
 * Failure leaves the vector unchanged.
 */
void *vec_push(Vec *v, const void *elem);
void vec_pop(Vec *v);
/* Returns 1 on success, 0 on failure; failure leaves the vector unchanged. */
int vec_shrink(Vec *v);
void *vec_at(Vec *v, size_t i);
int vec_reserve(Vec *v, size_t new_capacity);
void vec_clear(Vec *v);
void *vec_back(Vec *v);
int vec_remove(Vec *v, size_t i);
void *vec_begin(Vec *v);
void *vec_end(Vec *v);
/* Releases only the buffer, then zeros the vector. Safe to repeat. */
void vec_destroy(Vec *v);

#endif
