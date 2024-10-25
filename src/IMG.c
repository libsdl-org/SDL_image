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

/* A simple library to load images of various formats as SDL surfaces */

#include <SDL3_image/SDL_image.h>
#include "IMG.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#if defined(SDL_BUILD_MAJOR_VERSION)
SDL_COMPILE_TIME_ASSERT(SDL_BUILD_MAJOR_VERSION,
                        SDL_IMAGE_MAJOR_VERSION == SDL_BUILD_MAJOR_VERSION);
SDL_COMPILE_TIME_ASSERT(SDL_BUILD_MINOR_VERSION,
                        SDL_IMAGE_MINOR_VERSION == SDL_BUILD_MINOR_VERSION);
SDL_COMPILE_TIME_ASSERT(SDL_BUILD_MICRO_VERSION,
                        SDL_IMAGE_MICRO_VERSION == SDL_BUILD_MICRO_VERSION);
#endif

/* Limited by its encoding in SDL_VERSIONNUM */
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MAJOR_VERSION_min, SDL_IMAGE_MAJOR_VERSION >= 0);
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MAJOR_VERSION_max, SDL_IMAGE_MAJOR_VERSION <= 10);
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MINOR_VERSION_min, SDL_IMAGE_MINOR_VERSION >= 0);
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MINOR_VERSION_max, SDL_IMAGE_MINOR_VERSION <= 999);
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MICRO_VERSION_min, SDL_IMAGE_MICRO_VERSION >= 0);
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MICRO_VERSION_max, SDL_IMAGE_MICRO_VERSION <= 999);

/* Table of image detection and loading functions */
static struct {
    const char *type;
    bool (SDLCALL *is)(SDL_IOStream *src);
    SDL_Surface *(SDLCALL *load)(SDL_IOStream *src);
} supported[] = {
    /* keep magicless formats first */
    { "TGA", NULL,      IMG_LoadTGA_IO },
    { "AVIF",IMG_isAVIF,IMG_LoadAVIF_IO },
    { "CUR", IMG_isCUR, IMG_LoadCUR_IO },
    { "ICO", IMG_isICO, IMG_LoadICO_IO },
    { "BMP", IMG_isBMP, IMG_LoadBMP_IO },
    { "GIF", IMG_isGIF, IMG_LoadGIF_IO },
    { "JPG", IMG_isJPG, IMG_LoadJPG_IO },
    { "JXL", IMG_isJXL, IMG_LoadJXL_IO },
    { "LBM", IMG_isLBM, IMG_LoadLBM_IO },
    { "PCX", IMG_isPCX, IMG_LoadPCX_IO },
    { "PNG", IMG_isPNG, IMG_LoadPNG_IO },
    { "PNM", IMG_isPNM, IMG_LoadPNM_IO }, /* P[BGP]M share code */
    { "SVG", IMG_isSVG, IMG_LoadSVG_IO },
    { "TIF", IMG_isTIF, IMG_LoadTIF_IO },
    { "XCF", IMG_isXCF, IMG_LoadXCF_IO },
    { "XPM", IMG_isXPM, IMG_LoadXPM_IO },
    { "XV",  IMG_isXV,  IMG_LoadXV_IO  },
    { "WEBP", IMG_isWEBP, IMG_LoadWEBP_IO },
    { "QOI", IMG_isQOI, IMG_LoadQOI_IO },
};

/* Table of animation detection and loading functions */
static struct {
    const char *type;
    bool (SDLCALL *is)(SDL_IOStream *src);
    IMG_Animation *(SDLCALL *load)(SDL_IOStream *src);
} supported_anims[] = {
    /* keep magicless formats first */
    { "GIF", IMG_isGIF, IMG_LoadGIFAnimation_IO },
    { "WEBP", IMG_isWEBP, IMG_LoadWEBPAnimation_IO },
};

int IMG_Version(void)
{
    return SDL_IMAGE_VERSION;
}

static IMG_InitFlags initialized = 0;

IMG_InitFlags IMG_Init(IMG_InitFlags flags)
{
    IMG_InitFlags result = 0;

    if (flags & IMG_INIT_AVIF) {
        if ((initialized & IMG_INIT_AVIF) || IMG_InitAVIF() == 0) {
            result |= IMG_INIT_AVIF;
        }
    }
    if (flags & IMG_INIT_JPG) {
        if ((initialized & IMG_INIT_JPG) || IMG_InitJPG() == 0) {
            result |= IMG_INIT_JPG;
        }
    }
    if (flags & IMG_INIT_JXL) {
        if ((initialized & IMG_INIT_JXL) || IMG_InitJXL() == 0) {
            result |= IMG_INIT_JXL;
        }
    }
    if (flags & IMG_INIT_PNG) {
        if ((initialized & IMG_INIT_PNG) || IMG_InitPNG() == 0) {
            result |= IMG_INIT_PNG;
        }
    }
    if (flags & IMG_INIT_TIF) {
        if ((initialized & IMG_INIT_TIF) || IMG_InitTIF() == 0) {
            result |= IMG_INIT_TIF;
        }
    }
    if (flags & IMG_INIT_WEBP) {
        if ((initialized & IMG_INIT_WEBP) || IMG_InitWEBP() == 0) {
            result |= IMG_INIT_WEBP;
        }
    }
    initialized |= result;

    return initialized;
}

void IMG_Quit(void)
{
    if (initialized & IMG_INIT_AVIF) {
        IMG_QuitAVIF();
    }
    if (initialized & IMG_INIT_JPG) {
        IMG_QuitJPG();
    }
    if (initialized & IMG_INIT_JXL) {
        IMG_QuitJXL();
    }
    if (initialized & IMG_INIT_PNG) {
        IMG_QuitPNG();
    }
    if (initialized & IMG_INIT_TIF) {
        IMG_QuitTIF();
    }
    if (initialized & IMG_INIT_WEBP) {
        IMG_QuitWEBP();
    }
    initialized = 0;
}

#if !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND)
/* Load an image from a file */
SDL_Surface *IMG_Load(const char *file)
{
#ifdef __EMSCRIPTEN__
    int w, h;
    char *data;
    SDL_Surface *surf;

    data = emscripten_get_preloaded_image_data(file, &w, &h);
    if (data != NULL) {
        surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ABGR8888);
        if (surf != NULL) {
            SDL_memcpy(surf->pixels, data, w * h * 4);
        }
        free(data); /* This should NOT be SDL_free() */
        return surf;
    }
#endif

    SDL_IOStream *src = SDL_IOFromFile(file, "rb");
    const char *ext = SDL_strrchr(file, '.');
    if (ext) {
        ext++;
    }
    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }
    return IMG_LoadTyped_IO(src, true, ext);
}
#endif

/* Load an image from an SDL datasource (for compatibility) */
SDL_Surface *IMG_Load_IO(SDL_IOStream *src, bool closeio)
{
    return IMG_LoadTyped_IO(src, closeio, NULL);
}

/* Load an image from an SDL datasource, optionally specifying the type */
SDL_Surface *IMG_LoadTyped_IO(SDL_IOStream *src, bool closeio, const char *type)
{
    size_t i;
    SDL_Surface *image;

    /* Make sure there is something to do.. */
    if ( src == NULL ) {
        SDL_SetError("Passed a NULL data source");
        return NULL;
    }

    /* See whether or not this data source can handle seeking */
    if (SDL_SeekIO(src, 0, SDL_IO_SEEK_CUR) < 0 ) {
        SDL_SetError("Can't seek in this data source");
        if (closeio)
            SDL_CloseIO(src);
        return NULL;
    }

#ifdef __EMSCRIPTEN__
    /*load through preloadedImages*/
    FILE *fp = (FILE *)SDL_GetPointerProperty(SDL_GetIOProperties(src), SDL_PROP_IOSTREAM_STDIO_FILE_POINTER, NULL);
    if (fp) {
        int w, h, success;
        char *data;
        SDL_Surface *surf;

        data = emscripten_get_preloaded_image_data_from_FILE(fp, &w, &h);
        if (data) {
            surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ABGR8888);
            if (surf != NULL) {
                SDL_memcpy(surf->pixels, data, w * h * 4);
            }
            free(data); /* This should NOT be SDL_free() */

            if (closeio)
                SDL_CloseIO(src);

            /* If SDL_CreateSurface returns NULL, it has set the error message for us */
            return surf;
        }
    }
#endif

    /* Detect the type of image being loaded */
    for ( i=0; i < SDL_arraysize(supported); ++i ) {
        if (supported[i].is) {
            if (!supported[i].is(src))
                continue;
        } else {
            /* magicless format */
            if (!type || SDL_strcasecmp(type, supported[i].type) != 0)
                continue;
        }
#ifdef DEBUG_IMGLIB
        fprintf(stderr, "IMGLIB: Loading image as %s\n",
            supported[i].type);
#endif
        image = supported[i].load(src);
        if (closeio)
            SDL_CloseIO(src);
        return image;
    }

    if ( closeio ) {
        SDL_CloseIO(src);
    }
    SDL_SetError("Unsupported image format");
    return NULL;
}

SDL_Texture *IMG_LoadTexture(SDL_Renderer *renderer, const char *file)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_Load(file);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    }
    return texture;
}

SDL_Texture *IMG_LoadTexture_IO(SDL_Renderer *renderer, SDL_IOStream *src, bool closeio)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_Load_IO(src, closeio);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    }
    return texture;
}

SDL_Texture *IMG_LoadTextureTyped_IO(SDL_Renderer *renderer, SDL_IOStream *src, bool closeio, const char *type)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_LoadTyped_IO(src, closeio, type);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    }
    return texture;
}

/* Load an animation from a file */
IMG_Animation *IMG_LoadAnimation(const char *file)
{
    SDL_IOStream *src = SDL_IOFromFile(file, "rb");
    const char *ext = SDL_strrchr(file, '.');
    if (ext) {
        ext++;
    }
    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }
    return IMG_LoadAnimationTyped_IO(src, true, ext);
}

/* Load an animation from an SDL datasource (for compatibility) */
IMG_Animation *IMG_LoadAnimation_IO(SDL_IOStream *src, bool closeio)
{
    return IMG_LoadAnimationTyped_IO(src, closeio, NULL);
}

/* Load an animation from an SDL datasource, optionally specifying the type */
IMG_Animation *IMG_LoadAnimationTyped_IO(SDL_IOStream *src, bool closeio, const char *type)
{
    size_t i;
    IMG_Animation *anim;
    SDL_Surface *image;

    /* Make sure there is something to do.. */
    if ( src == NULL ) {
        SDL_SetError("Passed a NULL data source");
        return NULL;
    }

    /* See whether or not this data source can handle seeking */
    if (SDL_SeekIO(src, 0, SDL_IO_SEEK_CUR) < 0 ) {
        SDL_SetError("Can't seek in this data source");
        if (closeio)
            SDL_CloseIO(src);
        return NULL;
    }

    /* Detect the type of image being loaded */
    for ( i=0; i < SDL_arraysize(supported_anims); ++i ) {
        if (supported_anims[i].is) {
            if (!supported_anims[i].is(src))
                continue;
        } else {
            /* magicless format */
            if (!type || SDL_strcasecmp(type, supported_anims[i].type) != 0)
                continue;
        }
#ifdef DEBUG_IMGLIB
        fprintf(stderr, "IMGLIB: Loading image as %s\n",
            supported_anims[i].type);
#endif
        anim = supported_anims[i].load(src);
        if (closeio)
            SDL_CloseIO(src);
        return anim;
    }

    /* Create a single frame animation from an image */
    image = IMG_LoadTyped_IO(src, closeio, type);
    if (image) {
        anim = (IMG_Animation *)SDL_malloc(sizeof(*anim));
        if (anim) {
            anim->w = image->w;
            anim->h = image->h;
            anim->count = 1;

            anim->frames = (SDL_Surface **)SDL_calloc(anim->count, sizeof(*anim->frames));
            anim->delays = (int *)SDL_calloc(anim->count, sizeof(*anim->delays));

            if (anim->frames && anim->delays) {
                anim->frames[0] = image;
                return anim;
            }
            IMG_FreeAnimation(anim);
        }
        SDL_DestroySurface(image);
    }
    return NULL;
}

void IMG_FreeAnimation(IMG_Animation *anim)
{
    if (anim) {
        if (anim->frames) {
            int i;
            for (i = 0; i < anim->count; ++i) {
                if (anim->frames[i]) {
                    SDL_DestroySurface(anim->frames[i]);
                }
            }
            SDL_free(anim->frames);
        }
        if (anim->delays) {
            SDL_free(anim->delays);
        }
        SDL_free(anim);
    }
}

