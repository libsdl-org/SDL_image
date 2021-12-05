/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_image.h"

#define LOAD_QOI

#ifdef LOAD_QOI

/* Replace C runtime functions with SDL C runtime functions for building on Windows */
#if defined(__WATCOMC__) && defined(HAVE_LIBC)
  /* With SDL math functions, Watcom builds are very much broken. */
#define acosf(x)    (float)acos((double)(x))
#define atan2f(x,y) (float)atan2((double)(x),(double)(y))
#define cosf(x)     (float)cos((double)(x))
#define ceilf(x)    (float)ceil((double)(x))
#define fabsf(x)    (float)fabs((double)(x))
#define floorf(x)   (float)floor((double)(x))
#define fmodf(x,y)  (float)fmod((double)(x),(double)(y))
#define sinf(x)     (float)sin((double)(x))
#define sqrtf(x)    (float)sqrt((double)(x))
#define tanf(x)     (float)tan((double)(x))
#else
#define acosf   SDL_acosf
#define atan2f  SDL_atan2f
#define cosf    SDL_cosf
#define ceilf   SDL_ceilf
#define fabs    SDL_fabs
#define fabsf   SDL_fabsf
#define floorf  SDL_floorf
#define fmodf   SDL_fmodf
#define pow     SDL_pow
#define sinf    SDL_sinf
#define sqrt    SDL_sqrt
#define sqrtf   SDL_sqrtf
#define tanf    SDL_tanf
#endif
#define free    SDL_free
#define malloc  SDL_malloc
#undef memcpy
#define memcpy  SDL_memcpy
#undef memset
#define memset  SDL_memset
#define qsort   SDL_qsort
#define realloc SDL_realloc
#define sscanf  SDL_sscanf
#undef strchr
#define strchr  SDL_strchr
#undef strcmp
#define strcmp  SDL_strcmp
#undef strncmp
#define strncmp SDL_strncmp
#undef strncpy
#define strncpy SDL_strlcpy
#define strlen  SDL_strlen
#define strstr  SDL_strstr
#define strtol  SDL_strtol
#define strtoll SDL_strtoll
#ifndef FLT_MAX
#define FLT_MAX     3.402823466e+38F
#endif
#undef HAVE_STDIO_H

#define QOI_IMPLEMENTATION
#include "qoi.h"

/* See if an image is contained in a data source */
int IMG_isQOI(SDL_RWops *src)
{
    Sint64 start;
    int is_QOI;
    char magic[4];

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_QOI = 0;
    if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
        if ( SDL_strncmp(magic, "qoif", 4) == 0 ) {
            is_QOI = 1;
        }
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_QOI);
}

/* Load a QOI type image from an SDL datasource */
SDL_Surface *IMG_LoadQOI_RW(SDL_RWops *src)
{
    void *data;
    size_t size;
    void *pixel_data;
    qoi_desc image_info;
    SDL_Surface *surface = NULL;

    data = (void *)SDL_LoadFile_RW(src, &size, SDL_FALSE);
    if ( !data ) {
        return NULL;
    }

    pixel_data = qoi_decode(data, size, &image_info, 4);
    SDL_free(data);
    if ( !pixel_data ) {
        IMG_SetError("Couldn't parse QOI image");
        return NULL;
    }

    surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                   image_info.width,
                                   image_info.height,
                                   32,
                                   0x000000FF,
                                   0x0000FF00,
                                   0x00FF0000,
                                   0xFF000000);
    if ( !surface ) {
        SDL_free(pixel_data);
        IMG_SetError("Couldn't create SDL_Surface");
        return NULL;
    }

    SDL_memcpy(surface->pixels, pixel_data, image_info.width*image_info.height*4);

    return surface;
}

#else
#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
int IMG_isSVG(SDL_RWops *src)
{
    return(0);
}

/* Load a SVG type image from an SDL datasource */
SDL_Surface *IMG_LoadSVG_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_SVG */
