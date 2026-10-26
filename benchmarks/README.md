# Benchmarks

These benchmarks measure specific Vec operations against a small typed-array
implementation in the same harness. They are not a ranking of C container
libraries or a promise of application performance.

## Reproduce

From the repository root:

```sh
make CC=gcc BUILD_DIR=build/bench-release \
    CFLAGS='-std=c11 -O2 -Wall -Wextra -Werror -pedantic' \
    build/bench-release/bench_vec
./build/bench-release/bench_vec 1000000 7 > results.csv
```

Or use `make bench BENCH_ARGS='1000000 7'` for a quick run with your current build.
The program accepts 1–10,000,000 elements and 1–99 measured trials. Large runs can
consume hundreds of megabytes. Use a separate build directory when changing
flags; do not compare sanitized timings with optimized timings.

CSV output contains minimum, median, and maximum milliseconds, input sizes,
trial counts, and a verified checksum. Run the binary directly when redirecting
CSV, so make's command output is not mixed into it.

## What is timed

| Case | Included | Excluded |
| --- | --- | --- |
| `push_growth` | Allocation, geometric growth, input generation, N checked pushes | Validation and destruction |
| `push_reserved` | One upfront allocation, input generation, N checked pushes | Validation and destruction |
| `scan_16_passes` | 16 direct typed traversals, summing the value field | Construction, validation, destruction |

Two element types are measured: `uint32_t` (4 bytes on the recorded platform) and
a 64-byte structure with a value and a filled payload. All element bytes and the
length are checked after each trial. Checksums are consumed through a volatile
sink, including between scan passes. Both implementations use the same inputs.

Each case runs one unrecorded warm-up per implementation, then the requested
number of measured trials. Implementation order alternates between trials.
The harness uses standard C `clock()` process CPU time; it does not measure wall
latency. Very small inputs can fall below timer resolution. Repeated traversals
favor warmed caches; this is not a cold-memory bandwidth test. Scan rows report
the entire 16 passes, not one pass; the CSV checksum describes one buffer.

The typed baseline stores a concrete type with a length and capacity. It uses
`realloc` and doubles capacity on growth; its reserved case uses `malloc`.
Allocation/push failure checks are present, but it is a benchmark implementation,
not a replacement public API with all Vec's contracts. Typed insertion is visible
to the optimizer in the benchmark translation unit; Vec is compiled separately
without link-time optimization. That deliberately measures the cost of the
ordinary generic API relative to specialized code, including function calls,
runtime copy sizes, and compiler optimization opportunities. Traversal uses typed
pointers for both implementations; it does not benchmark repeated `vec_at` calls.

## Recorded run

Environment reported by the execution environment:

- Linux x86_64, kernel `7.0.0-38-generic`.
- Intel Core i7-4770HQ CPU @ 2.20 GHz, 8 logical CPUs reported.
- GCC 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1).
- `-std=c11 -O2 -Wall -Wextra -Werror -pedantic`, no LTO or sanitizers.
- 1,000,000 elements, 7 measured trials plus warm-up; CPU affinity and frequency
  were not pinned. Executed in the development environment, not a dedicated host.

Raw data: [results-linux-gcc13.csv](results-linux-gcc13.csv).

Median CPU time in milliseconds:

| Elements | Operation | Vec | Typed baseline |
| --- | --- | ---: | ---: |
| uint32_t | Push with growth | 4.352 | 0.418 |
| uint32_t | Reserve + push | 4.184 | 0.419 |
| uint32_t | 16 scans | 7.017 | 7.067 |
| 64-byte record | Push with growth | 38.528 | 33.411 |
| 64-byte record | Reserve + push | 37.486 | 32.024 |
| 64-byte record | 16 scans | 56.769 | 56.135 |

In this run, small-element pushes through Vec cost about 10 times as much as the
specialized baseline. The 64-byte insertion gap is about 15–17%, and direct
traversal is close. These results expose a real tradeoff of the generic API;
Vec should not be chosen on an unqualified claim of speed. Reserving did not
produce a large improvement in this workload; allocator behavior and available
capacity matter, so reserving is not a universal speedup.

## Limits

This is one run on one environment. CPU scheduling, allocator state, compiler,
optimization flags, cache state, and input size can change the result. Minimum
and maximum values are provided to show within-run variation, not confidence
intervals. No conclusions are drawn about other container libraries, other
machines, concurrent access, removal-heavy workloads, allocation-failure speed,
or application latency. CI only runs small correctness checks; it never enforces
these timings. Re-run with your own workload before making a performance choice.
