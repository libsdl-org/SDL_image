## Summary

A malformed APNG file can trigger use of uninitialized frame metadata in core frame iteration logic:

**Affected Version**: `release-3.4.2`
**File**: `src/IMG_libpng.c`
**Function**: `IMG_AnimationDecoderGetNextFrame_Internal(...)`
**Fault line**: `1116` - uses `ctx->fctl_frames[ctx->current_frame_index]` without validating that an `fcTL` entry was actually parsed for that frame

This is reachable through normal public animation-decoder API usage, and was reproduced against an ASan-instrumented `SDL_image` build using a standalone consumer harness.

### Reachability reproduction path

The crash occurs through standard public API:

1. `IMG_CreateAnimationDecoder_IO` (`src/IMG_anim_decoder.c:138`)
2. `IMG_CreateAnimationDecoderWithProperties` (`src/IMG_anim_decoder.c:214`)
3. `IMG_CreateAPNGAnimationDecoder` (`src/IMG_libpng.c:1314`, `1387`, `1502`)
4. `IMG_GetAnimationDecoderFrame` (`src/IMG_anim_decoder.c:278`)
5. `IMG_AnimationDecoderGetNextFrame_Internal` (`src/IMG_libpng.c:1116`)
6. `decompress_png_frame_data` (`src/IMG_libpng.c:1153`)

## Root Cause

During APNG parsing, the decoder accepts two independently controlled counts:

- `ctx->actl.num_frames` from the `acTL` chunk
- `ctx->fctl_count` from actual parsed `fcTL` chunks

A malformed file can satisfy:

- `acTL.num_frames > 1`
- only one `fcTL` chunk is provided
- `ctx->fctl_count == 1`
- `ctx->fctl_capacity` is still at least `4` after the first reallocation

The decoder validates only:

- `ctx->fctl_count != 0`

but never validates:

- `ctx->actl.num_frames <= ctx->fctl_count`

When the caller requests the next frame, `IMG_AnimationDecoderGetNextFrame_Internal` checks `ctx->actl.num_frames - ctx->current_frame_index < 1` and then directly uses `ctx->fctl_frames[ctx->current_frame_index]`.

In the malformed case, this accesses an allocated but uninitialized `fctl_frames` slot. Those garbage fields are then consumed as:

- frame width / height
- offsets
- raw compressed frame pointer
- raw compressed frame size

This leads to invalid decoder state and a crash during later processing.

## Reproduction

### 1) Standalone minimal reproducer (`harness.c`)

```c
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <stdio.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s <input.apng>\n", argv[0]);
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
    fprintf(stderr, "decoder creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  for (int i = 0; i < 2; ++i) {
    SDL_Surface* frame = NULL;
    Uint64 duration = 0;
    if (!IMG_GetAnimationDecoderFrame(decoder, &frame, &duration)) {
      break;
    }
    if (frame) {
      SDL_DestroySurface(frame);
    }
  }

  IMG_CloseAnimationDecoder(decoder);
  SDL_Quit();
  return 0;
}
```

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

The trigger file is constructed so that:

- `acTL.num_frames = 2`
- only one `fcTL` chunk exists
- `IDAT` exists for the first frame so initial decoding succeeds
- second frame request consumes an uninitialized `fctl_frames` slot

This does not immediately produce a clean out-of-bounds read because the vector capacity is larger than the parsed count, but it does produce attacker-triggerable misuse of uninitialized heap data.

### 4) Run

```bash
LD_LIBRARY_PATH=/tmp/SDL_image-asan-prefix/lib:/tmp/SDL3-asan-prefix/lib \
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
./sdl_image_asan_harness \
apng_missing_fctl_oob.apng
```

### 5) Result

```text
=================================================================
==788137==ERROR: AddressSanitizer: requested allocation size 0xbebebebebebec400 ...
    #0 ... in realloc
    #1 ... in SDL_realloc_REAL
    #2 ... in dynamic_mem_realloc
    #3 ... in dynamic_mem_write
    #4 ... in decompress_png_frame_data /home/zerovirus/workspace/SDL_image/src/IMG_libpng.c:851
    #5 ... in IMG_AnimationDecoderGetNextFrame_Internal /home/zerovirus/workspace/SDL_image/src/IMG_libpng.c:1153
    #6 ... in IMG_GetAnimationDecoderFrame /home/zerovirus/workspace/SDL_image/src/IMG_anim_decoder.c:278
    #7 ... in main .../harness.c:73

SUMMARY: AddressSanitizer: allocation-size-too-big ... in realloc
==788137==ABORTING
```

ASan shows that the malformed second-frame state leads to consumption of uninitialized metadata and an invalid oversized allocation path during frame decompression, with the core frame sequence through:

- `IMG_AnimationDecoderGetNextFrame_Internal`
- `decompress_png_frame_data`

## Impact

- Type: Uninitialized heap data use caused by frame-count mismatch
- Security effect: process crash / denial of service on attacker-controlled APNG input
- Scope: applications using SDL_image's public animation-decoder API, not limited to examples or tests

The trigger is in core APNG sequence parsing and frame iteration reached through public API on untrusted input.
