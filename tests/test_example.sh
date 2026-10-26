#!/bin/sh
set -eu
binary=${1:-./build/example}
expected='status requests bytes
200 2 1536
404 1 128'
actual=$(printf '200 512\n404 128\n200 1024\n' | "$binary")
[ "$actual" = "$expected" ]
[ "$(printf '' | "$binary")" = 'status requests bytes' ]
[ "$(printf '\n  \n200 0' | "$binary")" = 'status requests bytes
200 1 0' ]
for input in '99 1' '600 1' '200' '200 -1' '200 1 extra' \
    '200 18446744073709551616' '200 18446744073709551615\n200 1' \
    '200 +1' 'hello' '200 1\n404 garbage'; do
    if printf '%b\n' "$input" | "$binary" >/dev/null 2>&1; then
        printf 'Expected failure for: %s\n' "$input" >&2
        exit 1
    fi
done
if awk 'BEGIN { for (i = 0; i < 300; ++i) printf "1"; print "" }' |
    "$binary" >/dev/null 2>&1; then
    echo 'Expected oversized line to fail' >&2
    exit 1
fi
printf '[OK] example: valid, empty, malformed, and overflow inputs\n'
