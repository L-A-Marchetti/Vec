# Vec

[![C tests](https://github.com/L-A-Marchetti/Vec/actions/workflows/ci.yml/badge.svg)](https://github.com/L-A-Marchetti/Vec/actions/workflows/ci.yml)

**A small generic dynamic array for C11. Two files, ordinary functions, explicit allocation errors.**

Copy [`vec.c`](vec.c) and [`vec.h`](vec.h) into your project. Vec stores fixed-size
elements contiguously, grows geometrically, and leaves existing contents intact
when allocation fails. It has no dependencies beyond the C standard library.

## Why Vec?

- **Easy to inspect:** a compact implementation with a function-based API.
- **Recoverable errors:** allocation failures return to your application; the library never prints or exits.
- **Predictable storage:** contiguous elements, bounds-checked access, explicit reserve and shrink.
- **Simple integration:** compile two files alongside your own code; no installation or code generation required.

Vec uses runtime element sizes and shallow copies. It offers no compile-time type
checking or automatic element cleanup. Specialized typed code can be faster,
especially for small elements; see the [measured results](benchmarks/README.md).

## Quick start

Save this as `main.c` beside `vec.c` and `vec.h`:

```c
#include "vec.h"
#include <stdio.h>

typedef struct Point { double x, y; } Point;

int main(void)
{
    Vec points = vec_new(sizeof(Point));
    Point point = {1.0, 2.0};

    if (vec_push(&points, &point) == NULL) {
        vec_destroy(&points);
        return 1;
    }
    for (Point *it = vec_begin(&points); it != vec_end(&points); ++it) {
        printf("%.1f, %.1f\n", it->x, it->y);
    }
    vec_destroy(&points);
    return 0;
}
```

```sh
cc -std=c11 -O2 -Wall -Wextra main.c vec.c -o app
./app
```

For another element type, use `sizeof(T)` and pass a pointer to a value of that type.

## Example: summarize request records

The [example program](example.c) reads HTTP status/response-size pairs, stores
request structures in a Vec, then groups them by status:

```sh
make example
printf '200 512\n404 128\n200 1024\n' | ./build/example
```

```text
status requests bytes
200 2 1536
404 1 128
```

It demonstrates structure copies, checked insertion, iteration, and cleanup on
input or allocation errors. This example retains all records for demonstration;
a tool needing only these totals could aggregate them as it reads instead.

## Build and check

```sh
make                 # build/libvec.a
make test            # local library tests; no installation required
make test-example    # valid, malformed, and overflow inputs
make sanitize        # library + example under ASan/UBSan
make bench           # measured runs against a specialized typed baseline
```

Tests include self-append across growth, overflow guards, and simulated allocation
failures. CI runs library/example checks, sanitizers, and a benchmark correctness
smoke run on Linux and macOS. CI timings are not performance thresholds.

## Documentation

- [API and ownership](docs/API.md): all functions, errors, pointer invalidation, structures, and owned pointers.
- [Build and integration](docs/BUILDING.md): static linking, installation, compiler flags, and sanitizers.
- [Benchmarks](benchmarks/README.md): methodology, reproduction commands, raw results, and limitations.
- [Changelog](CHANGELOG.md).

MIT licensed; see [LICENSE](LICENSE).
