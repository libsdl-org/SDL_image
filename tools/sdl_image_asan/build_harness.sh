#!/usr/bin/env bash
set -eu

if [ $# -lt 1 ]; then
    echo "usage: $0 <output-binary> [extra-cflags...]" >&2
    echo "env: set PKG_CONFIG_PATH if SDL3/SDL3_image .pc files are not installed globally" >&2
    exit 2
fi

out="$1"
shift

cc="${CC:-gcc}"
cflags="$(pkg-config --cflags sdl3-image sdl3)"
libs="$(pkg-config --libs sdl3-image sdl3)"

exec "$cc" \
    -fsanitize=address \
    -fno-omit-frame-pointer \
    -g \
    -O1 \
    tools/sdl_image_asan/harness.c \
    $cflags \
    "$@" \
    $libs \
    -o "$out"
