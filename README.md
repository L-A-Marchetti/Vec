# Vec

A small generic dynamic array for C11, with ordinary functions and explicit
allocation errors. Copy `vec.c` and `vec.h` into your project; no dependencies
beyond the C standard library.

Vec stores shallow copies of fixed-size elements in contiguous memory. It grows
geometrically, provides bounds-checked access, and leaves the vector unchanged
when an allocation fails. It does not print errors or terminate your program.

## Quick start

```c
#include "vec.h"
#include <stdio.h>

int main(void)
{
    Vec values = vec_new(sizeof(int));
    for (int i = 1; i <= 5; ++i) {
        if (vec_push(&values, &i) == NULL) {
            perror("vec_push");
            vec_destroy(&values);
            return 1;
        }
    }
    for (int *it = vec_begin(&values); it != vec_end(&values); ++it) {
        printf("%d\n", *it);
    }
    vec_destroy(&values);
    return 0;
}
```

Compile alongside your application:

```sh
cc -std=c11 -O2 -Wall -Wextra -Ipath/to/Vec main.c path/to/Vec/vec.c -o app
```

The public header includes only `<stddef.h>`. Include the standard headers your
application uses, such as `<stdio.h>`, `<stdint.h>`, or `<errno.h>`.

## Build and test

```sh
make                 # build/libvec.a
make test            # test the local sources; no installation required
make sanitize        # AddressSanitizer + UndefinedBehaviorSanitizer
make example         # build the stdin example
printf '10 20 30\n' | ./build/example  # prints 20 and 30 on separate lines
```

Tests cover operations, self-append across growth, invalid element sizes,
overflow, and simulated allocation failures. CI runs tests and sanitizers on
Linux and macOS with GCC/Clang compiler commands (macOS gcc may be Apple Clang).

Optional installation:

```sh
make install PREFIX="$HOME/.local"
```

For an installed library, pass the appropriate include/library directories and
link with `-lvec` after your source/object files. `CC`, `CPPFLAGS`, `CFLAGS`,
`LDFLAGS`, `BUILD_DIR`, `PREFIX`, and `DESTDIR` can be overridden with make.
Use a clean or separate build directory when changing compiler flags.

## API

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

## Migrating from the method-pointer API

This is a breaking API/ABI change. Recompile the library and all consumers.

```c
/* Before */
v.push(&v, &item);
v.destroy(&v);

/* Now */
vec_push(&v, &item);
vec_destroy(&v);
```

Method pointers and the `LITE` configuration are removed. The structure now always
contains one data pointer and three sizes. `vec_shrink` now returns an `int` so
callers can check for allocation failure. The header no longer implicitly
includes unrelated standard headers.

## Scope

Choose Vec if you want a small, inspectable implementation with a function-based
API and recoverable allocation errors. A typed macro-based container can be a
better fit if compile-time type checking and terse element expressions matter
more to you. Vec aims to remain a focused dynamic array.

MIT licensed; see [LICENSE](LICENSE).
