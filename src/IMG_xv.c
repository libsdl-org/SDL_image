/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

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

/* This is a XV thumbnail image file loading framework */

#include <SDL3_image/SDL_image.h>
#include "IMG.h"

#ifdef LOAD_XV

static int get_line(SDL_IOStream *src, char *line, int size)
{
    while ( size > 0 ) {
        if (SDL_ReadIO(src, line, 1) != 1 ) {
            return -1;
        }
        if ( *line == '\r' ) {
            continue;
        }
        if ( *line == '\n' ) {
            *line = '\0';
            return 0;
        }
        ++line;
        --size;
    }
    /* Out of space for the line */
    return -1;
}

static int get_header(SDL_IOStream *src, int *w, int *h)
{
    char line[1024];

    *w = 0;
    *h = 0;

    /* Check the header magic */
    if ( (get_line(src, line, sizeof(line)) < 0) ||
         (SDL_memcmp(line, "P7 332", 6) != 0) ) {
        return -1;
    }

    /* Read the header */
    while ( get_line(src, line, sizeof(line)) == 0 ) {
        if ( SDL_memcmp(line, "#BUILTIN:", 9) == 0 ) {
            /* Builtin image, no data */
            break;
        }
        if ( SDL_memcmp(line, "#END_OF_COMMENTS", 16) == 0 ) {
            if ( get_line(src, line, sizeof(line)) == 0 ) {
                SDL_sscanf(line, "%d %d", w, h);
                if ( *w >= 0 && *h >= 0 ) {
                    return 0;
                }
            }
            break;
        }
    }
    /* No image data */
    return -1;
}

/* See if an image is contained in a data source */
SDL_bool IMG_isXV(SDL_IOStream *src)
{
    Sint64 start;
    SDL_bool is_XV;
    int w, h;

    if (!src) {
        return SDL_FALSE;
    }

    start = SDL_TellIO(src);
    is_XV = SDL_FALSE;
    if ( get_header(src, &w, &h) == 0 ) {
        is_XV = SDL_TRUE;
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_XV;
}

/* Load a XV thumbnail image from an SDL datasource */
SDL_Surface *IMG_LoadXV_IO(SDL_IOStream *src)
{
    Sint64 start;
    const char *error = NULL;
    SDL_Surface *surface = NULL;
    int w, h;
    Uint8 *pixels;

    if ( !src ) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }
    start = SDL_TellIO(src);

    /* Read the header */
    if ( get_header(src, &w, &h) < 0 ) {
        error = "Unsupported image format";
        goto done;
    }

    /* Create the 3-3-2 indexed palette surface */
    surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGB332);
    if ( surface == NULL ) {
        error = "Out of memory";
        goto done;
    }

    /* Load the image data */
    for ( pixels = (Uint8 *)surface->pixels; h > 0; --h ) {
        if (SDL_ReadIO(src, pixels, w) != (size_t)w ) {
            error = "Couldn't read image data";
            goto done;
        }
        pixels += surface->pitch;
    }

done:
    if ( error ) {
        SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
        if ( surface ) {
            SDL_DestroySurface(surface);
            surface = NULL;
        }
        SDL_SetError("%s", error);
    }
    return surface;
}

#else
#if defined(_MSC_VER) && _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
SDL_bool IMG_isXV(SDL_IOStream *src)
{
    return SDL_FALSE;
}

/* Load a XXX type image from an SDL datasource */
SDL_Surface *IMG_LoadXV_IO(SDL_IOStream *src)
{
    return NULL;
}

#endif /* LOAD_XV */
