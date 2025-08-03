/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* This is a PNG image file loading framework */

#include <SDL3_image/SDL_image.h>

#if !defined(SDL_IMAGE_LIBPNG)

/* We'll have PNG save support by default */
#if !defined(SDL_IMAGE_SAVE_PNG)
#define SDL_IMAGE_SAVE_PNG 1
#endif

/* We'll have PNG load support by default */
#if !defined(LOAD_PNG_DYNAMIC)
#define LOAD_PNG_DYNAMIC 1
#endif

#if defined(LOAD_PNG) && defined(USE_STBIMAGE)

extern SDL_Surface *IMG_LoadSTB_IO(SDL_IOStream *src);

/* FIXME: This is a copypaste from LIBPNG! Pull that out of the ifdefs */
/* See if an image is contained in a data source */
bool IMG_isPNG(SDL_IOStream *src)
{
    Sint64 start;
    bool is_PNG;
    Uint8 magic[4];

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_PNG = false;
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic)) {
        if (magic[0] == 0x89 &&
            magic[1] == 'P' &&
            magic[2] == 'N' &&
            magic[3] == 'G') {
            is_PNG = true;
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_PNG;
}

/* Load a PNG type image from an SDL datasource */
SDL_Surface *IMG_LoadPNG_IO(SDL_IOStream *src)
{
    return IMG_LoadSTB_IO(src);
}

#else

/* See if an image is contained in a data source */
bool IMG_isPNG(SDL_IOStream *src)
{
    (void)src;
    return false;
}

/* Load a PNG type image from an SDL datasource */
SDL_Surface *IMG_LoadPNG_IO(SDL_IOStream *src)
{
    (void)src;
    return NULL;
}

#endif /* LOAD_PNG */

#if SDL_IMAGE_SAVE_PNG

static const Uint32 png_format = SDL_PIXELFORMAT_RGBA32;

#if defined(LOAD_PNG_DYNAMIC)
/* Replace C runtime functions with SDL C runtime functions for building on Windows */
#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_SDL_MALLOC
#define MZ_ASSERT(x) SDL_assert(x)
#undef memcpy
#define memcpy SDL_memcpy
#undef memset
#define memset SDL_memset
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define MINIZ_LITTLE_ENDIAN 1
#else
#define MINIZ_LITTLE_ENDIAN 0
#endif
#define MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#define MINIZ_SDL_NOUNUSED
#include "miniz.h"

static bool IMG_SavePNG_IO_miniz(SDL_Surface *surface, SDL_IOStream *dst)
{
    size_t size = 0;
    void *png = NULL;
    bool result = false;

    if (!dst) {
        return SDL_SetError("Passed NULL dst");
    }

    if (surface->format == png_format) {
        png = tdefl_write_image_to_png_file_in_memory(surface->pixels, surface->w, surface->h, SDL_BYTESPERPIXEL(surface->format), surface->pitch, &size);
    } else {
        SDL_Surface *cvt = SDL_ConvertSurface(surface, png_format);
        if (cvt) {
            png = tdefl_write_image_to_png_file_in_memory(cvt->pixels, cvt->w, cvt->h, SDL_BYTESPERPIXEL(cvt->format), cvt->pitch, &size);
            SDL_DestroySurface(cvt);
        }
    }
    if (png) {
        if (SDL_WriteIO(dst, png, size)) {
            result = true;
        }
        mz_free(png); /* calls SDL_free() */
    } else {
        return SDL_SetError("Failed to convert and save image");
    }
    return result;
}
#endif /* LOAD_PNG_DYNAMIC || !WANT_LIBPNG */

#endif /* SDL_IMAGE_SAVE_PNG */

bool IMG_SavePNG(SDL_Surface *surface, const char *file)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return IMG_SavePNG_IO(surface, dst, 1);
    } else {
        return false;
    }
}

bool IMG_SavePNG_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    bool result = false;

    if (!dst) {
        return SDL_SetError("Passed NULL dst");
    }

#if SDL_IMAGE_SAVE_PNG

#if defined(LOAD_PNG_DYNAMIC)
    if (!result) {
        result = IMG_SavePNG_IO_miniz(surface, dst);
    }
#endif

#else
    result = SDL_SetError("SDL_image built without PNG save support");
#endif

    if (closeio) {
        SDL_CloseIO(dst);
    }
    return result;
}

#endif /* SDL_IMAGE_LIBPNG */
