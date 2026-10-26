# Changelog

## Unreleased

### Breaking changes

- Remove method pointers and the `LITE` build configuration. Use `vec_*` functions
  and recompile all consumers.
- Return success/failure from `vec_shrink`.
- Limit public header dependencies to `<stddef.h>`.
- Explicitly reject zero element sizes and NULL push sources with `EINVAL`.

### Fixes

- Preserve an internal push source across buffer reallocation.
- Remove library stderr output and preserve documented allocation error reporting.
- Allow growth up to the maximum representable element capacity.
- Build tests against the local library without requiring installation.

### Validation and documentation

- Add self-push, invalid-input, and simulated allocation-failure tests.
- Add sanitizer target and Linux/macOS CI using GCC/Clang compiler commands.
- Replace method-style examples, document ownership and pointer invalidation,
  and provide a stdin-based CLI example.
