#include "vec.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Vec vec_new(size_t elem_size)
{
    if (elem_size == 0) errno = EINVAL;
    return (Vec){ .elem_size = elem_size };
}

int vec_reserve(Vec *v, size_t new_capacity)
{
    if (v->elem_size == 0) {
        errno = EINVAL;
        return 0;
    }
    if (new_capacity <= v->capacity) return 1;
    if (new_capacity > SIZE_MAX / v->elem_size) {
        errno = ENOMEM;
        return 0;
    }
    void *tmp = realloc(v->data, new_capacity * v->elem_size);
    if (!tmp) {
        errno = ENOMEM;
        return 0;
    }
    v->data = tmp;
    v->capacity = new_capacity;
    return 1;
}

void *vec_push(Vec *v, const void *elem)
{
    if (v->elem_size == 0 || elem == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (v->len == v->capacity) {
        size_t max_capacity = SIZE_MAX / v->elem_size;
        if (v->capacity >= max_capacity) {
            errno = ENOMEM;
            return NULL;
        }
        size_t new_capacity = v->capacity == 0 ? 1 :
            (v->capacity > max_capacity / 2 ? max_capacity : v->capacity * 2);

        /* Record an internal source before realloc invalidates its address.
         * Integer comparisons avoid ordering unrelated C pointers.
         */
        uintptr_t source = (uintptr_t)elem;
        uintptr_t base = (uintptr_t)v->data;
        size_t bytes = v->len * v->elem_size;
        int internal = v->data != NULL && source >= base && source - base < bytes;
        size_t offset = internal ? (size_t)(source - base) : 0;
        if (internal && (offset % v->elem_size != 0)) {
            errno = EINVAL;
            return NULL;
        }
        if (!vec_reserve(v, new_capacity)) return NULL;
        if (internal) elem = v->data + offset;
    }
    void *dest = v->data + v->len * v->elem_size;
    memcpy(dest, elem, v->elem_size);
    v->len++;
    return dest;
}

void vec_pop(Vec *v)
{
    if (v->len > 0) v->len--;
}

int vec_shrink(Vec *v)
{
    if (v->len == 0) {
        free(v->data);
        v->data = NULL;
        v->capacity = 0;
        return 1;
    }
    if (v->len == v->capacity) return 1;
    void *tmp = realloc(v->data, v->len * v->elem_size);
    if (!tmp) {
        errno = ENOMEM;
        return 0;
    }
    v->data = tmp;
    v->capacity = v->len;
    return 1;
}

void *vec_at(Vec *v, size_t i)
{
    if (i >= v->len || v->data == NULL) return NULL;
    return v->data + i * v->elem_size;
}

void vec_destroy(Vec *v)
{
    free(v->data);
    *v = (Vec){0};
}

void vec_clear(Vec *v)
{
    v->len = 0;
}

void *vec_back(Vec *v)
{
    if (v->len == 0) return NULL;
    return vec_at(v, v->len - 1);
}

int vec_remove(Vec *v, size_t i)
{
    if (i >= v->len) return 0;
    if (i < v->len - 1) {
        memmove(v->data + i * v->elem_size,
                v->data + (i + 1) * v->elem_size,
                (v->len - i - 1) * v->elem_size);
    }
    v->len--;
    return 1;
}

void *vec_begin(Vec *v)
{
    return v->data;
}

void *vec_end(Vec *v)
{
    if (v->data == NULL) return NULL;
    return v->data + v->len * v->elem_size;
}
