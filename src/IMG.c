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

/* A simple library to load images of various formats as SDL surfaces */

#include <SDL3_image/SDL_image.h>

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
    { "GIF", IMG_isGIF, IMG_LoadGIFAnimation_IO     },
    { "WEBP", IMG_isWEBP, IMG_LoadWEBPAnimation_IO  },
    { "APNG", IMG_isPNG, IMG_LoadAPNGAnimation_IO   },
    { "AVIFS", IMG_isAVIF, IMG_LoadAVIFAnimation_IO },
    { "ANI", IMG_isANI, IMG_LoadANIAnimation_IO },
};

int IMG_Version(void)
{
    return SDL_IMAGE_VERSION;
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
        surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
        if (surf != NULL) {
            SDL_memcpy(surf->pixels, data, w * h * 4);
        }
        free(data); /* This should NOT be SDL_free() */
        return surf;
    }
#endif

    SDL_IOStream *src = SDL_IOFromFile(file, "rb");
    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }

    const char *ext = SDL_strrchr(file, '.');
    if (ext) {
        ext++;
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
    if (!src) {
        SDL_InvalidParamError("src");
        return NULL;
    }

    /* See whether or not this data source can handle seeking */
    if (SDL_SeekIO(src, 0, SDL_IO_SEEK_CUR) < 0) {
        SDL_SetError("Can't seek in this data source");
        if (closeio) {
            SDL_CloseIO(src);
        }
        return NULL;
    }

#ifdef __EMSCRIPTEN__
    /*load through preloadedImages*/
    FILE *fp = (FILE *)SDL_GetPointerProperty(SDL_GetIOProperties(src), SDL_PROP_IOSTREAM_STDIO_FILE_POINTER, NULL);
    if (fp) {
        int w, h;
        char *data;

        data = emscripten_get_preloaded_image_data_from_FILE(fp, &w, &h);
        if (data) {
            image = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
            if (image != NULL) {
                SDL_memcpy(image->pixels, data, w * h * 4);
            }
            free(data); /* This should NOT be SDL_free() */

            if (closeio) {
                SDL_CloseIO(src);
            }

            /* If SDL_CreateSurface returns NULL, it has set the error message for us */
            return image;
        }
    }
#endif

    /* Detect the type of image being loaded */
    for (i = 0; i < SDL_arraysize(supported); ++i) {
        if (supported[i].is) {
            if (!supported[i].is(src)) {
                continue;
            }
        } else {
            /* magicless format */
            if (!type || SDL_strcasecmp(type, supported[i].type) != 0) {
                continue;
            }
        }
#ifdef DEBUG_IMGLIB
        SDL_Log("IMGLIB: Loading image as %s\n", supported[i].type);
#endif
        image = supported[i].load(src);
        if (closeio) {
            SDL_CloseIO(src);
        }
        return image;
    }

    if (closeio) {
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
    if (!src) {
        SDL_InvalidParamError("src");
        return NULL;
    }

    /* See whether or not this data source can handle seeking */
    if (SDL_SeekIO(src, 0, SDL_IO_SEEK_CUR) < 0) {
        SDL_SetError("Can't seek in this data source");
        if (closeio) {
            SDL_CloseIO(src);
        }
        return NULL;
    }

    /* Detect the type of image being loaded */
    for (i = 0; i < SDL_arraysize(supported_anims); ++i) {
        if (supported_anims[i].is) {
            if (!supported_anims[i].is(src)) {
                continue;
            }
        } else {
            /* magicless format */
            if (!type || SDL_strcasecmp(type, supported_anims[i].type) != 0) {
                continue;
            }
        }
#ifdef DEBUG_IMGLIB
        SDL_Log("IMGLIB: Loading image as %s\n", supported_anims[i].type);
#endif
        anim = supported_anims[i].load(src);
        if (closeio) {
            SDL_CloseIO(src);
        }
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

bool IMG_Save(SDL_Surface *surface, const char *file)
{
    if (!surface) {
        return SDL_InvalidParamError("surface");
    }

    if (!file || !*file) {
        return SDL_InvalidParamError("file");
    }

    const char *type = SDL_strrchr(file, '.');
    if (type) {
        // Skip the '.' in the file extension
        ++type;
    } else {
        return SDL_SetError("Couldn't determine file type");
    }

    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (!dst) {
        return false;
    }

    return IMG_SaveTyped_IO(surface, dst, true, type);
}

bool IMG_SaveTyped_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio, const char *type)
{
    bool result = false;

    if (!surface) {
        SDL_InvalidParamError("surface");
        goto done;
    }

    if (!dst) {
        SDL_InvalidParamError("dst");
        goto done;
    }

    if (!type || !*type) {
        SDL_InvalidParamError("type");
        goto done;
    }

    if (SDL_strcasecmp(type, "avif") == 0) {
        result = IMG_SaveAVIF_IO(surface, dst, false, 90);
    } else if (SDL_strcasecmp(type, "bmp") == 0) {
        result = IMG_SaveBMP_IO(surface, dst, false);
    } else if (SDL_strcasecmp(type, "cur") == 0) {
        result = IMG_SaveCUR_IO(surface, dst, false);
    } else if (SDL_strcasecmp(type, "gif") == 0) {
        result = IMG_SaveGIF_IO(surface, dst, false);
    } else if (SDL_strcasecmp(type, "ico") == 0) {
        result = IMG_SaveICO_IO(surface, dst, false);
    } else if (SDL_strcasecmp(type, "jpg") == 0 ||
               SDL_strcasecmp(type, "jpeg") == 0) {
        result = IMG_SaveJPG_IO(surface, dst, false, 90);
    } else if (SDL_strcasecmp(type, "png") == 0) {
        result = IMG_SavePNG_IO(surface, dst, false);
    } else if (SDL_strcasecmp(type, "tga") == 0) {
        result = IMG_SaveTGA_IO(surface, dst, false);
    } else if (SDL_strcasecmp(type, "webp") == 0) {
        result = IMG_SaveWEBP_IO(surface, dst, false, 90.0f);
    } else {
        result = SDL_SetError("Unsupported image format");
    }

done:
    if (dst && closeio) {
        result &= SDL_CloseIO(dst);
    }
    return result;
}

bool IMG_SaveAnimation(IMG_Animation *anim, const char *file)
{
    if (!anim) {
        return SDL_InvalidParamError("anim");
    }

    if (!file || !*file) {
        return SDL_InvalidParamError("file");
    }

    const char *type = SDL_strrchr(file, '.');
    if (type) {
        // Skip the '.' in the file extension
        ++type;
    } else {
        return SDL_SetError("Couldn't determine file type");
    }

    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (!dst) {
        return false;
    }

    return IMG_SaveAnimationTyped_IO(anim, dst, true, type);
}

bool IMG_SaveAnimationTyped_IO(IMG_Animation *anim, SDL_IOStream *dst, bool closeio, const char *type)
{
    bool result = false;

    if (!anim) {
        SDL_InvalidParamError("anim");
        goto done;
    }

    if (!dst) {
        SDL_InvalidParamError("dst");
        goto done;
    }

    if (!type || !*type) {
        SDL_InvalidParamError("type");
        goto done;
    }

    if (SDL_strcasecmp(type, "ani") == 0) {
        result = IMG_SaveANIAnimation_IO(anim, dst, false);
    } else if (SDL_strcasecmp(type, "apng") == 0 || SDL_strcasecmp(type, "png") == 0) {
        result = IMG_SaveAPNGAnimation_IO(anim, dst, false);
    } else if (SDL_strcasecmp(type, "avif") == 0) {
        result = IMG_SaveAVIFAnimation_IO(anim, dst, false, 90);
    } else if (SDL_strcasecmp(type, "gif") == 0) {
        result = IMG_SaveGIFAnimation_IO(anim, dst, false);
    } else if (SDL_strcasecmp(type, "webp") == 0) {
        result = IMG_SaveWEBPAnimation_IO(anim, dst, false, 90);
    } else {
        result = SDL_SetError("Unsupported image format");
    }

done:
    if (dst && closeio) {
        result &= SDL_CloseIO(dst);
    }
    return result;
}

SDL_Surface *IMG_GetClipboardImage(void)
{
    SDL_Surface *surface = NULL;

    char **mime_types = SDL_GetClipboardMimeTypes(NULL);
    if (mime_types) {
        for (int i = 0; !surface && mime_types[i]; ++i) {
            if (SDL_strncmp(mime_types[i], "image/", 6) == 0) {
                size_t size = 0;
                void *data = SDL_GetClipboardData(mime_types[i], &size);
                if (data) {
                    SDL_IOStream *src = SDL_IOFromConstMem(data, size);
                    if (src) {
                        surface = IMG_Load_IO(src, true);
                    }
                    SDL_free(data);
                }
            }
        }
    }
    if (!surface) {
        SDL_SetError("No clipboard image available");
    }
    return surface;
}

SDL_Cursor *IMG_CreateAnimatedCursor(IMG_Animation *anim, int hot_x, int hot_y)
{
    int i;
    SDL_CursorFrameInfo *frames;
    SDL_Cursor *cursor;

    if (!anim) {
        SDL_InvalidParamError("anim");
        return NULL;
    }

    frames = (SDL_CursorFrameInfo *)SDL_calloc(anim->count, sizeof(*frames));
    if (!frames) {
        return NULL;
    }

    for (i = 0; i < anim->count; ++i) {
        frames[i].surface = anim->frames[i];
        frames[i].duration = (Uint32)anim->delays[i];
    }

    cursor = SDL_CreateAnimatedCursor(frames, anim->count, hot_x, hot_y);

    SDL_free(frames);

    return cursor;
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

Uint64 IMG_TimebaseDuration(Uint64 pts, Uint64 duration, Uint64 src_numerator, Uint64 src_denominator, Uint64 dst_numerator, Uint64 dst_denominator)
{
    Uint64 a = ( ( ( ( pts + duration ) * 2 ) + 1 ) * src_numerator * dst_denominator ) / ( 2 * src_denominator * dst_numerator );
    Uint64 b = ( ( ( pts * 2 ) + 1 ) * src_numerator * dst_denominator ) / ( 2 * src_denominator * dst_numerator );
    return (a - b);
}
