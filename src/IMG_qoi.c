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

/* This file use QOI library:
 * https://github.com/phoboslab/qoi
 */

#include <SDL3_image/SDL_image.h>
#include <limits.h> /* for INT_MAX */

#ifdef LOAD_QOI

/* SDL < 2.0.12 compatibility */
#ifndef SDL_zeroa
#define SDL_zeroa(x) SDL_memset((x), 0, sizeof((x)))
#endif

#define QOI_MALLOC(sz) SDL_malloc(sz)
#define QOI_FREE(p)    SDL_free(p)
#define QOI_ZEROARR(a) SDL_zeroa(a)

#define QOI_NO_STDIO
#define QOI_IMPLEMENTATION
#include "qoi.h"

/* See if an image is contained in a data source */
bool IMG_isQOI(SDL_IOStream *src)
{
    Sint64 start;
    bool is_QOI;
    char magic[4];

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_QOI = false;
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic) ) {
        if ( SDL_strncmp(magic, "qoif", 4) == 0 ) {
            is_QOI = true;
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_QOI;
}

/* Load a QOI type image from an SDL datasource */
SDL_Surface *IMG_LoadQOI_IO(SDL_IOStream *src)
{
    void *data;
    size_t size;
    void *pixel_data;
    qoi_desc image_info;
    SDL_Surface *surface = NULL;

    data = (void *)SDL_LoadFile_IO(src, &size, false);
    if ( !data ) {
        return NULL;
    }
    if ( size > INT_MAX ) {
        SDL_free(data);
        SDL_SetError("QOI image is too big.");
        return NULL;
    }

    pixel_data = qoi_decode(data, (int)size, &image_info, 4);
    /* pixel_data is in R,G,B,A order regardless of endianness */
    SDL_free(data);
    if ( !pixel_data ) {
        SDL_SetError("Couldn't parse QOI image");
        return NULL;
    }

    surface = SDL_CreateSurfaceFrom(image_info.width,
                                    image_info.height,
                                    SDL_PIXELFORMAT_RGBA32,
                                    pixel_data,
                                    (image_info.width * 4));
    if ( !surface ) {
        QOI_FREE(pixel_data);
        SDL_SetError("Couldn't create SDL_Surface");
        return NULL;
    }

    /* Let SDL manage the memory now */
    surface->flags &= ~SDL_SURFACE_PREALLOCATED;

    return surface;
}

#else

/* See if an image is contained in a data source */
bool IMG_isQOI(SDL_IOStream *src)
{
    (void)src;
    return false;
}

/* Load a QOI type image from an SDL datasource */
SDL_Surface *IMG_LoadQOI_IO(SDL_IOStream *src)
{
    (void)src;
    return NULL;
}

#endif /* LOAD_QOI */
