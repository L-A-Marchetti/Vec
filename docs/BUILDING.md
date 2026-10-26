# Build and integration

## Compile with your sources

Vec requires C11 and the C standard library. Add `vec.c` to your build and make
`vec.h` available to the compiler:

```sh
cc -std=c11 -O2 -Wall -Wextra -Ipath/to/Vec main.c path/to/Vec/vec.c -o app
```

The header includes only `<stddef.h>`. Include headers such as `<stdio.h>`,
`<errno.h>`, and `<stdint.h>` yourself when using their declarations.

## Static library

```sh
make
cc -std=c11 -O2 -I. main.c build/libvec.a -o app
```

Put the library after sources/object files that reference its symbols.

Optional installation, with no root access required:

```sh
make install PREFIX="$HOME/.local"
cc -std=c11 -O2 -I"$HOME/.local/include" main.c \
    -L"$HOME/.local/lib" -lvec -o app
```

`DESTDIR` supports staged installations. The Makefile also accepts `CC`, `AR`,
`CPPFLAGS`, `CFLAGS`, `LDFLAGS`, and `BUILD_DIR`. Compiler flags are not tracked as
file dependencies: use `make clean` or a separate build directory after changes.
For example:

```sh
make CC=clang BUILD_DIR=build/clang test test-example
```

The library itself is standard C; the Makefile and shell tests assume a Unix-like
build environment. Windows builds are not currently covered by CI.

## Tests and sanitizers

```sh
make test test-example
make sanitize
```

`make test` uses the local static library and requires no prior installation.
Allocation-failure tests inject a failing realloc in a separate test translation
unit, without adding allocator hooks to the public API. Example tests require a
POSIX shell and awk.

`make sanitize` uses a separate `build/sanitize` directory (or a `sanitize`
subdirectory under your chosen BUILD_DIR), with AddressSanitizer and
UndefinedBehaviorSanitizer. It requires compiler/runtime support. LeakSanitizer
availability depends on the platform. In environments where process tracing
prevents leak detection, `ASAN_OPTIONS=detect_leaks=0 make sanitize` keeps address
and undefined-behavior checks enabled but does not check for leaks.

CI uses Linux/macOS and gcc/clang compiler commands; on macOS the gcc command may
resolve to Apple Clang. The benchmark smoke run validates output data, not speed.
