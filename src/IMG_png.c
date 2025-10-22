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
#if !defined(SAVE_PNG)
#define SAVE_PNG 1
#endif

#if defined(LOAD_PNG) && defined(USE_STBIMAGE)

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
    return SDL_LoadPNG_IO(src, false);
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
    SDL_SetError("SDL_image built without PNG support");
    return NULL;
}

#endif /* LOAD_PNG */

#if SAVE_PNG

bool IMG_SavePNG_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    return SDL_SavePNG_IO(surface, dst, closeio);
}

bool IMG_SavePNG(SDL_Surface *surface, const char *file)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return IMG_SavePNG_IO(surface, dst, true);
    } else {
        return false;
    }
}

#else // !SAVE_PNG

bool IMG_SavePNG_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    return SDL_SetError("SDL_image built without PNG save support");
}

bool IMG_SavePNG(SDL_Surface *surface, const char *file)
{
    (void)surface;
    (void)file;
    return SDL_SetError("SDL_image built without PNG save support");
}

#endif // SAVE_PNG

#endif /* SDL_IMAGE_LIBPNG */
