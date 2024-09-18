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

/* This is a PNG image file loading framework */

#include <SDL3_image/SDL_image.h>
#include "IMG.h"

/* We'll have PNG save support by default */
#if !defined(SDL_IMAGE_SAVE_PNG)
#  define SDL_IMAGE_SAVE_PNG 1
#endif

#if defined(USE_STBIMAGE)
#undef WANT_LIBPNG
#elif defined(SDL_IMAGE_USE_COMMON_BACKEND)
#define WANT_LIBPNG
#elif defined(SDL_IMAGE_USE_WIC_BACKEND)
#undef WANT_LIBPNG
#elif defined(__APPLE__) && defined(PNG_USES_IMAGEIO)
#undef WANT_LIBPNG
#else
#define WANT_LIBPNG
#endif

#ifdef LOAD_PNG

#ifdef WANT_LIBPNG

#define USE_LIBPNG

/* This code was originally written by Philippe Lavoie (2 November 1998) */

#include <SDL3/SDL_endian.h>

#ifdef macintosh
#define MACOS
#endif
#include <png.h>

/* Check for the older version of libpng */
#if (PNG_LIBPNG_VER_MAJOR == 1)
#if (PNG_LIBPNG_VER_MINOR < 5)
#define LIBPNG_VERSION_12
typedef png_bytep png_const_bytep;
typedef png_color *png_const_colorp;
typedef png_color_16 *png_const_color_16p;
#endif
#if (PNG_LIBPNG_VER_MINOR < 4)
typedef png_structp png_const_structp;
typedef png_infop png_const_infop;
#endif
#if (PNG_LIBPNG_VER_MINOR < 6)
typedef png_structp png_structrp;
typedef png_infop png_inforp;
typedef png_const_structp png_const_structrp;
typedef png_const_infop png_const_inforp;
/* noconst15: version < 1.6 doesn't have const, >= 1.6 adds it */
/* noconst16: version < 1.6 does have const, >= 1.6 removes it */
typedef png_structp png_noconst15_structrp;
typedef png_inforp png_noconst15_inforp;
typedef png_const_inforp png_noconst16_inforp;
#else
typedef png_const_structp png_noconst15_structrp;
typedef png_const_inforp png_noconst15_inforp;
typedef png_inforp png_noconst16_inforp;
#endif
#else
typedef png_const_structp png_noconst15_structrp;
typedef png_const_inforp png_noconst15_inforp;
typedef png_inforp png_noconst16_inforp;
#endif

static struct {
    int loaded;
    void *handle;
    png_infop (*png_create_info_struct) (png_noconst15_structrp png_ptr);
    png_structp (*png_create_read_struct) (png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
    void (*png_destroy_read_struct) (png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr);
    png_uint_32 (*png_get_IHDR) (png_noconst15_structrp png_ptr, png_noconst15_inforp info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_method, int *compression_method, int *filter_method);
    png_voidp (*png_get_io_ptr) (png_noconst15_structrp png_ptr);
    png_byte (*png_get_channels) (png_const_structrp png_ptr, png_const_inforp info_ptr);
    png_uint_32 (*png_get_PLTE) (png_const_structrp png_ptr, png_noconst16_inforp info_ptr, png_colorp *palette, int *num_palette);
    png_uint_32 (*png_get_tRNS) (png_const_structrp png_ptr, png_inforp info_ptr, png_bytep *trans, int *num_trans, png_color_16p *trans_values);
    png_uint_32 (*png_get_valid) (png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 flag);
    void (*png_read_image) (png_structrp png_ptr, png_bytepp image);
    void (*png_read_info) (png_structrp png_ptr, png_inforp info_ptr);
    void (*png_read_update_info) (png_structrp png_ptr, png_inforp info_ptr);
    void (*png_set_expand) (png_structrp png_ptr);
    void (*png_set_gray_to_rgb) (png_structrp png_ptr);
    void (*png_set_packing) (png_structrp png_ptr);
    void (*png_set_read_fn) (png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr read_data_fn);
    void (*png_set_strip_16) (png_structrp png_ptr);
    int (*png_set_interlace_handling) (png_structrp png_ptr);
    int (*png_sig_cmp) (png_const_bytep sig, png_size_t start, png_size_t num_to_check);
#ifdef PNG_SETJMP_SUPPORTED
#ifndef LIBPNG_VERSION_12
    jmp_buf* (*png_set_longjmp_fn) (png_structrp, png_longjmp_ptr, size_t);
#endif
#endif
#if SDL_IMAGE_SAVE_PNG
    png_structp (*png_create_write_struct) (png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
    void (*png_destroy_write_struct) (png_structpp png_ptr_ptr, png_infopp info_ptr_ptr);
    void (*png_set_write_fn) (png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn);
    void (*png_set_IHDR) (png_noconst15_structrp png_ptr, png_inforp info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type, int compression_type, int filter_type);
    void (*png_write_info) (png_structrp png_ptr, png_noconst15_inforp info_ptr);
    void (*png_set_rows) (png_noconst15_structrp png_ptr, png_inforp info_ptr, png_bytepp row_pointers);
    void (*png_write_png) (png_structrp png_ptr, png_inforp info_ptr, int transforms, png_voidp params);
    void (*png_set_PLTE) (png_structrp png_ptr, png_inforp info_ptr, png_const_colorp palette, int num_palette);
    void (*png_set_tRNS) (png_structrp png_ptr, png_inforp info_ptr, png_const_bytep trans_alpha, int num_trans, png_const_color_16p trans_color);
#endif
} lib;

#ifdef LOAD_PNG_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = (SIG) SDL_LoadFunction(lib.handle, #FUNC); \
    if (lib.FUNC == NULL) { SDL_UnloadObject(lib.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = FUNC;
#endif

int IMG_InitPNG(void)
{
    if ( lib.loaded == 0 ) {
#ifdef LOAD_PNG_DYNAMIC
        lib.handle = SDL_LoadObject(LOAD_PNG_DYNAMIC);
        if ( lib.handle == NULL ) {
            return -1;
        }
#endif
        FUNCTION_LOADER(png_create_info_struct, png_infop (*) (png_noconst15_structrp png_ptr))
        FUNCTION_LOADER(png_create_read_struct, png_structp (*) (png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn))
        FUNCTION_LOADER(png_destroy_read_struct, void (*) (png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr))
        FUNCTION_LOADER(png_get_IHDR, png_uint_32 (*) (png_noconst15_structrp png_ptr, png_noconst15_inforp info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_method, int *compression_method, int *filter_method))
        FUNCTION_LOADER(png_get_io_ptr, png_voidp (*) (png_noconst15_structrp png_ptr))
        FUNCTION_LOADER(png_get_channels, png_byte (*) (png_const_structrp png_ptr, png_const_inforp info_ptr))
        FUNCTION_LOADER(png_get_PLTE, png_uint_32 (*) (png_const_structrp png_ptr, png_noconst16_inforp info_ptr, png_colorp *palette, int *num_palette))
        FUNCTION_LOADER(png_get_tRNS, png_uint_32 (*) (png_const_structrp png_ptr, png_inforp info_ptr, png_bytep *trans, int *num_trans, png_color_16p *trans_values))
        FUNCTION_LOADER(png_get_valid, png_uint_32 (*) (png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 flag))
        FUNCTION_LOADER(png_read_image, void (*) (png_structrp png_ptr, png_bytepp image))
        FUNCTION_LOADER(png_read_info, void (*) (png_structrp png_ptr, png_inforp info_ptr))
        FUNCTION_LOADER(png_read_update_info, void (*) (png_structrp png_ptr, png_inforp info_ptr))
        FUNCTION_LOADER(png_set_expand, void (*) (png_structrp png_ptr))
        FUNCTION_LOADER(png_set_gray_to_rgb, void (*) (png_structrp png_ptr))
        FUNCTION_LOADER(png_set_packing, void (*) (png_structrp png_ptr))
        FUNCTION_LOADER(png_set_read_fn, void (*) (png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr read_data_fn))
        FUNCTION_LOADER(png_set_strip_16, void (*) (png_structrp png_ptr))
        FUNCTION_LOADER(png_set_interlace_handling, int (*) (png_structrp png_ptr))
        FUNCTION_LOADER(png_sig_cmp, int (*) (png_const_bytep sig, png_size_t start, png_size_t num_to_check))
#ifdef PNG_SETJMP_SUPPORTED
#ifndef LIBPNG_VERSION_12
        FUNCTION_LOADER(png_set_longjmp_fn, jmp_buf* (*) (png_structrp, png_longjmp_ptr, size_t))
#endif
#endif
#if SDL_IMAGE_SAVE_PNG
        FUNCTION_LOADER(png_create_write_struct, png_structp (*) (png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn))
        FUNCTION_LOADER(png_destroy_write_struct, void (*) (png_structpp png_ptr_ptr, png_infopp info_ptr_ptr))
        FUNCTION_LOADER(png_set_write_fn, void (*) (png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn))
        FUNCTION_LOADER(png_set_IHDR, void (*) (png_noconst15_structrp png_ptr, png_inforp info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type, int compression_type, int filter_type))
        FUNCTION_LOADER(png_write_info, void (*) (png_structrp png_ptr, png_noconst15_inforp info_ptr))
        FUNCTION_LOADER(png_set_rows, void (*) (png_noconst15_structrp png_ptr, png_inforp info_ptr, png_bytepp row_pointers))
        FUNCTION_LOADER(png_write_png, void (*) (png_structrp png_ptr, png_inforp info_ptr, int transforms, png_voidp params))
        FUNCTION_LOADER(png_set_PLTE, void (*) (png_structrp png_ptr, png_inforp info_ptr, png_const_colorp palette, int num_palette))
        FUNCTION_LOADER(png_set_tRNS, void (*) (png_structrp png_ptr, png_inforp info_ptr, png_const_bytep trans_alpha, int num_trans, png_const_color_16p trans_color))
#endif
    }
    ++lib.loaded;

    return 0;
}
void IMG_QuitPNG(void)
{
    if ( lib.loaded == 0 ) {
        return;
    }
    if ( lib.loaded == 1 ) {
#ifdef LOAD_PNG_DYNAMIC
        SDL_UnloadObject(lib.handle);
#endif
    }
    --lib.loaded;
}

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
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic) ) {
        if ( magic[0] == 0x89 &&
             magic[1] == 'P' &&
             magic[2] == 'N' &&
             magic[3] == 'G' ) {
            is_PNG = true;
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_PNG;
}

/* Load a PNG type image from an SDL datasource */
static void png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
    SDL_IOStream *src;

    src = (SDL_IOStream *)lib.png_get_io_ptr(ctx);
    SDL_ReadIO(src, area, size);
}

struct loadpng_vars {
    const char *error;
    SDL_Surface *surface;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
};

static void LIBPNG_LoadPNG_IO(SDL_IOStream *src, struct loadpng_vars *vars)
{
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, num_channels;
    Uint32 format;
    int row, i;
    int ckey;
    png_color_16 *transv;

    /* Create the PNG loading context structure */
    vars->png_ptr = lib.png_create_read_struct(PNG_LIBPNG_VER_STRING,
                      NULL,NULL,NULL);
    if (vars->png_ptr == NULL) {
        vars->error = "Couldn't allocate memory for PNG file or incompatible PNG dll";
        return;
    }

     /* Allocate/initialize the memory for image information.  REQUIRED. */
    vars->info_ptr = lib.png_create_info_struct(vars->png_ptr);
    if (vars->info_ptr == NULL) {
        vars->error = "Couldn't create image information for PNG file";
        return;
    }

    /* Set error handling if you are using setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in png_create_read_struct() earlier.
     */

#ifdef PNG_SETJMP_SUPPORTED
#ifdef _MSC_VER
#pragma warning(disable:4611)   /* warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable */
#endif
#ifndef LIBPNG_VERSION_12
    if (setjmp(*lib.png_set_longjmp_fn(vars->png_ptr, longjmp, sizeof(jmp_buf))))
#else
    if (setjmp(vars->png_ptr->jmpbuf))
#endif
    {
        vars->error = "Error reading the PNG file.";
        return;
    }
#endif
    /* Set up the input control */
    lib.png_set_read_fn(vars->png_ptr, src, png_read_data);

    /* Read PNG header info */
    lib.png_read_info(vars->png_ptr, vars->info_ptr);
    lib.png_get_IHDR(vars->png_ptr, vars->info_ptr, &width, &height, &bit_depth,
            &color_type, &interlace_type, NULL, NULL);

    /* tell libpng to strip 16 bit/color files down to 8 bits/color */
    lib.png_set_strip_16(vars->png_ptr);

    /* tell libpng to de-interlace (if the image is interlaced) */
    lib.png_set_interlace_handling(vars->png_ptr);

    /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
     * byte into separate bytes (useful for paletted and grayscale images).
     */
    lib.png_set_packing(vars->png_ptr);

    /* scale greyscale values to the range 0..255 */
    if (color_type == PNG_COLOR_TYPE_GRAY)
        lib.png_set_expand(vars->png_ptr);

    /* For images with a single "transparent colour", set colour key;
       if more than one index has transparency, or if partially transparent
       entries exist, use full alpha channel */
    ckey = -1;
    if (lib.png_get_valid(vars->png_ptr, vars->info_ptr, PNG_INFO_tRNS)) {
        int num_trans;
        Uint8 *trans;
        lib.png_get_tRNS(vars->png_ptr, vars->info_ptr, &trans, &num_trans, &transv);
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            /* Check if all tRNS entries are opaque except one */
            int j, t = -1;
            for (j = 0; j < num_trans; j++) {
                if (trans[j] == 0) {
                    if (t >= 0) {
                        break;
                    }
                    t = j;
                } else if (trans[j] != 255) {
                    break;
                }
            }
            if (j == num_trans) {
                /* exactly one transparent index */
                ckey = t;
            } else {
                /* more than one transparent index, or translucency */
                lib.png_set_expand(vars->png_ptr);
            }
        } else if (color_type == PNG_COLOR_TYPE_GRAY) {
            /* This will be turned into PNG_COLOR_TYPE_GRAY_ALPHA, so expand to RGBA */
            lib.png_set_gray_to_rgb(vars->png_ptr);
        } else {
            ckey = 0; /* actual value will be set later */
        }
    }

    lib.png_read_update_info(vars->png_ptr, vars->info_ptr);

    lib.png_get_IHDR(vars->png_ptr, vars->info_ptr, &width, &height, &bit_depth,
            &color_type, &interlace_type, NULL, NULL);

    /* Allocate the SDL surface to hold the image */
    num_channels = lib.png_get_channels(vars->png_ptr, vars->info_ptr);

    format = SDL_PIXELFORMAT_UNKNOWN;
    if (num_channels == 3) {
       format = SDL_PIXELFORMAT_RGB24;
    } else if (num_channels == 4) {
       format = SDL_PIXELFORMAT_RGBA32;
    } else {
       /* Not sure they are all supported by png */
       switch (bit_depth * num_channels) {
          case 1:
             format = SDL_PIXELFORMAT_INDEX1MSB;
             break;
          case 4:
             format = SDL_PIXELFORMAT_INDEX4MSB;
             break;
          case 8:
             format = SDL_PIXELFORMAT_INDEX8;
             break;
          case 12:
             format = SDL_PIXELFORMAT_XRGB4444;
             break;
          case 15:
             format = SDL_PIXELFORMAT_XRGB1555;
             break;
          case 16:
             format = SDL_PIXELFORMAT_RGB565;
             break;

          default:
             break;
       }
    }

    vars->surface = SDL_CreateSurface(width, height, format);
    if (vars->surface == NULL) {
        vars->error = SDL_GetError();
        return;
    }

    if (ckey != -1) {
        if (color_type != PNG_COLOR_TYPE_PALETTE) {
            /* FIXME: Should these be truncated or shifted down? */
            ckey = SDL_MapSurfaceRGB(vars->surface,
                         (Uint8)transv->red,
                         (Uint8)transv->green,
                         (Uint8)transv->blue);
        }
        SDL_SetSurfaceColorKey(vars->surface, true, ckey);
    }

    /* Create the array of pointers to image data */
    vars->row_pointers = (png_bytep*) SDL_malloc(sizeof(png_bytep)*height);
    if (!vars->row_pointers) {
        vars->error = "Out of memory";
        return;
    }
    for (row = 0; row < (int)height; row++) {
        vars->row_pointers[row] = (png_bytep)
                (Uint8 *)vars->surface->pixels + row*vars->surface->pitch;
    }

    /* Read the entire image in one go */
    lib.png_read_image(vars->png_ptr, vars->row_pointers);

    /* and we're done!  (png_read_end() can be omitted if no processing of
     * post-IDAT text/time/etc. is desired)
     * In some cases it can't read PNG's created by some popular programs (ACDSEE),
     * we do not want to process comments, so we omit png_read_end

    lib.png_read_end(png_ptr, info_ptr);
    */

    /* Load the palette, if any */
    if (SDL_ISPIXELFORMAT_INDEXED(vars->surface->format)) {
        SDL_Palette *palette = NULL;
        int png_num_palette;
        png_colorp png_palette;
        lib.png_get_PLTE(vars->png_ptr, vars->info_ptr, &png_palette, &png_num_palette);
        if (color_type == PNG_COLOR_TYPE_GRAY) {
            palette = SDL_CreateSurfacePalette(vars->surface);
            if (!palette) {
                vars->error = SDL_GetError();
                return;
            }
            for (i = 0; i < 256; i++) {
                palette->colors[i].r = (Uint8)i;
                palette->colors[i].g = (Uint8)i;
                palette->colors[i].b = (Uint8)i;
            }
        } else if (png_num_palette > 0 ) {
            palette = SDL_CreateSurfacePalette(vars->surface);
            if (!palette) {
                vars->error = SDL_GetError();
                return;
            }
            if (png_num_palette > palette->ncolors) {
                png_num_palette = palette->ncolors;
            }
            palette->ncolors = png_num_palette;

            for (i = 0; i < png_num_palette; ++i) {
                palette->colors[i].b = png_palette[i].blue;
                palette->colors[i].g = png_palette[i].green;
                palette->colors[i].r = png_palette[i].red;
            }
        }
    }
}

SDL_Surface *IMG_LoadPNG_IO(SDL_IOStream *src)
{
    Sint64 start;
    struct loadpng_vars vars;

    if ( !src ) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }

    if ( (IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0 ) {
        return NULL;
    }

    start = SDL_TellIO(src);
    SDL_zero(vars);

    LIBPNG_LoadPNG_IO(src, &vars);

    if (vars.png_ptr) {
        lib.png_destroy_read_struct(&vars.png_ptr,
                                vars.info_ptr ? &vars.info_ptr : (png_infopp)0,
                                (png_infopp)0);
    }
    if (vars.row_pointers) {
        SDL_free(vars.row_pointers);
    }
    if (vars.error) {
        SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
        if (vars.surface) {
            SDL_DestroySurface(vars.surface);
            vars.surface = NULL;
        }
        SDL_SetError("%s", vars.error);
    }

    return vars.surface;
}

#elif defined(USE_STBIMAGE)

extern SDL_Surface *IMG_LoadSTB_IO(SDL_IOStream *src);

int IMG_InitPNG(void)
{
    /* Nothing to load */
    return 0;
}

void IMG_QuitPNG(void)
{
    /* Nothing to unload */
}

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
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic) ) {
        if ( magic[0] == 0x89 &&
             magic[1] == 'P' &&
             magic[2] == 'N' &&
             magic[3] == 'G' ) {
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

#endif /* WANT_LIBPNG */

#else
#if defined(_MSC_VER) && _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

int IMG_InitPNG(void)
{
    SDL_SetError("PNG images are not supported");
    return -1;
}

void IMG_QuitPNG(void)
{
}

/* See if an image is contained in a data source */
bool IMG_isPNG(SDL_IOStream *src)
{
    return false;
}

/* Load a PNG type image from an SDL datasource */
SDL_Surface *IMG_LoadPNG_IO(SDL_IOStream *src)
{
    return NULL;
}

#endif /* LOAD_PNG */

#if SDL_IMAGE_SAVE_PNG

static const Uint32 png_format = SDL_PIXELFORMAT_RGBA32;

#ifdef USE_LIBPNG

static void png_write_data(png_structp png_ptr, png_bytep src, png_size_t size)
{
    SDL_IOStream *dst = (SDL_IOStream *)lib.png_get_io_ptr(png_ptr);
    SDL_WriteIO(dst, src, size);
}

static void png_flush_data(png_structp png_ptr)
{
    (void)png_ptr;
}

struct savepng_vars {
    png_structp png_ptr;
    png_infop info_ptr;
    const char *error;
    png_colorp color_ptr;
    png_bytep *row_pointers;
    SDL_Surface *source;
};

static bool LIBPNG_SavePNG_IO(struct savepng_vars *vars, SDL_Surface *surface, SDL_IOStream *dst)
{
    Uint8 transparent_table[256];
    SDL_Palette *palette;
    int png_color_type;

    vars->source = surface;

    vars->png_ptr = lib.png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (vars->png_ptr == NULL) {
        vars->error = "Couldn't allocate memory for PNG file or incompatible PNG dll";
        return false;
    }

    vars->info_ptr = lib.png_create_info_struct(vars->png_ptr);
    if (vars->info_ptr == NULL) {
        vars->error = "Couldn't create image information for PNG file";
        return false;
    }
#ifdef PNG_SETJMP_SUPPORTED
#ifndef LIBPNG_VERSION_12
    if (setjmp(*lib.png_set_longjmp_fn(vars->png_ptr, longjmp, sizeof (jmp_buf))))
#else
    if (setjmp(vars->png_ptr->jmpbuf))
#endif
#endif
    {
        vars->error = "Error writing the PNG file.";
        return false;
    }

    palette = SDL_GetSurfacePalette(surface);
    if (palette) {
        const int ncolors = palette->ncolors;
        int i;
        int last_transparent = -1;

        vars->color_ptr = (png_colorp)SDL_malloc(sizeof(png_color) * ncolors);
        if (vars->color_ptr == NULL) {
            vars->error = "Couldn't create palette for PNG file";
            return false;
        }
        for (i = 0; i < ncolors; i++) {
            vars->color_ptr[i].red = palette->colors[i].r;
            vars->color_ptr[i].green = palette->colors[i].g;
            vars->color_ptr[i].blue = palette->colors[i].b;
            if (palette->colors[i].a != 255) {
                last_transparent = i;
            }
        }
        lib.png_set_PLTE(vars->png_ptr, vars->info_ptr, vars->color_ptr, ncolors);
        png_color_type = PNG_COLOR_TYPE_PALETTE;

        if (last_transparent >= 0) {
            for (i = 0; i <= last_transparent; ++i) {
                transparent_table[i] = palette->colors[i].a;
            }
            lib.png_set_tRNS(vars->png_ptr, vars->info_ptr, transparent_table, last_transparent + 1, NULL);
        }
    }
    else if (surface->format == SDL_PIXELFORMAT_RGB24) {
        /* If the surface is exactly the right RGB format it is just passed through */
        png_color_type = PNG_COLOR_TYPE_RGB;
    }
    else if (!SDL_ISPIXELFORMAT_ALPHA(surface->format)) {
        /* If the surface is not exactly the right RGB format but does not have alpha
           information, it should be converted to RGB24 before being passed through */
        png_color_type = PNG_COLOR_TYPE_RGB;
        vars->source = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGB24);
    }
    else if (surface->format != png_format) {
        /* Otherwise, (surface has alpha data), and it is not in the exact right
           format , so it should be converted to that */
        png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        vars->source = SDL_ConvertSurface(surface, png_format);
    } else {
        png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }

    lib.png_set_write_fn(vars->png_ptr, dst, png_write_data, png_flush_data);

    lib.png_set_IHDR(vars->png_ptr, vars->info_ptr, surface->w, surface->h,
                     8, png_color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (vars->source) {
        int row;
        vars->row_pointers = (png_bytep *) SDL_malloc(sizeof(png_bytep) * vars->source->h);
        if (!vars->row_pointers) {
            vars->error = "Out of memory";
            return false;
        }
        for (row = 0; row < (int)vars->source->h; row++) {
            vars->row_pointers[row] = (png_bytep) (Uint8 *) vars->source->pixels + row * vars->source->pitch;
        }

        lib.png_set_rows(vars->png_ptr, vars->info_ptr, vars->row_pointers);
        lib.png_write_png(vars->png_ptr, vars->info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    }

    return true;
}

static bool IMG_SavePNG_IO_libpng(SDL_Surface *surface, SDL_IOStream *dst)
{
    struct savepng_vars vars;
    bool result;

    if (!IMG_Init(IMG_INIT_PNG)) {
        return false;
    }

    SDL_zero(vars);
    result = LIBPNG_SavePNG_IO(&vars, surface, dst);

    if (vars.png_ptr) {
        lib.png_destroy_write_struct(&vars.png_ptr, &vars.info_ptr);
    }

    if (vars.color_ptr) {
        SDL_free(vars.color_ptr);
    }

    if (vars.row_pointers) {
        SDL_free(vars.row_pointers);
    }

    if (vars.source != surface) {
        SDL_DestroySurface(vars.source);
    }

    if (vars.error) {
        SDL_SetError("%s", vars.error);
    }

    return result;
}

#endif /* USE_LIBPNG */

#if defined(LOAD_PNG_DYNAMIC) || !defined(WANT_LIBPNG)
/* Replace C runtime functions with SDL C runtime functions for building on Windows */
#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_SDL_MALLOC
#define MZ_ASSERT(x) SDL_assert(x)
#undef memcpy
#define memcpy  SDL_memcpy
#undef memset
#define memset  SDL_memset
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

bool IMG_SavePNG_IO(SDL_Surface *surface, SDL_IOStream *dst, int closeio)
{
    bool result = false;

    if (!dst) {
        return SDL_SetError("Passed NULL dst");
    }

#if SDL_IMAGE_SAVE_PNG
#ifdef USE_LIBPNG
    if (!result) {
        result = IMG_SavePNG_IO_libpng(surface, dst);
    }
#endif

#if defined(LOAD_PNG_DYNAMIC) || !defined(WANT_LIBPNG)
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
