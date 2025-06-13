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

/* This is a generic "format not supported" image framework */

#include <SDL3_image/SDL_image.h>

#ifdef LOAD_XXX

/* See if an image is contained in a data source */
/* Remember to declare this procedure in IMG.h . */
bool IMG_isXXX(SDL_IOStream *src)
{
    int start;
    bool is_XXX;

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_XXX = false;

    /* Detect the image here */

    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_XXX;
}

/* Load an XXX type image from an SDL datasource */
/* Remember to declare this procedure in IMG.h . */
SDL_Surface *IMG_LoadXXX_IO(SDL_IOStream *src)
{
    int start;
    const char *error = NULL;
    SDL_Surface *surface = NULL;

    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }

    start = SDL_TellIO(src);

    /* Load the image here */

    if (error) {
        SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
        if (surface) {
            SDL_DestroySurface(surface);
            surface = NULL;
        }
        SDL_SetError("%s", error);
    }

    return surface;
}

#else

bool IMG_isXXX(SDL_IOStream *src)
{
    (void)src;
    return false;
}

SDL_Surface *IMG_LoadXXX_IO(SDL_IOStream *src)
{
    (void)src;
    return NULL;
}

#endif /* LOAD_XXX */
