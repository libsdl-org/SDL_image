## Summary

A malformed APNG file can trigger a heap buffer overflow in core APNG chunk parsing logic:

**Affected Version**: `release-3.4.2`
**File**: `src/IMG_libpng.c`
**Function**: `read_png_chunk(...)` via `IMG_CreateAPNGAnimationDecoder(...)`
**Fault line**: `1017` - header/data writes into an undersized heap buffer after 32-bit size wrap

This is reachable through normal public animation-decoder API usage, and was reproduced against an ASan-instrumented `SDL_image` build using a standalone consumer harness.

### Reachability reproduction path

The crash occurs through standard public API:

1. `IMG_CreateAnimationDecoder_IO` (`src/IMG_anim_decoder.c:138`)
2. `IMG_CreateAnimationDecoderWithProperties` (`src/IMG_anim_decoder.c:214`)
3. `IMG_CreateAPNGAnimationDecoder` (`src/IMG_libpng.c:1279`)
4. `read_png_chunk` (`src/IMG_libpng.c:1017`)

## Root Cause

In `read_png_chunk`, the code reads the attacker-controlled chunk length into `data_length`, then computes:

- `*chunk_size = sizeof(header) + *data_length + 4`

using `Uint32`.

A malformed APNG chunk can satisfy:

- `data_length` near `UINT32_MAX`
- 32-bit addition wraps to a very small `chunk_size`
- `SDL_malloc(*chunk_size)` returns a too-small heap buffer
- subsequent writes still use the original large logical chunk size

This causes an undersized allocation followed by out-of-bounds writes.

The later `chunk_length > SDL_MAX_SINT32` check in `IMG_CreateAPNGAnimationDecoder` happens only after `read_png_chunk` has already allocated and written through the wrapped size, so it does not prevent the memory corruption.

## Reproduction

### 1) Standalone minimal reproducer (`harness.c`)

```c
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <stdio.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s <input.png>\n", argv[0]);
    return 2;
  }

  if (!SDL_Init(0)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_IOStream* src = SDL_IOFromFile(argv[1], "rb");
  if (!src) {
    fprintf(stderr, "SDL_IOFromFile failed: %s\n", SDL_GetError());
    return 1;
  }

  IMG_AnimationDecoder* decoder = IMG_CreateAnimationDecoder_IO(src, true, "apng");
  if (!decoder) {
    fprintf(stderr, "IMG_CreateAnimationDecoder_IO failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  IMG_CloseAnimationDecoder(decoder);
  SDL_Quit();
  return 0;
}
```

This app only calls:

- `SDL_Init()`
- `SDL_IOFromFile()`
- `IMG_CreateAnimationDecoder_IO(..., "apng")`
- `IMG_CloseAnimationDecoder()`

### 2) Build against local ASan SDL3 / SDL_image builds

```bash
PKG_CONFIG_PATH=/tmp/SDL_image-asan-prefix/lib/pkgconfig:/tmp/SDL3-asan-prefix/lib/pkgconfig \
gcc -fsanitize=address -fno-omit-frame-pointer -g -O1 \
  harness.c \
  $(PKG_CONFIG_PATH=/tmp/SDL_image-asan-prefix/lib/pkgconfig:/tmp/SDL3-asan-prefix/lib/pkgconfig \
    pkg-config --cflags --libs sdl3-image sdl3) \
  -Wl,-rpath,/tmp/SDL_image-asan-prefix/lib \
  -Wl,-rpath,/tmp/SDL3-asan-prefix/lib \
  -o sdl_image_asan_harness
```

### 3) PoC file

The trigger file uses a PNG/APNG chunk with an oversized length field so that:

- `data_length` is attacker-controlled
- `chunk_size` wraps in 32-bit arithmetic
- the decoder allocates only a tiny heap buffer
- `read_png_chunk()` writes past the end immediately

### 4) Run

```bash
LD_LIBRARY_PATH=/tmp/SDL_image-asan-prefix/lib:/tmp/SDL3-asan-prefix/lib \
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
./sdl_image_asan_harness \
apng_chunk_wrap_trigger.png
```

### 5) Result

```text
=================================================================
==788075==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000270 at pc ...
WRITE of size 8 at 0x502000000270 thread T0
    #0 ... in memcpy
    #1 ... in read_png_chunk /home/zerovirus/workspace/SDL_image/src/IMG_libpng.c:1017
    #2 ... in IMG_CreateAPNGAnimationDecoder /home/zerovirus/workspace/SDL_image/src/IMG_libpng.c:1279
    #3 ... in IMG_CreateAnimationDecoderWithProperties /home/zerovirus/workspace/SDL_image/src/IMG_anim_decoder.c:214
    #4 ... in IMG_CreateAnimationDecoder_IO /home/zerovirus/workspace/SDL_image/src/IMG_anim_decoder.c:138
    #5 ... in main .../harness.c:51

0x502000000274 is located 0 bytes after 4-byte region [0x502000000270,0x502000000274)
allocated by thread T0 here:
    #0 ... in malloc
    #1 ... in SDL_malloc_REAL
    #2 ... in read_png_chunk /home/zerovirus/workspace/SDL_image/src/IMG_libpng.c:1013

SUMMARY: AddressSanitizer: heap-buffer-overflow ... in memcpy
==788075==ABORTING
```

ASan confirms a real heap-buffer-overflow in the live APNG decoder path, with the core frame at:

`read_png_chunk(...)` in `src/IMG_libpng.c:1017`

## Impact

- Type: Heap buffer overflow
- Security effect: process crash and attacker-controlled heap corruption on malformed APNG input
- Scope: applications using SDL_image's public animation-decoder API, not limited to examples or tests

The trigger is in core APNG parsing reached by public API on untrusted file input.
