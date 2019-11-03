/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_image.h"

/* always enabled, apparently. */
#define SAVE_PNG 1

#if !(defined(__APPLE__) || defined(SDL_IMAGE_USE_WIC_BACKEND)) || defined(SDL_IMAGE_USE_COMMON_BACKEND)

#ifdef LOAD_PNG

#define LODEPNG_NO_COMPILE_ALLOCATORS 1  /* use SDL_malloc instead. */
#define LODEPNG_NO_COMPILE_DISK 1  /* we do this from SDL_RWops, never filenames. */
#ifndef SAVE_PNG
#  define LODEPNG_NO_COMPILE_ENCODER 1
#endif

/* #define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS */
/* #define LODEPNG_NO_COMPILE_ERROR_TEXT */
#include "external/lodepng/lodepng.c"
void* lodepng_malloc(size_t size) { return SDL_malloc(size); }
void* lodepng_realloc(void* ptr, size_t new_size) { return SDL_realloc(ptr, new_size); }
void lodepng_free(void* ptr) { return SDL_free(ptr); }


int IMG_InitPNG()
{
    return 0;
}

void IMG_QuitPNG()
{
}

int IMG_isPNG(SDL_RWops *src)
{
    Sint64 start;
    Uint8 buf[8];
    int retval = 0;

    if (src) {
        start = SDL_RWtell(src);
        if (SDL_RWread(src, buf, sizeof (buf), 1)) {
            static const Uint8 magic[] = { 137, 80, 78, 71, 13, 10, 26, 10 };  /* stole this list from lodepng's sources. */
            SDL_assert(sizeof (buf) == sizeof (magic));
            if (SDL_memcmp(buf, magic, sizeof (buf)) == 0) {
                retval = 1;  /* it's a PNG signature. */
            }
        }
        SDL_RWseek(src, start, RW_SEEK_SET);
    }

    return retval;
}

SDL_Surface *IMG_LoadPNG_RW(SDL_RWops *src)
{
    unsigned char *png = NULL;
    size_t pngsize = 0;
    Sint64 start = 0;
    unsigned int w, h;
    LodePNGState state;
    SDL_bool has_alpha = SDL_FALSE;
    SDL_Surface *retval = NULL;
    unsigned int error = 0;
    unsigned char *pixels = NULL;
    Uint32 pixfmt = 0;
    int srcpitch = 0;

    if ( (IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0 ) {
        return NULL;
    }

    SDL_zero(state);
    lodepng_state_init(&state);

    start = SDL_RWtell(src);
    png = SDL_LoadFile_RW(src, &pngsize, 0);
    SDL_RWseek(src, start, RW_SEEK_SET);

    if (!png) {
        goto failed;  /* assume SDL set the error string */
    }

    error = lodepng_inspect(&w, &h, &state, png, pngsize);
    if (error) {
#ifdef LODEPNG_COMPILE_ERROR_TEXT
        IMG_SetError("Failed to parse PNG metadata: %s", lodepng_error_text(error));
#else
        IMG_SetError("Failed to parse PNG metadata: %d", error);
#endif
        goto failed;
    }

    /* Decide what kind of SDL surface we want. */
    switch (state.info_png.color.colortype) {
        case LCT_GREY_ALPHA:
        case LCT_RGBA:
            pixfmt = SDL_PIXELFORMAT_ABGR8888;
            error = lodepng_decode32(&pixels, &w, &h, png, pngsize);
            break;

        case LCT_GREY:
        case LCT_RGB:
            /* !!! FIXME: we can report a colorkey here if we don't convert. */
            pixfmt = SDL_PIXELFORMAT_BGR888;  /* this is actually 32-bit BGRX */
            error = lodepng_decode32(&pixels, &w, &h, png, pngsize);
            break;

        case LCT_PALETTE: {
            const int bitdepth = state.info_png.color.bitdepth;
            pixfmt = SDL_PIXELFORMAT_INDEX8;
            lodepng_state_cleanup(&state);
            SDL_zero(state);
            lodepng_state_init(&state);
            state.decoder.color_convert = 0;
            if (bitdepth != 8) {
                state.info_raw.colortype = LCT_PALETTE;
                state.info_raw.bitdepth = 8;
                state.decoder.color_convert = 1;
            }
            error = lodepng_decode(&pixels, &w, &h, &state, png, pngsize);
            break;
        }

        default: break;
    }

    if (error) {
#ifdef LODEPNG_COMPILE_ERROR_TEXT
        IMG_SetError("Failed to decode PNG file: %s", lodepng_error_text(error));
#else
        IMG_SetError("Failed to decode PNG file: %d", error);
#endif
        goto failed;
    }

    SDL_free(png);  /* don't need the original bytes anymore. */
    png = NULL;

    retval = SDL_CreateRGBSurfaceWithFormat(0, w, h, SDL_BITSPERPIXEL(pixfmt), pixfmt);
    if (!retval) {
        goto failed;  /* assume SDL set the error string */
    } else {
        const int srcpitch = w * SDL_BYTESPERPIXEL(pixfmt);
        const int dstpitch = retval->pitch;
        const Uint8 *src = (const Uint8 *) pixels;
        Uint8 *dst = (Uint8 *) retval->pixels;

        if (dstpitch == srcpitch) {
            SDL_memcpy(dst, src, dstpitch * h);
        } else {
            const int cpylen = SDL_min(srcpitch, dstpitch);
            unsigned int y;
            for (y = 0; y < h; y++) {
                SDL_memcpy(dst, src, cpylen);
                dst += dstpitch;
                src += srcpitch;
            }
        }
    }

    lodepng_free(pixels);
    pixels = NULL;

    if (pixfmt == SDL_PIXELFORMAT_INDEX8) {
        const size_t total = state.info_png.color.palettesize;
        const unsigned char *src = state.info_png.color.palette;
        SDL_Color *palette = NULL;
        SDL_Color *paldst = NULL;
        size_t i;

        if ((src == NULL) || (total == 0)) {
            IMG_SetError("PNG has palette but lodepng didn't provide it");
            goto failed;
        }

        palette = (SDL_Color *) SDL_calloc(total, sizeof (SDL_Color));
        if (!palette) {
            SDL_OutOfMemory();
            goto failed;
        }

        paldst = palette;
        for (i = 0; i < total; i++) {
            paldst->r = src[0];
            paldst->g = src[1];
            paldst->b = src[2];
            paldst->a = src[3];
            src += 4;
            paldst++;
        }

        error = SDL_SetPaletteColors(retval->format->palette, palette, 0, (int) total);
        SDL_free(palette);

        if (error == -1) {
            goto failed;  /* assume SDL set the error string */
        }
    }

    lodepng_state_cleanup(&state);

    return retval;

failed:
    lodepng_state_cleanup(&state);
    if (png) SDL_free(png);
    if (pixels) lodepng_free(pixels);
    if (retval) SDL_FreeSurface(retval);
    return NULL;
}

#else

#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

int IMG_InitPNG()
{
    IMG_SetError("PNG images are not supported");
    return -1;
}

void IMG_QuitPNG()
{
}

/* See if an image is contained in a data source */
int IMG_isPNG(SDL_RWops *src)
{
    return 0;
}

/* Load a PNG type image from an SDL datasource */
SDL_Surface *IMG_LoadPNG_RW(SDL_RWops *src)
{
    return NULL;
}

#endif /* LOAD_PNG */

#endif /* !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND) */

#ifdef SAVE_PNG

int IMG_SavePNG(SDL_Surface *surface, const char *file)
{
    SDL_RWops *dst = SDL_RWFromFile(file, "wb");
    if (dst) {
        return IMG_SavePNG_RW(surface, dst, 1);
    }
    return -1;
}

static int IMG_SavePNG_RW_internal(SDL_Surface *surface, SDL_RWops *dst)
{
#ifndef LOAD_PNG  /* no lodepng support without LOAD_PNG defined... */
    return IMG_SetError("SDL_image not built with lodepng, saving not supported");
#else
    const Uint32 pixfmt = surface->format->format;
    unsigned int error = 0;
    unsigned char *png = NULL;
    SDL_Surface *png_surface = surface;
    size_t pngsize = 0;
    LodePNGState state;

    SDL_assert(dst != NULL);
    SDL_assert(surface != NULL);

    SDL_zero(state);
    lodepng_state_init(&state);

    if (SDL_ISPIXELFORMAT_INDEXED(pixfmt)) {
        const unsigned int bpp = (unsigned int) surface->format->BitsPerPixel;
        const int ncolors = surface->format->palette->ncolors;
        const SDL_Color *c = surface->format->palette->colors;
        int i, error = 0;
        state.info_raw.colortype = LCT_PALETTE;
        state.info_raw.bitdepth = bpp;
        state.info_png.color.colortype = LCT_PALETTE;
        state.info_png.color.bitdepth = bpp;
        state.encoder.auto_convert = 0;
        for (i = 0; i < ncolors; i++) {
            error = lodepng_palette_add(&state.info_png.color, c->r, c->g, c->b, c->a);
            if (error) break;
            error = lodepng_palette_add(&state.info_raw, c->r, c->g, c->b, c->a);
            if (error) break;
            c++;
        }
        if (error) {
            lodepng_state_cleanup(&state);
#ifdef LODEPNG_COMPILE_ERROR_TEXT
            return IMG_SetError("Couldn't prepare palette: %s", lodepng_error_text(error));
#else
            return IMG_SetError("Couldn't prepare palette: %d", error);
#endif
        }
    } else if (SDL_ISPIXELFORMAT_ALPHA(pixfmt)) {
        state.info_raw.colortype = LCT_RGBA;
        state.info_raw.bitdepth = 8;
        state.info_png.color.colortype = LCT_RGBA;
        state.info_png.color.bitdepth = 8;
        if (pixfmt != SDL_PIXELFORMAT_ABGR8888) {
            png_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
            if (!png_surface) {
                lodepng_state_cleanup(&state);
                return -1;  /* assume SDL set the error string */
            }
        }
    } else {
        state.info_raw.colortype = LCT_RGB;
        state.info_raw.bitdepth = 8;
        state.info_png.color.colortype = LCT_RGB;
        state.info_png.color.bitdepth = 8;
        if (pixfmt != SDL_PIXELFORMAT_BGR24) {
            png_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_BGR24, 0);
            if (!png_surface) {
                lodepng_state_cleanup(&state);
                return -1;  /* assume SDL set the error string */
            }
        }
    }

    error = lodepng_encode(&png, &pngsize, png_surface->pixels, png_surface->w, png_surface->h, &state);

    if (png_surface != surface) {
        SDL_FreeSurface(png_surface);
    }
    lodepng_state_cleanup(&state);

    if (error) {
#ifdef LODEPNG_COMPILE_ERROR_TEXT
        return IMG_SetError("Couldn't encode PNG: %s", lodepng_error_text(error));
#else
        return IMG_SetError("Couldn't encode PNG: %d", error);
#endif
    }

    /* assume SDL_RWwrite will set the error string if necessary. */
    error = (SDL_RWwrite(dst, png, 1, pngsize) != pngsize) ? -1 : 0;

    lodepng_free(png);

    return error;
#endif
}

int IMG_SavePNG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst)
{
    int retval = -1;

    if (!dst) {
        IMG_SetError("Passed NULL dst");
    } else if (!surface) {
        IMG_SetError("Passed NULL surface");
    } else {
        retval = IMG_SavePNG_RW_internal(surface, dst);
    }

    if (freedst) {
        if (SDL_RWclose(dst) == -1) {
            retval = -1;
        }
    }

    return retval;
}

#endif /* SAVE_PNG */

