#!/bin/sh

set -eu

CC="${CC:-gcc}"

"$CC" \
    -Wall \
    -Wextra \
    -D_XOPEN_SOURCE=700 \
    -std=c23 \
    -Iinclude \
    src/tomlc17/*.c \
    src/*.c \
    -o target/kart

echo "kart successfully compiled!"
