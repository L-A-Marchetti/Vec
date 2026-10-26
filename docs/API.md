# API reference

## Type and functions

```c
typedef struct Vec {
    unsigned char *data;
    size_t len;
    size_t capacity;
    size_t elem_size;
} Vec;
```

| Function | Behavior |
| --- | --- |
| `Vec vec_new(size_t elem_size)` | Create an empty vector; size must be nonzero. |
| `void *vec_push(Vec *v, const void *elem)` | Copy one element; return its address or `NULL` on failure. |
| `void vec_pop(Vec *v)` | Remove the last element; no-op when empty. |
| `int vec_reserve(Vec *v, size_t capacity)` | Ensure at least this capacity; return 1 on success, 0 on failure. |
| `int vec_shrink(Vec *v)` | Reduce capacity to length; return 1 on success, 0 on failure. Free the buffer when empty. |
| `void vec_clear(Vec *v)` | Set length to zero, retaining capacity. |
| `void *vec_at(Vec *v, size_t i)` | Return an element address, or `NULL` if out of bounds. |
| `void *vec_back(Vec *v)` | Return the last element address, or `NULL` when empty. |
| `int vec_remove(Vec *v, size_t i)` | Remove and shift the tail; return 1, or 0 if out of bounds. |
| `void *vec_begin(Vec *v)` | Return the start of the buffer. |
| `void *vec_end(Vec *v)` | Return one past the last element; never dereference it. |
| `void vec_destroy(Vec *v)` | Free the buffer and zero all fields; safe to repeat. |

`push` is amortized O(1), `remove` is O(n − i), and reallocations may copy O(n)
elements. Access, pop, clear, and iteration endpoints are O(1).

## Contract and limitations

- Pass a valid `Vec *` to every operation. Initialize with `vec_new(sizeof(T))`.
  Do not modify its fields directly; they are exposed for reading and iteration.
- `vec_new(0)` returns a zero vector and sets `errno = EINVAL`. Push and reserve
  reject a zero element size; push also rejects a `NULL` source, with `EINVAL`.
- Allocation failure or capacity overflow returns `NULL`/0 and sets
  `errno = ENOMEM`. The vector remains unchanged. Inspect `errno` only after
  failure; successful operations do not promise to reset it.
- Push reads exactly `elem_size` bytes from its source. Supply a complete,
  initialized element of the correct type. There is no compile-time type checking.
- `vec_push(&v, vec_at(&v, i))` supports copying an existing element, even when
  growth moves the allocation. Arbitrary partial/overlapping sources are not supported.
- Pointers into the buffer may become invalid after a growing push, reserve,
  shrink, or destroy. Remove shifts elements; clear/pop end their logical lifetime.
- Empty vectors have equal begin/end pointers, which may be `NULL`. Do not
  dereference them or subtract NULL endpoints.
- Elements are shallow byte copies. If they contain pointers or other resources,
  the caller manages those resources. Clear, pop, remove, and destroy do not
  invoke element destructors.
- A `Vec` owns its buffer. Assignment copies that ownership without duplicating
  storage: do not independently use/destroy both copies. No clone API is provided.
- After destroy, reinitialize with `vec_new(sizeof(T))` before reuse.
- Storage uses `realloc`, suitable for ordinary types with fundamental alignment.
  Over-aligned types and elements requiring a stable address are not supported.
- Concurrent mutation/access of the same vector requires external synchronization.


## Structures

```c
struct Point { double x, y; };
Vec points = vec_new(sizeof(struct Point));
struct Point point = {1.0, 2.0};
if (vec_push(&points, &point) == NULL) {
    /* Handle allocation failure. */
}
/* point was copied; changing it does not modify points. */
vec_destroy(&points);
```

## Pointers and ownership

```c
#include "vec.h"
#include <stdlib.h>
#include <string.h>

int main(void)
{
    Vec strings = vec_new(sizeof(char *));
    char *text = malloc(sizeof "hello");
    if (text == NULL) return 1;
    memcpy(text, "hello", sizeof "hello");
    if (vec_push(&strings, &text) == NULL) {
        free(text);
        vec_destroy(&strings);
        return 1;
    }
    /* Only the pointer was copied. Free each allocation exactly once. */
    for (char **it = vec_begin(&strings); it != vec_end(&strings); ++it) {
        free(*it);
    }
    vec_destroy(&strings);
    return 0;
}
```

For a vector of pointers, pass the address of the pointer to `vec_push`, not the
pointer itself. Free owned resources before removing/clearing elements. Pushing
the same owning pointer twice requires a separate ownership strategy to avoid
a double free.

## Error handling

`vec_new` does not allocate. For a valid element size, construction cannot fail.
`vec_reserve` and `vec_shrink` return 1 on success and 0 on allocation failure;
`vec_push` returns an inserted-slot pointer or NULL. Check their return values
before using the result. On failure the existing data, length, and capacity are
preserved. A failed shrink is harmless if retaining the extra capacity is
acceptable to your application.

Out-of-range access/removal and empty pop are ordinary outcomes, not errno-based
errors. No operation prints diagnostics. NULL vector pointers and corrupted or
uninitialized vector structures are outside the API contract.

## Pointer invalidation example

```c
int *first = vec_at(&values, 0);
/* first may become invalid when the buffer grows. */
if (vec_push(&values, &next_value) == NULL) {
    /* Handle failure; previously valid pointers remain valid. */
}
first = vec_at(&values, 0); /* Reacquire after successful mutation. */
```

Reserve sufficient capacity before taking pointers if you need them to survive
subsequent pushes within that capacity. Removing an earlier element changes
which value lives at an address even when no allocation occurs.
