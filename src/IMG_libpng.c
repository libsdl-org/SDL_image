/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

/*                    This is a PNG image file framework                        */
/********************************************************************************
 * *                                                                           **
 * Initially written entirely by Xen (@lordofxen) on 7/28/2025.                 *
 * Improvements made by: slouken, sezero, madebr, and smcv.                     *
 * *                                                                           **
 *******************************************************************************/

#include <SDL3_image/SDL_image.h>

#include "IMG_libpng.h"
#include "IMG_anim_encoder.h"
#include "IMG_anim_decoder.h"
#include "IMG_utils.h"

#ifdef SDL_IMAGE_LIBPNG
#ifdef INCLUDE_PNG_FRAMEWORK
#include <png/png.h>
#else
#include <png.h>
#endif

#if defined(LOAD_LIBPNG_DYNAMIC) && defined(SDL_ELF_NOTE_DLOPEN)
SDL_ELF_NOTE_DLOPEN(
    "png",
    "Support for PNG images using libpng",
    SDL_ELF_NOTE_DLOPEN_PRIORITY_SUGGESTED,
    LOAD_LIBPNG_DYNAMIC
)
#endif

#ifndef PNG_DISPOSE_OP_NONE
#define PNG_DISPOSE_OP_NONE 0
#endif
#ifndef PNG_DISPOSE_OP_BACKGROUND
#define PNG_DISPOSE_OP_BACKGROUND 1
#endif
#ifndef PNG_DISPOSE_OP_PREVIOUS
#define PNG_DISPOSE_OP_PREVIOUS 2
#endif

#ifndef PNG_BLEND_OP_SOURCE
#define PNG_BLEND_OP_SOURCE 0
#endif
#ifndef PNG_BLEND_OP_OVER
#define PNG_BLEND_OP_OVER 1
#endif

#ifndef APNG_DEFAULT_DENOMINATOR
#define APNG_DEFAULT_DENOMINATOR 100
#endif

// We will have the PNG saving feature by default
#ifndef SAVE_PNG
#define SAVE_PNG 1
#endif

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
typedef png_const_structrp png_noconst15_structrp;
typedef png_const_inforp png_noconst15_inforp;
typedef png_inforp png_noconst16_inforp;
#endif
#else
typedef png_const_structrp png_noconst15_structrp;
typedef png_const_inforp png_noconst15_inforp;
typedef png_inforp png_noconst16_inforp;
#endif

static struct
{
    int loaded;
    #ifdef LOAD_LIBPNG_DYNAMIC
    void *handle_libpng;

    /* Uncomment this if you want to use zlib with libpng to decompress / compress manually if you'd prefer that.
     *
     * void *handle_zlib;
     */
    #endif

    png_infop (*png_create_info_struct)(png_noconst15_structrp png_ptr);
    png_structp (*png_create_read_struct)(png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
    void (*png_destroy_read_struct)(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr);
    png_uint_32 (*png_get_IHDR)(png_noconst15_structrp png_ptr, png_noconst15_inforp info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_method, int *compression_method, int *filter_method);
    png_voidp (*png_get_io_ptr)(png_noconst15_structrp png_ptr);
    png_byte (*png_get_channels)(png_const_structrp png_ptr, png_const_inforp info_ptr);

    void (*png_error)(png_noconst15_structrp png_ptr, png_const_charp error_message);

    png_uint_32 (*png_get_PLTE)(png_const_structrp png_ptr, png_noconst16_inforp info_ptr, png_colorp *palette, int *num_palette);
    png_uint_32 (*png_get_tRNS)(png_const_structrp png_ptr, png_inforp info_ptr, png_bytep *trans, int *num_trans, png_color_16p *trans_values);
    png_uint_32 (*png_get_valid)(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 flag);
    void (*png_read_image)(png_structrp png_ptr, png_bytepp image);
    void (*png_read_info)(png_structrp png_ptr, png_inforp info_ptr);
    void (*png_read_update_info)(png_structrp png_ptr, png_inforp info_ptr);
    void (*png_set_expand)(png_structrp png_ptr);
    void (*png_set_gray_to_rgb)(png_structrp png_ptr);
    void (*png_set_read_fn)(png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr read_data_fn);
    void (*png_set_strip_16)(png_structrp png_ptr);
    int (*png_set_interlace_handling)(png_structrp png_ptr);
    int (*png_sig_cmp)(png_const_bytep sig, png_size_t start, png_size_t num_to_check);
#ifndef LIBPNG_VERSION_12
    jmp_buf *(*png_set_longjmp_fn)(png_structrp, png_longjmp_ptr, size_t);
#endif
    void (*png_set_palette_to_rgb)(png_structrp png_ptr);
    void (*png_set_tRNS_to_alpha)(png_structrp png_ptr);
    void (*png_set_filler)(png_structrp png_ptr, png_uint_32 filler, int flags);

    void (*png_set_read_user_chunk_fn)(png_structrp png_ptr, png_voidp user_chunk_ptr, png_user_chunk_ptr read_user_chunk_fn);
    void (*png_set_keep_unknown_chunks)(png_structrp png_ptr, int keep, png_const_bytep chunk_list, int num_chunks);
    void (*png_set_sig_bytes)(png_structrp png_ptr, int num_bytes);
    void (*png_set_compression_level)(png_structrp png_ptr, int level);

    void (*png_set_filter)(png_structrp png_ptr, int method, int filters);

    png_structp (*png_create_write_struct)(png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
    void (*png_destroy_write_struct)(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr);
    void (*png_set_write_fn)(png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn);
    void (*png_set_IHDR)(png_noconst15_structrp png_ptr, png_inforp info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type, int compression_type, int filter_type);
    void (*png_write_info)(png_structrp png_ptr, png_noconst15_inforp info_ptr);
    void (*png_set_rows)(png_noconst15_structrp png_ptr, png_inforp info_ptr, png_bytepp row_pointers);
    void (*png_set_PLTE)(png_structrp png_ptr, png_inforp info_ptr, png_const_colorp palette, int num_palette);
    void (*png_set_tRNS)(png_structrp png_ptr, png_inforp info_ptr, png_const_bytep trans_alpha, int num_trans, png_const_color_16p trans_color);

    void (*png_write_image)(png_structrp png_ptr, png_bytepp image);
    void (*png_write_end)(png_structrp png_ptr, png_inforp info_ptr);

    void (*png_read_end)(png_structrp png_ptr, png_inforp info_ptr);

    png_byte (*png_get_bit_depth)(png_const_structrp png_ptr, png_const_inforp info_ptr);
    png_byte (*png_get_color_type)(png_const_structrp png_ptr, png_const_inforp info_ptr);
    png_uint_32 (*png_get_image_width)(png_const_structrp png_ptr, png_const_inforp info_ptr);
    png_uint_32 (*png_get_image_height)(png_const_structrp png_ptr, png_const_inforp info_ptr);
#if (PNG_LIBPNG_VER_MAJOR == 1) && (PNG_LIBPNG_VER_MINOR < 6)
    png_uint_32 (*png_get_text)(png_const_structp png_ptr, png_const_infop info_ptr, png_textp *text_ptr, int *num_text);
#else
    int (*png_get_text)(png_const_structrp png_ptr, png_inforp info_ptr, png_textp *text_ptr, int *num_text);
#endif

    void (*png_write_flush)(png_structrp png_ptr);
} lib;

#define libpng_get_uint_32(buf)           \
   (((png_uint_32)( *(buf)     ) << 24) + \
    ((png_uint_32)(*((buf) + 1)) << 16) + \
    ((png_uint_32)(*((buf) + 2)) <<  8) + \
    ((png_uint_32)(*((buf) + 3))))

#ifdef LOAD_LIBPNG_DYNAMIC
    #define FUNCTION_LOADER_LIBPNG(FUNC, SIG)                       \
        lib.FUNC = (SIG)SDL_LoadFunction(lib.handle_libpng, #FUNC); \
        if (lib.FUNC == NULL) {                                     \
            SDL_UnloadObject(lib.handle_libpng);                    \
            return false;                                           \
        }

    /* Uncomment this if you want to use zlib with libpng to decompress / compress manually if you'd prefer that.
    *
    #define FUNCTION_LOADER_ZLIB(FUNC, SIG)                             \
        lib.FUNC = (SIG)SDL_LoadFunction(lib.handle_zlib, #FUNC);       \
        if (lib.FUNC == NULL) {                                         \
            SDL_UnloadObject(lib.handle_zlib);                          \
            return false;                                               \
        }
     */
#else
    #define FUNCTION_LOADER_LIBPNG(FUNC, SIG)               \
        lib.FUNC = FUNC;                                    \
        if (lib.FUNC == NULL) {                             \
            return SDL_SetError("Missing png.framework");   \
        }
#endif

#ifdef __APPLE__
/* Need to turn off optimizations so weak framework load check works */
__attribute__((optnone))
#endif
bool IMG_InitPNG(void)
{
    if (lib.loaded == 0) {
        /* Uncomment this if you want to use zlib with libpng to decompress / compress manually if you'd prefer that.
         *
        lib.handle_zlib = SDL_LoadObject(LOAD_ZLIB_DYNAMIC);
        if (lib.handle_zlib == NULL) {
            return false;
        }
        */

#ifdef LOAD_LIBPNG_DYNAMIC
        lib.handle_libpng = SDL_LoadObject(LOAD_LIBPNG_DYNAMIC);
        if (lib.handle_libpng == NULL) {
            return false;
        }
#endif

        FUNCTION_LOADER_LIBPNG(png_create_info_struct, png_infop(*)(png_noconst15_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_create_read_struct, png_structp(*)(png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn))
        FUNCTION_LOADER_LIBPNG(png_destroy_read_struct, void (*)(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr))
        FUNCTION_LOADER_LIBPNG(png_get_IHDR, png_uint_32(*)(png_noconst15_structrp png_ptr, png_noconst15_inforp info_ptr, png_uint_32 * width, png_uint_32 * height, int *bit_depth, int *color_type, int *interlace_method, int *compression_method, int *filter_method))
        FUNCTION_LOADER_LIBPNG(png_get_io_ptr, png_voidp(*)(png_noconst15_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_get_channels, png_byte(*)(png_const_structrp png_ptr, png_const_inforp info_ptr))

        FUNCTION_LOADER_LIBPNG(png_error,void (*)(png_noconst15_structrp png_ptr, png_const_charp error_message))

        FUNCTION_LOADER_LIBPNG(png_get_PLTE, png_uint_32(*)(png_const_structrp png_ptr, png_noconst16_inforp info_ptr, png_colorp * palette, int *num_palette))
        FUNCTION_LOADER_LIBPNG(png_get_tRNS, png_uint_32(*)(png_const_structrp png_ptr, png_inforp info_ptr, png_bytep * trans, int *num_trans, png_color_16p *trans_values))
        FUNCTION_LOADER_LIBPNG(png_get_valid, png_uint_32(*)(png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 flag))
        FUNCTION_LOADER_LIBPNG(png_read_image, void (*)(png_structrp png_ptr, png_bytepp image))
        FUNCTION_LOADER_LIBPNG(png_read_info, void (*)(png_structrp png_ptr, png_inforp info_ptr))
        FUNCTION_LOADER_LIBPNG(png_read_update_info, void (*)(png_structrp png_ptr, png_inforp info_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_expand, void (*)(png_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_gray_to_rgb, void (*)(png_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_read_fn, void (*)(png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr read_data_fn))
        FUNCTION_LOADER_LIBPNG(png_set_strip_16, void (*)(png_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_interlace_handling, int (*)(png_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_sig_cmp, int (*)(png_const_bytep sig, png_size_t start, png_size_t num_to_check))
#ifndef LIBPNG_VERSION_12
        FUNCTION_LOADER_LIBPNG(png_set_longjmp_fn, jmp_buf * (*)(png_structrp, png_longjmp_ptr, size_t))
#endif
        FUNCTION_LOADER_LIBPNG(png_set_palette_to_rgb, void (*)(png_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_tRNS_to_alpha, void (*)(png_structrp png_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_filler, void (*)(png_structrp png_ptr, png_uint_32 filler, int flags))

        FUNCTION_LOADER_LIBPNG(png_set_read_user_chunk_fn, void (*)(png_structrp png_ptr, png_voidp user_chunk_ptr, png_user_chunk_ptr read_user_chunk_fn))
        FUNCTION_LOADER_LIBPNG(png_set_keep_unknown_chunks, void (*)(png_structrp png_ptr, int keep, png_const_bytep chunk_list, int num_chunks))
        FUNCTION_LOADER_LIBPNG(png_set_sig_bytes, void (*)(png_structrp png_ptr, int num_bytes))
        FUNCTION_LOADER_LIBPNG(png_set_compression_level, void (*)(png_structrp png_ptr, int level))

        FUNCTION_LOADER_LIBPNG(png_set_filter, void (*)(png_structrp png_ptr, int method, int filters))

        FUNCTION_LOADER_LIBPNG(png_create_write_struct, png_structp(*)(png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn))
        FUNCTION_LOADER_LIBPNG(png_destroy_write_struct, void (*)(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_write_fn, void (*)(png_structrp png_ptr, png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn))
        FUNCTION_LOADER_LIBPNG(png_set_IHDR, void (*)(png_noconst15_structrp png_ptr, png_inforp info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type, int compression_type, int filter_type))
        FUNCTION_LOADER_LIBPNG(png_write_info, void (*)(png_structrp png_ptr, png_noconst15_inforp info_ptr))
        FUNCTION_LOADER_LIBPNG(png_set_rows, void (*)(png_noconst15_structrp png_ptr, png_inforp info_ptr, png_bytepp row_pointers))
        FUNCTION_LOADER_LIBPNG(png_set_PLTE, void (*)(png_structrp png_ptr, png_inforp info_ptr, png_const_colorp palette, int num_palette))
        FUNCTION_LOADER_LIBPNG(png_set_tRNS, void (*)(png_structrp png_ptr, png_inforp info_ptr, png_const_bytep trans_alpha, int num_trans, png_const_color_16p trans_color))

        FUNCTION_LOADER_LIBPNG(png_write_image, void (*)(png_structrp png_ptr, png_bytepp image))
        FUNCTION_LOADER_LIBPNG(png_write_end, void (*)(png_structrp png_ptr, png_inforp info_ptr))

        FUNCTION_LOADER_LIBPNG(png_read_end, void (*)(png_structrp png_ptr, png_inforp info_ptr))

        FUNCTION_LOADER_LIBPNG(png_get_bit_depth, png_byte(*)(png_const_structrp png_ptr, png_const_inforp info_ptr))
        FUNCTION_LOADER_LIBPNG(png_get_color_type, png_byte(*)(png_const_structrp png_ptr, png_const_inforp info_ptr))
        FUNCTION_LOADER_LIBPNG(png_get_image_width, png_uint_32(*)(png_const_structrp png_ptr, png_const_inforp info_ptr))
        FUNCTION_LOADER_LIBPNG(png_get_image_height, png_uint_32(*)(png_const_structrp png_ptr, png_const_inforp info_ptr))
#if (PNG_LIBPNG_VER_MAJOR == 1) && (PNG_LIBPNG_VER_MINOR < 6)
        FUNCTION_LOADER_LIBPNG(png_get_text, png_uint_32 (*)(png_const_structp png_ptr, png_const_infop info_ptr, png_textp *text_ptr, int *num_text))
#else
        FUNCTION_LOADER_LIBPNG(png_get_text, int (*)(png_const_structrp png_ptr, png_inforp info_ptr, png_textp *text_ptr, int *num_text))
#endif

        FUNCTION_LOADER_LIBPNG(png_write_flush, void (*)(png_structrp png_ptr))
    }
    ++lib.loaded;
    return true;
}

static const png_byte png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

// Custom implementation of png_save_uint_32 to ensure network byte order (big-endian) writing.
static void custom_png_save_uint_32(png_bytep buf, png_uint_32 i)
{
    buf[0] = (png_byte)((i >> 24) & 0xff);
    buf[1] = (png_byte)((i >> 16) & 0xff);
    buf[2] = (png_byte)((i >> 8) & 0xff);
    buf[3] = (png_byte)(i & 0xff);
}

// Custom implementation of png_save_uint_16 to ensure network byte order (big-endian) writing.
static void custom_png_save_uint_16(png_bytep buf, png_uint_16 i)
{
    buf[0] = (png_byte)((i >> 8) & 0xff);
    buf[1] = (png_byte)(i & 0xff);
}

static void png_read_data(png_structp png_ptr, png_bytep area, png_size_t size)
{
    SDL_IOStream *src = (SDL_IOStream *)lib.png_get_io_ptr(png_ptr);
    if (SDL_ReadIO(src, area, size) != size) {
        lib.png_error(png_ptr, "Failed to read all expected data from SDL_IOStream for PNG image.");
    }
}

static void png_write_data(png_structp png_ptr, png_bytep src, png_size_t size)
{
    SDL_IOStream *dst = (SDL_IOStream *)lib.png_get_io_ptr(png_ptr);
    if (SDL_WriteIO(dst, src, size) != size) {
        lib.png_error(png_ptr, "Failed to write all expected data to SDL_IOStream for PNG image.");
    }
}

static void png_flush_data(png_structp png_ptr)
{
    lib.png_write_flush(png_ptr);
}

struct png_load_vars
{
    const char *error;
    SDL_Surface *surface;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    png_colorp color_ptr;
    SDL_Surface *source_surface_for_save;

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    Uint32 format;
    unsigned char header[8];
    png_colorp png_palette;
    int num_palette;
    png_bytep trans;
    int num_trans;
    png_color_16p trans_values;
};

static bool LIBPNG_LoadPNG_IO_Internal(SDL_IOStream *src, struct png_load_vars *vars)
{
    if (SDL_ReadIO(src, vars->header, sizeof(vars->header)) != sizeof(vars->header)) {
        vars->error = "Failed to read PNG header from SDL_IOStream";
        return false;
    }
    if (lib.png_sig_cmp(vars->header, 0, 8)) {
        vars->error = "Not a valid PNG file signature";
        return false;
    }

    vars->png_ptr = lib.png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (vars->png_ptr == NULL) {
        vars->error = "Couldn't allocate memory for PNG read struct";
        return false;
    }
    vars->info_ptr = lib.png_create_info_struct(vars->png_ptr);
    if (vars->info_ptr == NULL) {
        vars->error = "Couldn't create image information for PNG file";
        return false;
    }
#ifndef LIBPNG_VERSION_12
    if (setjmp(*lib.png_set_longjmp_fn(vars->png_ptr, longjmp, sizeof(jmp_buf))))
#else
    if (setjmp(vars->png_ptr->jmpbuf))
#endif
    {
        vars->error = "Error during PNG read operation";
        return false;
    }

    lib.png_set_read_fn(vars->png_ptr, src, png_read_data);
    lib.png_set_sig_bytes(vars->png_ptr, 8);

    lib.png_read_info(vars->png_ptr, vars->info_ptr);
    lib.png_get_IHDR(vars->png_ptr, vars->info_ptr, &vars->width, &vars->height, &vars->bit_depth,
                     &vars->color_type, &vars->interlace_type, NULL, NULL);

    // Only convert non-palette formats to RGB/RGBA
    // TODO: Convert this to a colour key - the specs say that there's
    // only one transparent colour for non-palette images
    if (vars->color_type != PNG_COLOR_TYPE_PALETTE) {
        if (lib.png_get_valid(vars->png_ptr, vars->info_ptr, PNG_INFO_tRNS)) {
            lib.png_set_tRNS_to_alpha(vars->png_ptr);
            vars->color_type |= PNG_COLOR_MASK_ALPHA;
        }
    }

    /* SDL doesn't currently support this format, so we convert to RGB for now. */
    if (vars->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        lib.png_set_gray_to_rgb(vars->png_ptr);
    }

    if (vars->color_type == PNG_COLOR_TYPE_PALETTE ||
        vars->color_type == PNG_COLOR_TYPE_GRAY) {
        if (vars->bit_depth == 1) {
            vars->format = SDL_PIXELFORMAT_INDEX1MSB;
        } else if (vars->bit_depth == 2) {
            vars->format = SDL_PIXELFORMAT_INDEX2MSB;
        } else if (vars->bit_depth == 4) {
            vars->format = SDL_PIXELFORMAT_INDEX4MSB;
        } else /* if (vars->bit_depth == 8) */ {
            vars->format = SDL_PIXELFORMAT_INDEX8;
        }
    } else if (vars->bit_depth == 16) {
        if (vars->color_type & PNG_COLOR_MASK_ALPHA) {
            vars->format = SDL_PIXELFORMAT_RGBA64;
        } else {
            vars->format = SDL_PIXELFORMAT_RGB48;
        }
    } else {
        if (vars->color_type & PNG_COLOR_MASK_ALPHA) {
            vars->format = SDL_PIXELFORMAT_RGBA32;
        } else {
            vars->format = SDL_PIXELFORMAT_RGB24;
        }
    }

    lib.png_read_update_info(vars->png_ptr, vars->info_ptr);
    lib.png_get_IHDR(vars->png_ptr, vars->info_ptr, &vars->width, &vars->height, &vars->bit_depth,
                     &vars->color_type, &vars->interlace_type, NULL, NULL);

    vars->surface = SDL_CreateSurface(vars->width, vars->height, vars->format);
    if (vars->surface == NULL) {
        vars->error = SDL_GetError();
        return false;
    }

    // For palette images, set up the palette
    if (vars->color_type == PNG_COLOR_TYPE_PALETTE) {
        if (lib.png_get_PLTE(vars->png_ptr, vars->info_ptr, &vars->png_palette, &vars->num_palette)) {
            SDL_Palette *palette = SDL_CreateSurfacePalette(vars->surface);
            if (!palette) {
                vars->error = "Failed to create palette for PNG image";
                return false;
            }

            for (int i = 0; i < palette->ncolors; i++) {
                palette->colors[i].r = vars->png_palette[i].red;
                palette->colors[i].g = vars->png_palette[i].green;
                palette->colors[i].b = vars->png_palette[i].blue;
                palette->colors[i].a = 255;
            }

            if (lib.png_get_tRNS(vars->png_ptr, vars->info_ptr, &vars->trans, &vars->num_trans, &vars->trans_values)) {
                for (int i = 0; i < vars->num_trans && i < palette->ncolors; i++) {
                    palette->colors[i].a = vars->trans[i];
                }
                SDL_SetSurfaceBlendMode(vars->surface, SDL_BLENDMODE_BLEND);
            }
        }
    } else if (vars->color_type == PNG_COLOR_TYPE_GRAY) {
        SDL_Palette *palette = SDL_CreateSurfacePalette(vars->surface);
        if (!palette) {
            vars->error = "Failed to create palette for PNG image";
            return false;
        }

        for (int i = 0; i < palette->ncolors; i++) {
            palette->colors[i].r = (i * 255) / palette->ncolors;
            palette->colors[i].g = (i * 255) / palette->ncolors;
            palette->colors[i].b = (i * 255) / palette->ncolors;
            palette->colors[i].a = 255;
        }
    }

    vars->row_pointers = (png_bytep *)SDL_malloc(sizeof(png_bytep) * vars->height);
    if (!vars->row_pointers) {
        vars->error = "Out of memory allocating row pointers";
        return false;
    }
    for (png_uint_32 y = 0; y < vars->height; y++) {
        vars->row_pointers[y] = (png_bytep)((Uint8 *)vars->surface->pixels + y * (size_t)vars->surface->pitch);
    }

    lib.png_read_image(vars->png_ptr, vars->row_pointers);

#if SDL_BYTEORDER != SDL_BIG_ENDIAN
    if (vars->format == SDL_PIXELFORMAT_RGBA64) {
        Uint16 *pixels = (Uint16 *)vars->surface->pixels;
        int num_pixels = vars->width * vars->height * 4;
        for (int i = 0; i < num_pixels; i++) {
            pixels[i] = SDL_Swap16(pixels[i]);
        }
    }
#endif

    png_textp text_ptr = NULL;
    int num_text = 0;
    if (lib.png_get_text(vars->png_ptr, vars->info_ptr, &text_ptr, &num_text) > 0) {
        for (int i = 0; i < num_text; ++i, ++text_ptr) {
            if (SDL_strcmp(text_ptr->key, "XML:com.adobe.xmp") == 0) {
                // Look for tiff:Orientation in the XMP data
                int orientation;
                const char *value = SDL_strstr(text_ptr->text, "tiff:Orientation=\"");
                if (value) {
                    value += 18;
                    orientation = (*value - '0');
                    vars->surface = IMG_ApplyOrientation(vars->surface, orientation);
                    if (!vars->surface) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

SDL_Surface *IMG_LoadPNG_LIBPNG(SDL_IOStream *src)
{
    Sint64 start_pos;
    bool success = false;

    if (!src) {
        SDL_SetError("SDL_IOStream is NULL");
        return NULL;
    }

    start_pos = SDL_TellIO(src);

    struct png_load_vars vars;
    SDL_zero(vars);

    success = LIBPNG_LoadPNG_IO_Internal(src, &vars);

    if (vars.png_ptr) {
        lib.png_destroy_read_struct(&vars.png_ptr,
                                    vars.info_ptr ? &vars.info_ptr : (png_infopp)NULL,
                                    (png_infopp)NULL);
    }
    if (vars.row_pointers) {
        SDL_free(vars.row_pointers);
    }

    if (success) {
        return vars.surface;
    } else {
        SDL_SeekIO(src, start_pos, SDL_IO_SEEK_SET);
        if (vars.surface) {
            SDL_DestroySurface(vars.surface);
        }
        if (vars.error) {
            SDL_SetError("%s", vars.error);
        }
        return NULL;
    }
}

#if SAVE_PNG

struct png_save_vars
{
    const char *error;
    SDL_Surface *surface;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    png_colorp color_ptr;
    SDL_Surface *source_surface_for_save;

    Uint8 transparent_table[256];
    SDL_Palette *palette;
    int png_color_type;
    int bit_depth; // default to 8
};

static bool LIBPNG_SavePNG_IO_Internal(struct png_save_vars *vars, SDL_Surface *surface, SDL_IOStream *dst)
{
    vars->source_surface_for_save = surface;

    vars->png_ptr = lib.png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (vars->png_ptr == NULL) {
        vars->error = "Couldn't allocate memory for PNG write struct";
        return false;
    }
    vars->info_ptr = lib.png_create_info_struct(vars->png_ptr);
    if (vars->info_ptr == NULL) {
        vars->error = "Couldn't create image information for PNG file";
        return false;
    }

#ifndef LIBPNG_VERSION_12
    if (setjmp(*lib.png_set_longjmp_fn(vars->png_ptr, longjmp, sizeof(jmp_buf))))
#else
    if (setjmp(vars->png_ptr->jmpbuf))
#endif
    {
        vars->error = "Error during PNG write operation";
        return false;
    }

    lib.png_set_write_fn(vars->png_ptr, dst, png_write_data, png_flush_data);

    vars->palette = SDL_GetSurfacePalette(surface);
    if (vars->palette) {
        const int ncolors = vars->palette->ncolors;
        int i;
        int last_transparent = -1;

        vars->color_ptr = (png_colorp)SDL_malloc(sizeof(png_color) * ncolors);
        if (vars->color_ptr == NULL) {
            vars->error = "Couldn't allocate palette for PNG file";
            return false;
        }
        for (i = 0; i < ncolors; i++) {
            vars->color_ptr[i].red = vars->palette->colors[i].r;
            vars->color_ptr[i].green = vars->palette->colors[i].g;
            vars->color_ptr[i].blue = vars->palette->colors[i].b;
            if (vars->palette->colors[i].a != 255) {
                last_transparent = i;
            }
        }
        lib.png_set_PLTE(vars->png_ptr, vars->info_ptr, vars->color_ptr, ncolors);
        vars->png_color_type = PNG_COLOR_TYPE_PALETTE;

        if (last_transparent >= 0) {
            for (i = 0; i <= last_transparent; ++i) {
                vars->transparent_table[i] = vars->palette->colors[i].a;
            }
            lib.png_set_tRNS(vars->png_ptr, vars->info_ptr, vars->transparent_table, last_transparent + 1, NULL);
        }
    } else if (surface->format == SDL_PIXELFORMAT_RGB24) {
        vars->png_color_type = PNG_COLOR_TYPE_RGB;
    } else if (!SDL_ISPIXELFORMAT_ALPHA(surface->format)) {
        vars->png_color_type = PNG_COLOR_TYPE_RGB;
        vars->source_surface_for_save = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGB24);
        if (!vars->source_surface_for_save) {
            vars->error = SDL_GetError();
            return false;
        }
    } else {
        vars->png_color_type = PNG_COLOR_TYPE_RGBA;
        vars->source_surface_for_save = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (!vars->source_surface_for_save) {
            vars->error = SDL_GetError();
            return false;
        }
    }

    lib.png_set_IHDR(vars->png_ptr, vars->info_ptr, vars->source_surface_for_save->w, vars->source_surface_for_save->h,
                     vars->bit_depth, vars->png_color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    lib.png_write_info(vars->png_ptr, vars->info_ptr);

    vars->row_pointers = (png_bytep *)SDL_malloc(sizeof(png_bytep) * vars->source_surface_for_save->h);
    if (!vars->row_pointers) {
        vars->error = "Out of memory allocating row pointers";
        return false;
    }
    for (int row = 0; row < (int)vars->source_surface_for_save->h; row++) {
        vars->row_pointers[row] = (png_bytep)((Uint8 *)vars->source_surface_for_save->pixels + row * (size_t)vars->source_surface_for_save->pitch);
    }

    lib.png_write_image(vars->png_ptr, vars->row_pointers);
    lib.png_write_end(vars->png_ptr, vars->info_ptr);

    return true;
}

bool IMG_SavePNG_LIBPNG(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    if (!surface || !dst) {
        SDL_SetError("Surface or SDL_IOStream is NULL");
        return false;
    }

    struct png_save_vars vars;
    bool result = false;

    SDL_zero(vars);
    vars.bit_depth = 8;

    result = LIBPNG_SavePNG_IO_Internal(&vars, surface, dst);

    if (vars.png_ptr) {
        lib.png_destroy_write_struct(&vars.png_ptr, &vars.info_ptr);
    }
    if (vars.color_ptr) {
        SDL_free(vars.color_ptr);
    }
    if (vars.row_pointers) {
        SDL_free(vars.row_pointers);
    }
    if (vars.source_surface_for_save && vars.source_surface_for_save != surface) {
        SDL_DestroySurface(vars.source_surface_for_save);
    }

    if (!result && vars.error) {
        SDL_SetError("%s", vars.error);
    }

    if (closeio) {
        result &= SDL_CloseIO(dst);
    }

    return result;
}

#endif // SAVE_PNG

typedef struct
{
    png_uint_32 num_frames;
    png_uint_32 num_plays;
} apng_acTL_chunk;

typedef struct
{
    png_uint_32 sequence_number;
    png_uint_32 width;
    png_uint_32 height;
    png_uint_32 x_offset;
    png_uint_32 y_offset;
    png_uint_16 delay_num;
    png_uint_16 delay_den;
    png_byte dispose_op;
    png_byte blend_op;
    png_bytep raw_idat_data;
    png_size_t raw_idat_size;
} apng_fcTL_chunk;

typedef struct
{
    SDL_IOStream *stream;
    apng_acTL_chunk actl;
    apng_fcTL_chunk *fctl_frames;
    int fctl_count;
    int fctl_capacity;
    bool is_apng;
    SDL_Surface *canvas;
    SDL_Surface *prev_canvas_copy;
    png_uint_32 current_idat_sequence;
} apng_read_context;

typedef struct
{
    SDL_IOStream *mem_stream;
    SDL_IOStream *read_stream;
    png_structp png_ptr;
    png_infop info_ptr;
    SDL_Surface *surface;
    png_bytep *row_pointers;
} DecompressionContext;

static SDL_Surface *decompress_png_frame_data(DecompressionContext *context, png_bytep compressed_data, png_size_t compressed_size,
                                              int width, int height, int png_color_type, int bit_depth, png_bytep chunk_PLTE, Uint32 size_PLTE, png_bytep chunk_tRNS, Uint32 size_tRNS)
{
    /*
     * Usually you'd directly decompress zlib but then we have to do defiltering and deinterlacing ourselves.
     * We can decompress zlib then pass those jobs to libpng but then libpng expects a fully compressed data,
     * therefore, we manually add chunks for header and other parts only for this given compressed data,
     * tricking libpng to believe this is a normal PNG file, then make it defilter (if any) and deinterlace
     * (if any) for us.
     */

    // Create a memory stream to hold our synthetic PNG
    context->mem_stream = SDL_IOFromDynamicMem();
    if (!context->mem_stream) {
        goto error;
    }

    // Write PNG signature
    if (SDL_WriteIO(context->mem_stream, png_sig, 8) != 8) {
        goto error;
    }

    // Write IHDR chunk
    {
        png_byte ihdr_data[13];
        png_byte ihdr_header[8] = { 0, 0, 0, 13, 'I', 'H', 'D', 'R' };

        // Write IHDR length and type
        if (SDL_WriteIO(context->mem_stream, ihdr_header, 8) != 8) {
            goto error;
        }

        // Write IHDR data
        custom_png_save_uint_32(ihdr_data, width);
        custom_png_save_uint_32(ihdr_data + 4, height);
        ihdr_data[8] = bit_depth;
        ihdr_data[9] = png_color_type;
        ihdr_data[10] = PNG_INTERLACE_NONE;
        ihdr_data[11] = PNG_COMPRESSION_TYPE_DEFAULT;
        ihdr_data[12] = PNG_FILTER_TYPE_DEFAULT;

        if (SDL_WriteIO(context->mem_stream, ihdr_data, 13) != 13) {
            goto error;
        }

        // Calculate and write IHDR CRC
        png_uint_32 crc = SDL_crc32(0, (Uint8 *)"IHDR", 4);
        crc = SDL_crc32(crc, ihdr_data, 13);
        png_byte crc_bytes[4];
        custom_png_save_uint_32(crc_bytes, crc);
        if (SDL_WriteIO(context->mem_stream, crc_bytes, 4) != 4) {
            goto error;
        }
    }

    // Write PLTE chunk
    if (chunk_PLTE) {
        if (SDL_WriteIO(context->mem_stream, chunk_PLTE, size_PLTE) != size_PLTE) {
            goto error;
        }
    }

    // Write tRNS chunk
    if (chunk_tRNS) {
        if (SDL_WriteIO(context->mem_stream, chunk_tRNS, size_tRNS) != size_tRNS) {
            goto error;
        }
    }

    // Write IDAT chunk
    {
        png_byte idat_header[8] = { 0, 0, 0, 0, 'I', 'D', 'A', 'T' };
        custom_png_save_uint_32(idat_header, (png_uint_32)compressed_size);

        // Write IDAT length and type
        if (SDL_WriteIO(context->mem_stream, idat_header, 8) != 8) {
            goto error;
        }

        // Write compressed data
        if (SDL_WriteIO(context->mem_stream, compressed_data, compressed_size) != compressed_size) {
            goto error;
        }

        // Calculate and write IDAT CRC
        png_uint_32 crc = SDL_crc32(0, (Uint8 *)"IDAT", 4);
        crc = SDL_crc32(crc, compressed_data, compressed_size);
        png_byte crc_bytes[4];
        custom_png_save_uint_32(crc_bytes, crc);
        if (SDL_WriteIO(context->mem_stream, crc_bytes, 4) != 4) {
            goto error;
        }
    }

    // Write IEND chunk
    {
        png_byte iend_chunk[12] = {
            0, 0, 0, 0,            // Length (0)
            'I', 'E', 'N', 'D',    // Type
            0xAE, 0x42, 0x60, 0x82 // CRC (precomputed for empty IEND)
        };

        if (SDL_WriteIO(context->mem_stream, iend_chunk, 12) != 12) {
            goto error;
        }
    }

    Sint64 data_size = SDL_TellIO(context->mem_stream);
    if (data_size < 0) {
        goto error;
    }
    if (data_size >= SDL_MAX_SINT32) {
        SDL_SetError("data size >= INT32_MAX");
        goto error;
    }
    void *buffer = NULL;

    if (SDL_SeekIO(context->mem_stream, 0, SDL_IO_SEEK_SET) < 0) {
        goto error;
    }

    buffer = SDL_malloc(data_size);
    if (!buffer) {
        goto error;
    }

    if (SDL_ReadIO(context->mem_stream, buffer, data_size) != (size_t)data_size) {
        SDL_free(buffer);
        goto error;
    }

    context->read_stream = SDL_IOFromConstMem(buffer, data_size);
    if (!context->read_stream) {
        SDL_free(buffer);
        goto error;
    }

    // Now we have a proper PNG file in memory, use libpng to read it
    context->png_ptr = lib.png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!context->png_ptr) {
        SDL_free(buffer);
        goto error;
    }

    context->info_ptr = lib.png_create_info_struct(context->png_ptr);
    if (!context->info_ptr) {
        SDL_free(buffer);
        goto error;
    }

#ifndef LIBPNG_VERSION_12
    if (setjmp(*lib.png_set_longjmp_fn(context->png_ptr, longjmp, sizeof(jmp_buf))))
#else
    if (setjmp(context->png_ptr->jmpbuf))
#endif
    {
        SDL_SetError("Error during PNG read");
        SDL_free(buffer);
        goto error;
    }

    lib.png_set_read_fn(context->png_ptr, context->read_stream, png_read_data);
    lib.png_read_info(context->png_ptr, context->info_ptr);

    if (png_color_type == PNG_COLOR_TYPE_PALETTE) {
        lib.png_set_palette_to_rgb(context->png_ptr);
    }

    if (bit_depth == 16) {
        lib.png_set_strip_16(context->png_ptr);
    }

    if (!(png_color_type & PNG_COLOR_MASK_ALPHA)) {
        lib.png_set_filler(context->png_ptr, 0xFF, PNG_FILLER_AFTER);
    }

    lib.png_read_update_info(context->png_ptr, context->info_ptr);

    context->surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (!context->surface) {
        SDL_free(buffer);
        goto error;
    }

    context->row_pointers = (png_bytep *)SDL_malloc(height * sizeof(png_bytep));
    if (!context->row_pointers) {
        SDL_free(buffer);
        goto error;
    }

    for (int y = 0; y < height; y++) {
        context->row_pointers[y] = (png_bytep)((Uint8 *)context->surface->pixels + y * (size_t)context->surface->pitch);
    }

    lib.png_read_image(context->png_ptr, context->row_pointers);

    SDL_free(context->row_pointers);
    lib.png_destroy_read_struct(&context->png_ptr, &context->info_ptr, NULL);
    SDL_CloseIO(context->read_stream);
    SDL_CloseIO(context->mem_stream);
    SDL_free(buffer);

    return context->surface;

error:
    if (context->row_pointers) {
        SDL_free(context->row_pointers);
    }
    if (context->png_ptr) {
        lib.png_destroy_read_struct(&context->png_ptr, context->info_ptr ? &context->info_ptr : NULL, NULL);
    }
    if (context->surface) {
        SDL_DestroySurface(context->surface);
    }
    if (context->read_stream) {
        SDL_CloseIO(context->read_stream);
    }
    if (context->mem_stream) {
        SDL_CloseIO(context->mem_stream);
    }

    return NULL;
}

static bool read_png_chunk(SDL_IOStream *stream, png_bytep *chunk, Uint32 *chunk_size, char *chunk_type, png_bytep *data, Uint32 *data_length)
{
    Uint8 header[8];

    // Read chunk header (8 bytes)
    if (SDL_ReadIO(stream, &header, sizeof(header)) != sizeof(header)) {
        return false;
    }

    // Get data length (4 bytes, big-endian)
    SDL_memcpy(data_length, header, 4);
    *data_length = SDL_Swap32BE(*data_length);

    // Get chunk type (4 bytes)
    SDL_memcpy(chunk_type, header+4, 4);

    // Allocate memory for chunk
    *chunk_size = sizeof(header) + *data_length + 4;
    *chunk = (png_bytep)SDL_malloc(*chunk_size);
    if (!*chunk) {
        return false;
    }
    SDL_memcpy(*chunk, header, sizeof(header));
    *data = *chunk + sizeof(header);

    // Read chunk data
    if (*data_length > 0) {
        if (SDL_ReadIO(stream, *data, *data_length) != *data_length) {
            SDL_free(*chunk);
            return false;
        }
    }

    // Read CRC (4 bytes, big-endian)
    if (SDL_ReadIO(stream, *data + *data_length, 4) != 4) {
        SDL_free(*chunk);
        return false;
    }

    return true;
}

struct IMG_AnimationDecoderContext
{
    apng_acTL_chunk actl;
    apng_fcTL_chunk *fctl_frames;
    int fctl_count;
    int fctl_capacity;
    bool is_apng;
    SDL_Surface *canvas;
    SDL_Surface *prev_canvas_copy;
    int current_frame_index;

    int width;
    int height;
    int bit_depth;
    int png_color_type;

    SDL_Palette *palette;
    png_bytep chunk_PLTE;
    Uint32 size_PLTE;
    png_bytep chunk_tRNS;
    Uint32 size_tRNS;
};

static bool IMG_AnimationDecoderReset_Internal(IMG_AnimationDecoder *decoder)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;

    ctx->current_frame_index = 0;
    if (SDL_SeekIO(decoder->src, decoder->start, SDL_IO_SEEK_SET) < 0) {
        return SDL_SetError("Failed to seek to beginning of APNG animation");
    }

    if (ctx->canvas) {
        SDL_FillSurfaceRect(ctx->canvas, NULL, 0x00000000);
    }

    if (ctx->prev_canvas_copy) {
        SDL_FillSurfaceRect(ctx->prev_canvas_copy, NULL, 0x00000000);
    }

    return true;
}

static bool IMG_AnimationDecoderGetNextFrame_Internal(IMG_AnimationDecoder *decoder, SDL_Surface **frame, Uint64 *duration)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;
    if (!ctx->is_apng) {
        return false;
    }

    if (ctx->actl.num_frames - ctx->current_frame_index < 1) {
        decoder->status = IMG_DECODER_STATUS_COMPLETE;
        return false;
    }

    if (!ctx->canvas) {
        ctx->canvas = SDL_CreateSurface(ctx->width, ctx->height, SDL_PIXELFORMAT_RGBA32);
        if (!ctx->canvas) {
            return SDL_SetError("Failed to create APNG canvas");
        }
        if (!SDL_SetSurfaceBlendMode(ctx->canvas, SDL_BLENDMODE_BLEND)) {
            return SDL_SetError("Failed to set APNG canvas blend mode");
        }
        if (!SDL_FillSurfaceRect(ctx->canvas, NULL, 0x00000000)) {
            return SDL_SetError("Failed to fill APNG canvas");
        }
    }

    if (!ctx->prev_canvas_copy) {
        ctx->prev_canvas_copy = SDL_CreateSurface(ctx->width, ctx->height, SDL_PIXELFORMAT_RGBA32);
        if (!ctx->prev_canvas_copy) {
            return SDL_SetError("Failed to create previous canvas copy");
        }
        if (!SDL_FillSurfaceRect(ctx->prev_canvas_copy, NULL, 0x00000000)) {
            return SDL_SetError("Failed to fill previous canvas copy");
        }
    }

    SDL_Surface *retval = NULL;
    apng_fcTL_chunk *fctl = &ctx->fctl_frames[ctx->current_frame_index];
    *duration = IMG_GetDecoderDuration(decoder, fctl->delay_num, fctl->delay_den);

    if (ctx->current_frame_index > 0) {
        apng_fcTL_chunk *prev_fctl = &ctx->fctl_frames[ctx->current_frame_index - 1];
        SDL_Rect prev_frame_rect = {
            (int)prev_fctl->x_offset,
            (int)prev_fctl->y_offset,
            (int)prev_fctl->width,
            (int)prev_fctl->height
        };

        switch (prev_fctl->dispose_op) {
        case PNG_DISPOSE_OP_NONE:
            // Do nothing
            break;
        case PNG_DISPOSE_OP_BACKGROUND:
            if (!SDL_FillSurfaceRect(ctx->canvas, &prev_frame_rect, 0x00000000)) {
                return SDL_SetError("Failed to fill canvas for background dispose operation");
            }
            break;
        case PNG_DISPOSE_OP_PREVIOUS:
            if (!SDL_BlitSurface(ctx->prev_canvas_copy, NULL, ctx->canvas, NULL)) {
                return SDL_SetError("Failed to restore previous canvas copy for dispose operation");
            }
            break;
        }
    }

    if (fctl->dispose_op == PNG_DISPOSE_OP_PREVIOUS) {
        if (!SDL_BlitSurface(ctx->canvas, NULL, ctx->prev_canvas_copy, NULL)) {
            return SDL_SetError("Failed to copy current canvas to previous canvas copy");
        }
    }

    DecompressionContext decompressionContext;
    SDL_zero(decompressionContext);
    SDL_Surface *temp_frame = decompress_png_frame_data(
        &decompressionContext,
        fctl->raw_idat_data,
        fctl->raw_idat_size,
        fctl->width,
        fctl->height,
        ctx->png_color_type,
        ctx->bit_depth,
        ctx->chunk_PLTE,
        ctx->size_PLTE,
        ctx->chunk_tRNS,
        ctx->size_tRNS);

    if (!temp_frame) {
        return SDL_SetError("Failed to decompress PNG frame data: %s", SDL_GetError());
    }

    if (temp_frame->format == SDL_PIXELFORMAT_INDEX8) {
        SDL_SetSurfacePalette(temp_frame, ctx->palette);
    }

    switch (fctl->blend_op) {
    case PNG_BLEND_OP_SOURCE:
        if (!SDL_SetSurfaceBlendMode(temp_frame, SDL_BLENDMODE_NONE)) {
            SDL_DestroySurface(temp_frame);
            return SDL_SetError("Failed to set blend mode for frame: %s", SDL_GetError());
        }
        break;
    case PNG_BLEND_OP_OVER:
        if (!SDL_SetSurfaceBlendMode(temp_frame, SDL_BLENDMODE_BLEND)) {
            SDL_DestroySurface(temp_frame);
            return SDL_SetError("Failed to set blend mode for frame: %s", SDL_GetError());
        }
        break;
    }

    SDL_Rect dest_rect = {
        (int)fctl->x_offset,
        (int)fctl->y_offset,
        (int)fctl->width,
        (int)fctl->height
    };

    if (!SDL_BlitSurface(temp_frame, NULL, ctx->canvas, &dest_rect)) {
        SDL_DestroySurface(temp_frame);
        return SDL_SetError("Failed to blit frame onto canvas: %s", SDL_GetError());
    }
    SDL_DestroySurface(temp_frame);

    retval = SDL_DuplicateSurface(ctx->canvas);
    if (!retval) {
        return false;
    }

    ++ctx->current_frame_index;

    *frame = retval;
    return true;
}

static bool IMG_AnimationDecoderClose_Internal(IMG_AnimationDecoder *decoder)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;

    if (ctx->fctl_frames) {
        for (int i = 0; i < ctx->fctl_count; i++) {
            if (ctx->fctl_frames[i].raw_idat_data) {
                SDL_free(ctx->fctl_frames[i].raw_idat_data);
            }
        }
        SDL_free(ctx->fctl_frames);
    }

    SDL_DestroyPalette(ctx->palette);
    SDL_free(ctx->chunk_PLTE);
    SDL_free(ctx->chunk_tRNS);
    SDL_DestroySurface(ctx->canvas);
    SDL_DestroySurface(ctx->prev_canvas_copy);

    SDL_free(ctx);
    decoder->ctx = NULL;
    return true;
}

bool IMG_CreateAPNGAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    if (!IMG_InitPNG()) {
        return false;
    }

    IMG_AnimationDecoderContext *ctx = (IMG_AnimationDecoderContext *)SDL_calloc(1, sizeof(IMG_AnimationDecoderContext));
    if (!ctx) {
        return SDL_SetError("Out of memory for APNG decoder context");
    }

    decoder->ctx = ctx;

    unsigned char header[8];
    if (SDL_ReadIO(decoder->src, header, sizeof(header)) != sizeof(header)) {
        SDL_SetError("Failed to read PNG header");
        IMG_AnimationDecoderClose_Internal(decoder);
        return false;
    }

    if (lib.png_sig_cmp(header, 0, 8)) {
        SDL_SetError("Not a valid PNG file signature");
        IMG_AnimationDecoderClose_Internal(decoder);
        return false;
    }

    // Extracted metadata will be assigned to variables below
    char *desc = NULL;
    char *rights = NULL;
    char *title = NULL;
    char *author = NULL;
    char *creationtime = NULL;

    bool found_iend = false;
    while (!found_iend) {
        char chunk_type[5] = { 0 };
        png_bytep chunk = NULL;
        Uint32 chunk_size;
        png_bytep chunk_data = NULL;
        Uint32 chunk_length = 0;
        bool chunk_saved = false;

        if (!read_png_chunk(decoder->src, &chunk, &chunk_size, chunk_type, &chunk_data, &chunk_length)) {
            IMG_AnimationDecoderClose_Internal(decoder);
            return false;
        }

        if (chunk_length > SDL_MAX_SINT32) {
            SDL_SetError("APNG chunk too large to process");
            SDL_free(chunk);
            IMG_AnimationDecoderClose_Internal(decoder);
            return false;
        }

        if (SDL_memcmp(chunk_type, "IHDR", 4) == 0) {
            if (chunk_length != 13) {
                SDL_SetError("Invalid IHDR chunk size");
                SDL_free(chunk);
                IMG_AnimationDecoderClose_Internal(decoder);
                return false;
            }

            // Extract image dimensions from IHDR
            ctx->width = SDL_Swap32BE(*(Uint32 *)chunk_data);
            ctx->height = SDL_Swap32BE(*(Uint32 *)(chunk_data + 4));
            ctx->bit_depth = *(Uint8 *)(chunk_data + 8);
            ctx->png_color_type = *(Uint8 *)(chunk_data + 9);

        } else if (SDL_memcmp(chunk_type, "acTL", 4) == 0) {
            if (chunk_length != 8) {
                SDL_SetError("Invalid acTL chunk size");
                SDL_free(chunk);
                IMG_AnimationDecoderClose_Internal(decoder);
                return false;
            }

            ctx->is_apng = true;
            ctx->actl.num_frames = SDL_Swap32BE(*(Uint32 *)chunk_data);
            ctx->actl.num_plays = SDL_Swap32BE(*(Uint32 *)(chunk_data + 4));

        } else if (SDL_memcmp(chunk_type, "PLTE", 4) == 0) {
            int num_entries = (int)chunk_length / 3;
            if (num_entries > 0 && num_entries <= 256) {
                SDL_DestroyPalette(ctx->palette);

                ctx->palette = SDL_CreatePalette(num_entries);
                if (!ctx->palette) {
                    SDL_free(chunk);
                    IMG_AnimationDecoderClose_Internal(decoder);
                    return false;
                }

                for (int i = 0; i < num_entries; i++) {
                    ctx->palette->colors[i].r = chunk_data[i * 3];
                    ctx->palette->colors[i].g = chunk_data[i * 3 + 1];
                    ctx->palette->colors[i].b = chunk_data[i * 3 + 2];
                    ctx->palette->colors[i].a = SDL_ALPHA_OPAQUE;
                }
            }

            SDL_free(ctx->chunk_PLTE);
            ctx->chunk_PLTE = chunk;
            ctx->size_PLTE = chunk_size;
            chunk_saved = true;

        } else if (SDL_memcmp(chunk_type, "tRNS", 4) == 0) {
            if (ctx->palette) {
                int num_trans = SDL_min((int)chunk_size, ctx->palette->ncolors);
                for (int i = 0; i < num_trans; i++) {
                    ctx->palette->colors[i].a = chunk_data[i];
                }
            }

            SDL_free(ctx->chunk_tRNS);
            ctx->chunk_tRNS = chunk;
            ctx->size_tRNS = chunk_size;
            chunk_saved = true;

        } else if (SDL_memcmp(chunk_type, "fcTL", 4) == 0) {
            if (chunk_length != 26) {
                SDL_SetError("Invalid fcTL chunk size");
                SDL_free(chunk);
                IMG_AnimationDecoderClose_Internal(decoder);
                return false;
            }

            if (ctx->fctl_count >= ctx->fctl_capacity) {
                ctx->fctl_capacity = ctx->fctl_capacity == 0 ? 4 : ctx->fctl_capacity * 2;
                ctx->fctl_frames = (apng_fcTL_chunk *)SDL_realloc(ctx->fctl_frames,
                                                                  sizeof(apng_fcTL_chunk) * ctx->fctl_capacity);
                if (!ctx->fctl_frames) {
                    SDL_SetError("Out of memory for fcTL chunks");
                    SDL_free(chunk);
                    IMG_AnimationDecoderClose_Internal(decoder);
                    return false;
                }
            }

            apng_fcTL_chunk *fctl = &ctx->fctl_frames[ctx->fctl_count];
            fctl->sequence_number = SDL_Swap32BE(*(Uint32 *)chunk_data);
            fctl->width = SDL_Swap32BE(*(Uint32 *)(chunk_data + 4));
            fctl->height = SDL_Swap32BE(*(Uint32 *)(chunk_data + 8));
            fctl->x_offset = SDL_Swap32BE(*(Uint32 *)(chunk_data + 12));
            fctl->y_offset = SDL_Swap32BE(*(Uint32 *)(chunk_data + 16));
            fctl->delay_num = SDL_Swap16BE(*(Uint16 *)(chunk_data + 20));
            fctl->delay_den = SDL_Swap16BE(*(Uint16 *)(chunk_data + 22));
            fctl->dispose_op = chunk_data[24];
            fctl->blend_op = chunk_data[25];
            fctl->raw_idat_data = NULL;
            fctl->raw_idat_size = 0;
            ctx->fctl_count++;

        } else if (SDL_memcmp(chunk_type, "IDAT", 4) == 0) {
            // Find fcTL with sequence number 0 (which corresponds to IDAT data)
            int matching_fctl_index = -1;
            for (int i = 0; i < ctx->fctl_count; ++i) {
                if (ctx->fctl_frames[i].sequence_number == 0) {
                    matching_fctl_index = i;
                    break;
                }
            }

            if (matching_fctl_index >= 0) {
                apng_fcTL_chunk *fctl = &ctx->fctl_frames[matching_fctl_index];

                // Allocate or reallocate buffer
                png_size_t new_size = fctl->raw_idat_size + chunk_length;
                if (new_size < fctl->raw_idat_size) {
                    SDL_SetError("IDAT size would overflow");
                    SDL_free(chunk);
                    IMG_AnimationDecoderClose_Internal(decoder);
                    return false;
                }

                png_bytep new_buffer = (png_bytep)SDL_realloc(fctl->raw_idat_data, new_size);
                if (!new_buffer) {
                    SDL_free(chunk);
                    IMG_AnimationDecoderClose_Internal(decoder);
                    return false;
                }

                fctl->raw_idat_data = new_buffer;
                SDL_memcpy(fctl->raw_idat_data + fctl->raw_idat_size, chunk_data, chunk_length);
                fctl->raw_idat_size = new_size;
            }
        } else if (SDL_memcmp(chunk_type, "fdAT", 4) == 0) {
            if (chunk_length < 4) {
                SDL_SetError("Invalid fdAT chunk size");
                SDL_free(chunk);
                IMG_AnimationDecoderClose_Internal(decoder);
                return false;
            }

            Uint32 sequence_number = SDL_Swap32BE(*(Uint32 *)chunk_data);

            // Find matching fcTL by sequence number (fdAT sequence - 1 matches fcTL sequence)
            int matching_fctl_index = -1;
            for (int i = 0; i < ctx->fctl_count; ++i) {
                if (ctx->fctl_frames[i].sequence_number == sequence_number - 1) {
                    matching_fctl_index = i;
                    break;
                }
            }

            if (matching_fctl_index >= 0) {
                apng_fcTL_chunk *fctl = &ctx->fctl_frames[matching_fctl_index];
                png_size_t new_size = fctl->raw_idat_size + (chunk_length - 4);

                if (new_size < fctl->raw_idat_size) {
                    SDL_SetError("fdAT size would overflow");
                    SDL_free(chunk);
                    IMG_AnimationDecoderClose_Internal(decoder);
                    return false;
                }

                png_bytep new_buffer = (png_bytep)SDL_realloc(fctl->raw_idat_data, new_size);
                if (!new_buffer) {
                    SDL_free(chunk);
                    IMG_AnimationDecoderClose_Internal(decoder);
                    return false;
                }

                fctl->raw_idat_data = new_buffer;
                SDL_memcpy(fctl->raw_idat_data + fctl->raw_idat_size, chunk_data + 4, chunk_length - 4);
                fctl->raw_idat_size = new_size;
            }
        } else if (SDL_memcmp(chunk_type, "IEND", 4) == 0) {
            found_iend = true;
        } else if (SDL_memcmp(chunk_type, "tEXt", 4) == 0) {
            char *separator = (char *)memchr(chunk_data, '\0', chunk_length);
            if (separator != NULL) {
                size_t keyword_len = separator - (char *)chunk_data;
                size_t text_len;
                if (keyword_len + 1 < chunk_length) {
                    text_len = chunk_length - (keyword_len + 1);
                } else {
                    text_len = chunk_length - keyword_len - 1;
                }

                char *keyword = (char *)SDL_malloc(keyword_len + 1);
                if (keyword) {
                    SDL_memcpy(keyword, chunk_data, keyword_len);
                    keyword[keyword_len] = '\0';
                    char *text = (char *)(separator + 1);
                    if (SDL_strcasecmp(keyword, "description") == 0) {
                        desc = SDL_strndup(text, text_len);
                    } else if (SDL_strcasecmp(keyword, "copyright") == 0) {
                        rights = SDL_strndup(text, text_len);
                    } else if (SDL_strcasecmp(keyword, "title") == 0) {
                        title = SDL_strndup(text, text_len);
                    } else if (SDL_strcasecmp(keyword, "author") == 0) {
                        author = SDL_strndup(text, text_len);
                    } else if (SDL_strcasecmp(keyword, "creation time") == 0) {
                        creationtime = SDL_strndup(text, text_len);
                    }
                    SDL_free(keyword);
                }
            }
        }

        if (!chunk_saved) {
            SDL_free(chunk);
        }
    }

    if (!ctx->is_apng || ctx->fctl_count == 0) {
        SDL_SetError("Not an APNG file or no frame control chunks found");
        IMG_AnimationDecoderClose_Internal(decoder);
        return false;
    }

    // Validate what might be missing or wrong
    if (ctx->bit_depth < 1 || ctx->png_color_type < 0 || ctx->width < 1 || ctx->height < 1) {
        SDL_SetError("Received invalid APNG with either corrupt or unspecified bit depth, color type, width or height");
        IMG_AnimationDecoderClose_Internal(decoder);
        return false;
    }

    decoder->GetNextFrame = IMG_AnimationDecoderGetNextFrame_Internal;
    decoder->Reset = IMG_AnimationDecoderReset_Internal;
    decoder->Close = IMG_AnimationDecoderClose_Internal;

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    if (!ignoreProps) {
        // Allow implicit properties to be set which are not globalized but specific to the decoder.
        SDL_SetNumberProperty(decoder->props, IMG_PROP_METADATA_FRAME_COUNT_NUMBER, ctx->actl.num_frames);

        // Set well-defined properties.
        SDL_SetNumberProperty(decoder->props, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, ctx->actl.num_plays);

        // Get other well-defined properties and set them in our props.
        if (desc) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_DESCRIPTION_STRING, desc);
            SDL_free(desc);
        }
        if (rights) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_COPYRIGHT_STRING, rights);
            SDL_free(rights);
        }
        if (title) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_TITLE_STRING, title);
            SDL_free(title);
        }
        if (author) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_AUTHOR_STRING, author);
            SDL_free(author);
        }
        if (creationtime) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_CREATION_TIME_STRING, creationtime);
            SDL_free(creationtime);
        }
    }

    return true;
}

#if SAVE_PNG

struct IMG_AnimationEncoderContext
{
    png_structp png_write_ptr;
    png_infop info_write_ptr;
    Sint64 acTL_chunk_start_pos;
    int current_frame_index; // This also serves as the sequence number for APNG chunks
    int apng_width;
    int apng_height;
    int compression_level;
    SDL_PixelFormat output_pixel_format;
    SDL_Palette *apng_palette_ptr;
    SDL_PropertiesID metadata;
};

static bool write_png_chunk(SDL_IOStream *stream, const char *chunk_type_str, png_bytep data, png_size_t size)
{
    png_byte crc_data[4];
    png_uint_32 crc;
    png_byte size_bytes[4];
    png_byte chunk_type[4];

    SDL_memcpy(chunk_type, chunk_type_str, 4);

    // Write chunk length
    custom_png_save_uint_32(size_bytes, (png_uint_32)size);
    if (SDL_WriteIO(stream, size_bytes, 4) != 4) {
        SDL_SetError("Failed to write chunk size for %s chunk", chunk_type_str);
        return false;
    }

    // Write chunk type
    if (SDL_WriteIO(stream, chunk_type, 4) != 4) {
        SDL_SetError("Failed to write chunk type for %s chunk", chunk_type_str);
        return false;
    }

    // Write chunk data (if any)
    if (data && size > 0) {
        if (SDL_WriteIO(stream, data, size) != size) {
            SDL_SetError("Failed to write chunk data for %s chunk", chunk_type_str);
            return false;
        }
    }

    // Calculate and write CRC
    crc = SDL_crc32(0L, NULL, 0);
    crc = SDL_crc32(crc, chunk_type, 4);
    if (data && size > 0) {
        crc = SDL_crc32(crc, data, size);
    }
    custom_png_save_uint_32(crc_data, crc);
    if (SDL_WriteIO(stream, crc_data, 4) != 4) {
        SDL_SetError("Failed to write chunk CRC for %s chunk", chunk_type_str);
        return false;
    }

    return true;
}

typedef struct
{
    png_structp temp_png_ptr;
    png_infop temp_info_ptr;
    SDL_IOStream *mem_stream;
    png_bytep full_zlib_data_buffer;
    png_bytep mem_buffer_ptr;
    png_bytep *row_pointers;
    bool iend_found;
    Sint64 current_pos;
    png_byte chunk_header[8];
    png_uint_32 chunk_len;
    png_byte chunk_type[4];
    Sint64 mem_buffer_size;
} CompressionContext;

static png_bytep compress_surface_to_png_data(CompressionContext *context, SDL_Surface *surface, png_size_t *compressed_size, int compression_level, int png_color_type)
{
    *compressed_size = 0; // Reset compressed_size to accumulate IDAT data

    // Create a growable memory buffer for the temporary PNG
    context->mem_stream = SDL_IOFromDynamicMem();
    if (!context->mem_stream) {
        SDL_SetError("Failed to create dynamic memory stream for PNG compression: %s", SDL_GetError());
        goto error;
    }

    context->temp_png_ptr = lib.png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!context->temp_png_ptr) {
        SDL_SetError("Couldn't allocate memory for temporary PNG write struct");
        goto error;
    }
    context->temp_info_ptr = lib.png_create_info_struct(context->temp_png_ptr);
    if (!context->temp_info_ptr) {
        SDL_SetError("Couldn't create temporary image information for PNG file");
        goto error;
    }

#ifndef LIBPNG_VERSION_12
    if (setjmp(*lib.png_set_longjmp_fn(context->temp_png_ptr, longjmp, sizeof(jmp_buf))))
#else
    if (setjmp(context->temp_png_ptr->jmpbuf))
#endif
    {
        SDL_SetError("Error during temporary PNG write operation for compression");
        goto error;
    }

    // png_io_context temp_io_context = { mem_stream };
    lib.png_set_write_fn(context->temp_png_ptr, context->mem_stream, png_write_data, png_flush_data);
    lib.png_set_compression_level(context->temp_png_ptr, compression_level);
    lib.png_set_filter(context->temp_png_ptr, 0, PNG_FILTER_TYPE_DEFAULT);
    lib.png_set_IHDR(context->temp_png_ptr, context->temp_info_ptr, surface->w, surface->h,
                     8, png_color_type, // Use specified color type
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (png_color_type == PNG_COLOR_TYPE_PALETTE) {
        if (surface->format != SDL_PIXELFORMAT_INDEX8 || !SDL_GetSurfacePalette(surface)) {
            SDL_SetError("compress_surface_to_png_data: Expected SDL_PIXELFORMAT_INDEX8 surface with palette for paletted PNG type.");
            goto error;
        }
        SDL_Palette *surface_palette = SDL_GetSurfacePalette(surface);
        if (surface_palette && surface_palette->ncolors > 0) {
            lib.png_set_PLTE(context->temp_png_ptr, context->temp_info_ptr, (png_colorp)surface_palette->colors, surface_palette->ncolors);
        } else {
            SDL_SetError("Surface has no palette for paletted PNG compression.");
            goto error;
        }
        png_byte tRNS_data[1];
        tRNS_data[0] = 0x00;
        lib.png_set_tRNS(context->temp_png_ptr, context->temp_info_ptr, tRNS_data, 1, NULL);
    } else if (png_color_type == PNG_COLOR_TYPE_RGBA) {
        if (surface->format != SDL_PIXELFORMAT_RGBA32) {
            SDL_SetError("compress_surface_to_png_data: Expected SDL_PIXELFORMAT_RGBA32 surface for RGBA PNG type.");
            goto error;
        }
    } else {
        SDL_SetError("Unsupported PNG color type for compression.");
        goto error;
    }

    lib.png_write_info(context->temp_png_ptr, context->temp_info_ptr);
    context->row_pointers = (png_bytep *)SDL_malloc(sizeof(png_bytep) * surface->h);
    if (!context->row_pointers) {
        SDL_SetError("Out of memory for temporary row pointers");
        goto error;
    }
    for (int y = 0; y < surface->h; y++) {
        context->row_pointers[y] = (png_bytep)((Uint8 *)surface->pixels + y * (size_t)surface->pitch);
    }

    lib.png_write_image(context->temp_png_ptr, context->row_pointers);
    lib.png_write_end(context->temp_png_ptr, context->temp_info_ptr);

    SDL_free(context->row_pointers);
    context->row_pointers = NULL;

    context->mem_buffer_size = SDL_TellIO(context->mem_stream);
    if (context->mem_buffer_size < 0) {
        SDL_SetError("Failed to get size of memory stream: %s", SDL_GetError());
        goto error;
    }

    // Sanity check
    if (context->mem_buffer_size < (Sint64)(8 + 12 + 13 + 12 + 12)) { // PNG_SIG + IHDR_CHUNK + IDAT_CHUNK + IEND_CHUNK
        SDL_SetError("Temporary PNG stream too small, likely corrupted during internal write.");
        goto error;
    }

    // Allocate buffer to hold the entire content of the temporary PNG
    context->mem_buffer_ptr = (png_bytep)SDL_malloc(context->mem_buffer_size);
    if (!context->mem_buffer_ptr) {
        SDL_SetError("Out of memory for reading memory stream content");
        goto error;
    }

    if (SDL_SeekIO(context->mem_stream, 0, SDL_IO_SEEK_SET) < 0) {
        SDL_SetError("Failed to seek memory stream to beginning: %s", SDL_GetError());
        goto error;
    }
    if (SDL_ReadIO(context->mem_stream, context->mem_buffer_ptr, (size_t)context->mem_buffer_size) != (size_t)context->mem_buffer_size) {
        SDL_SetError("Failed to read all data from memory stream: %s", SDL_GetError());
        goto error;
    }

    context->current_pos = 8;      // Skip PNG signature (8 bytes)

    // Parse the temporary PNG in memory to extract only the IDAT chunk data (which is a full zlib stream)
    while (context->current_pos < context->mem_buffer_size) {
        if ((png_size_t)context->current_pos + 8 > (png_size_t)context->mem_buffer_size) {
            break;
        }
        SDL_memcpy(context->chunk_header, context->mem_buffer_ptr + context->current_pos, 8);
        context->chunk_len = libpng_get_uint_32(context->chunk_header);
        SDL_memcpy(context->chunk_type, context->chunk_header + 4, 4);

        context->current_pos += 8;

        if (SDL_memcmp(context->chunk_type, "IDAT", 4) == 0) {
            context->full_zlib_data_buffer = (png_bytep)SDL_realloc(context->full_zlib_data_buffer, *compressed_size + context->chunk_len);
            if (!context->full_zlib_data_buffer) {
                SDL_SetError("Out of memory for IDAT data aggregation (full zlib stream)");
                goto error;
            }
            SDL_memcpy(context->full_zlib_data_buffer + *compressed_size, context->mem_buffer_ptr + context->current_pos, context->chunk_len);
            *compressed_size += context->chunk_len;
        } else if (SDL_memcmp(context->chunk_type, "IEND", 4) == 0) {
            context->iend_found = true;
            break;
        }

        context->current_pos += (Sint64)context->chunk_len + 4;
    }

    if (*compressed_size == 0) {
        SDL_SetError("Could not find IDAT chunk in temporary PNG for compression");
        goto error;
    }
    if (!context->iend_found) {
        SDL_SetError("IEND chunk not found in temporary PNG, likely incomplete write.");
        goto error;
    }

    SDL_free(context->mem_buffer_ptr); // Free the temporary buffer holding the full PNG
    context->mem_buffer_ptr = NULL;
    SDL_CloseIO(context->mem_stream);  // Close the memory stream
    lib.png_destroy_write_struct(&context->temp_png_ptr, &context->temp_info_ptr);

    return context->full_zlib_data_buffer;

error:
    if (context->temp_png_ptr) {
        lib.png_destroy_write_struct(&context->temp_png_ptr, &context->temp_info_ptr);
    }
    if (context->mem_stream) {
        SDL_CloseIO(context->mem_stream);
    }
    if (context->full_zlib_data_buffer) {
        SDL_free(context->full_zlib_data_buffer);
    }
    if (context->mem_buffer_ptr) {
        SDL_free(context->mem_buffer_ptr);
    }

    return NULL;
}

static bool writetEXtchunk(SDL_IOStream *dst, const char *keyword, const char *value)
{
    size_t total_len = SDL_strlen(keyword) + 1 + SDL_strlen(value) + 1;
    png_byte *buffer = (png_byte *)SDL_malloc(total_len);
    if (!buffer) {
        return SDL_SetError("Out of memory for tEXt chunk");
    }
    SDL_snprintf((char *)buffer, total_len, "%s%c%s", keyword, '\0', value);
    if (!write_png_chunk(dst, "tEXt", buffer, total_len)) {
        SDL_free(buffer);
        return SDL_SetError("Failed to write png chunk tEXt");
    }
    SDL_free(buffer);
    return true;
}

static bool SaveAPNGAnimationPushFrame(IMG_AnimationEncoder *encoder, SDL_Surface *frame, Uint64 duration)
{
    if (!encoder->ctx) {
        // bogus call, not initialized
        SDL_SetError("APNG animation context not initialized.");
        return false;
    }

    if (!encoder->ctx->png_write_ptr) {
        // bogus call, not initialized
        SDL_SetError("APNG animation write not started.");
        return false;
    }
    if (!frame) {
        SDL_SetError("Frame surface is NULL.");
        return false;
    }

    SDL_Surface *current_frame_for_processing = NULL;
    SDL_Surface *final_frame_for_compression = NULL;
    png_bytep full_zlib_data = NULL;
    png_size_t full_zlib_size = 0;

    if (encoder->ctx->current_frame_index == 0) {
        png_byte pngColorType;
        png_byte bit_depth;

        encoder->ctx->output_pixel_format = frame->format;
        if (encoder->ctx->output_pixel_format != SDL_PIXELFORMAT_RGBA32 && encoder->ctx->output_pixel_format != SDL_PIXELFORMAT_INDEX8) {
            encoder->ctx->output_pixel_format = SDL_PIXELFORMAT_RGBA32;
        }

        if (encoder->ctx->output_pixel_format == SDL_PIXELFORMAT_INDEX8) {
            pngColorType = PNG_COLOR_TYPE_PALETTE; // Use palette for paletted output
        } else {
            encoder->ctx->output_pixel_format = SDL_PIXELFORMAT_RGBA32;
            pngColorType = PNG_COLOR_TYPE_RGBA; // Use RGBA for other formats
        }

        const SDL_PixelFormatDetails *pfd = SDL_GetPixelFormatDetails(encoder->ctx->output_pixel_format);
        if (pfd) {
            bit_depth = pfd->bits_per_pixel / pfd->bytes_per_pixel;
        } else {
            bit_depth = 8;
        }

        encoder->ctx->apng_width = frame->w;
        encoder->ctx->apng_height = frame->h;

        png_byte ihdr_data[13];
        custom_png_save_uint_32(ihdr_data, (png_int_32)encoder->ctx->apng_width);
        custom_png_save_uint_32(ihdr_data + 4, (png_int_32)encoder->ctx->apng_height);

        ihdr_data[8] = bit_depth;
        ihdr_data[9] = pngColorType;
        ihdr_data[10] = PNG_COMPRESSION_TYPE_BASE;
        ihdr_data[11] = PNG_FILTER_TYPE_BASE;
        ihdr_data[12] = PNG_INTERLACE_NONE;
        if (!write_png_chunk(encoder->dst, "IHDR", ihdr_data, 13)) {
            goto error;
        }

        encoder->ctx->acTL_chunk_start_pos = SDL_TellIO(encoder->dst);

        // Write a placeholder acTL chunk (animation control)
        // num_frames and num_plays will be updated in SaveAPNGAnimationEnd.
        png_byte actl_data[8];
        custom_png_save_uint_32(actl_data, 0);     // Placeholder for num_frames
        custom_png_save_uint_32(actl_data + 4, 0); // num_plays (0 for infinite loop)

        if (!write_png_chunk(encoder->dst, "acTL", actl_data, 8)) {
            goto error;
        }

        current_frame_for_processing = frame;
    } else {
        if (frame->w != encoder->ctx->apng_width || frame->h != encoder->ctx->apng_height) {
            current_frame_for_processing = frame;
            // The current API is unspecified about deciding whether to fail or resize subsequent frames according to the first frame so,
            // we will fail here as default, if API changes in the future requires us to resize the subsequent frames, please uncomment the code below.

            SDL_SetError("Frame %i doesn't match the first frame's width (current=%i | expected=%i) and height (current=%i | expected=%i)", encoder->ctx->current_frame_index, frame->w, encoder->ctx->apng_width, frame->h, encoder->ctx->apng_height);
            goto error;

            //    current_frame_for_processing = SDL_CreateSurface(stream->ctx->apng_width, stream->ctx->apng_height, SDL_PIXELFORMAT_RGBA32);
            //    if (!current_frame_for_processing) {
            //        SDL_SetError("Failed to create resized surface for APNG frame: %s", SDL_GetError());
            //        goto error;
            //    }
            //    if (!SDL_BlitSurfaceScaled(frame, NULL, current_frame_for_processing, NULL, SDL_SCALEMODE_NEAREST)) {
            //        SDL_SetError("Failed to scale surface for APNG frame: %s", SDL_GetError());
            //        goto error;
            //    }
        } else {
            current_frame_for_processing = frame;
        }
    }

    // We do manually convert surface pixel format right now if it doesn't much INDEX8 and RGBA32,
    // since those two are the only ones supported by this implementation of libpng right now (usually libpng only uses palette, rgb/a or gray/with alpha).
    if (encoder->ctx->output_pixel_format == SDL_PIXELFORMAT_INDEX8) {
        if (current_frame_for_processing->format != SDL_PIXELFORMAT_INDEX8) {
            final_frame_for_compression = SDL_ConvertSurface(current_frame_for_processing, SDL_PIXELFORMAT_INDEX8);
            if (!final_frame_for_compression) {
                SDL_SetError("Failed to convert frame to INDEX8 for compression: %s", SDL_GetError());
                goto error;
            }
        } else {
            final_frame_for_compression = current_frame_for_processing;
        }
    } else { // Default to RGBA32
        if (current_frame_for_processing->format != SDL_PIXELFORMAT_RGBA32) {
            final_frame_for_compression = SDL_ConvertSurface(current_frame_for_processing, SDL_PIXELFORMAT_RGBA32);
            if (!final_frame_for_compression) {
                SDL_SetError("Failed to convert frame to RGBA32 for compression: %s", SDL_GetError());
                goto error;
            }
        } else {
            final_frame_for_compression = current_frame_for_processing;
        }
    }

    int png_color_type_for_compression;
    if (encoder->ctx->output_pixel_format == SDL_PIXELFORMAT_INDEX8) {
        png_color_type_for_compression = PNG_COLOR_TYPE_PALETTE;
    } else {
        png_color_type_for_compression = PNG_COLOR_TYPE_RGBA;
    }

    png_uint_16 delay_den = (png_uint_16)encoder->timebase_denominator;
    png_uint_16 delay_num = (png_uint_16)(duration * encoder->timebase_numerator);

    // Default image + first animated frame
    if (encoder->ctx->current_frame_index == 0) {
        // If paletted output, create and write PLTE and tRNS chunks
        if (encoder->ctx->output_pixel_format == SDL_PIXELFORMAT_INDEX8) {
            SDL_Palette *first_frame_palette = SDL_GetSurfacePalette(final_frame_for_compression);
            if (first_frame_palette && first_frame_palette->ncolors > 0) {
                encoder->ctx->apng_palette_ptr = SDL_CreatePalette(first_frame_palette->ncolors);
                if (!encoder->ctx->apng_palette_ptr) {
                    SDL_SetError("Failed to allocate palette for APNG: %s", SDL_GetError());
                    goto error;
                }
                SDL_SetPaletteColors(encoder->ctx->apng_palette_ptr, first_frame_palette->colors, 0, first_frame_palette->ncolors);
            } else {
                SDL_SetError("First frame has no palette after conversion to indexed format.");
                goto error;
            }

            // Write PLTE chunk
            png_bytep plte_data = (png_bytep)SDL_malloc((size_t)encoder->ctx->apng_palette_ptr->ncolors * 3); // 3 bytes per color (RGB)
            if (!plte_data) {
                SDL_SetError("Out of memory for PLTE data");
                goto error;
            }
            for (int i = 0; i < encoder->ctx->apng_palette_ptr->ncolors; ++i) {
                plte_data[i * 3 + 0] = encoder->ctx->apng_palette_ptr->colors[i].r;
                plte_data[i * 3 + 1] = encoder->ctx->apng_palette_ptr->colors[i].g;
                plte_data[i * 3 + 2] = encoder->ctx->apng_palette_ptr->colors[i].b;
            }
            if (!write_png_chunk(encoder->dst, "PLTE", plte_data, (size_t)encoder->ctx->apng_palette_ptr->ncolors * 3)) {
                goto error;
            }
            SDL_free(plte_data);

            // Write tRNS chunk (based on hex dump, first entry transparent)
            png_byte tRNS_data[1];
            tRNS_data[0] = 0x00;
            if (!write_png_chunk(encoder->dst, "tRNS", tRNS_data, 1)) {
                goto error;
            }
        }

        if (IMG_HasMetadata(encoder->ctx->metadata)) {

            const char *desc = SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_DESCRIPTION_STRING, NULL);
            const char *rights = SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_COPYRIGHT_STRING, NULL);
            const char *title = SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_TITLE_STRING, NULL);
            const char *author = SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_AUTHOR_STRING, NULL);
            const char *creationtime = SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_CREATION_TIME_STRING, NULL);

            if (desc) {
                if (!writetEXtchunk(encoder->dst, "Description", desc)) {
                    goto error;
                }
            }
            if (rights) {
                if (!writetEXtchunk(encoder->dst, "Copyright", rights)) {
                    goto error;
                }
            }
            if (title) {
                if (!writetEXtchunk(encoder->dst, "Title", title)) {
                    goto error;
                }
            }
            if (author) {
                if (!writetEXtchunk(encoder->dst, "Author", author)) {
                    goto error;
                }
            }
            if (creationtime) {
                if (!writetEXtchunk(encoder->dst, "Creation Time", creationtime)) {
                    goto error;
                }
            }
        }

        // Now, write the fcTL for the first animated frame (sequence 0)
        png_byte fctl_data[26];
        custom_png_save_uint_32(fctl_data, 0);                                          // Sequence number for the first animated frame is fixed at 0.
        custom_png_save_uint_32(fctl_data + 4, (png_uint_32)encoder->ctx->apng_width);  // Frame width
        custom_png_save_uint_32(fctl_data + 8, (png_uint_32)encoder->ctx->apng_height); // Frame height
        custom_png_save_uint_32(fctl_data + 12, 0);                                     // x_offset
        custom_png_save_uint_32(fctl_data + 16, 0);                                     // y_offset

        custom_png_save_uint_16(fctl_data + 20, delay_num);
        custom_png_save_uint_16(fctl_data + 22, delay_den);
        fctl_data[24] = PNG_DISPOSE_OP_NONE; // dispose_op
        fctl_data[25] = PNG_BLEND_OP_SOURCE; // blend_op
        if (!write_png_chunk(encoder->dst, "fcTL", fctl_data, 26)) {
            goto error;
        }

        // Compress the current frame's surface into a full zlib stream (IDAT payload)
        CompressionContext compressionContext;
        SDL_zero(compressionContext);
        full_zlib_data = compress_surface_to_png_data(&compressionContext, final_frame_for_compression, &full_zlib_size, encoder->ctx->compression_level, png_color_type_for_compression);
        if (!full_zlib_data || full_zlib_size == 0) {
            SDL_SetError("Failed to compress frame data for default IDAT.");
            goto error;
        }

        // Write IDAT chunk: This is the default image, NOT part of the animation sequence
        if (!write_png_chunk(encoder->dst, "IDAT", full_zlib_data, full_zlib_size)) {
            goto error;
        }
        SDL_free(full_zlib_data);
        full_zlib_data = NULL;

        // We have no fdAT chunk for the first frame, so we increase our index only by 1.
        encoder->ctx->current_frame_index = 1;

    } else { // For subsequent animated frames (current_frame_index > 0)
        // Compress the current frame's surface into a full zlib stream (IDAT payload)
        CompressionContext compressionContext;
        SDL_zero(compressionContext);
        full_zlib_data = compress_surface_to_png_data(&compressionContext, final_frame_for_compression, &full_zlib_size, encoder->ctx->compression_level, png_color_type_for_compression);
        if (!full_zlib_data || full_zlib_size == 0) {
            SDL_SetError("Failed to compress frame data or compressed data is empty.");
            goto error;
        }

        // Write fcTL chunk for this frame
        png_byte fctl_data[26];
        custom_png_save_uint_32(fctl_data, (png_uint_32)encoder->ctx->current_frame_index); // Sequence number for this fcTL
        custom_png_save_uint_32(fctl_data + 4, (png_uint_32)encoder->ctx->apng_width);      // Frame width
        custom_png_save_uint_32(fctl_data + 8, (png_uint_32)encoder->ctx->apng_height);     // Frame height
        custom_png_save_uint_32(fctl_data + 12, 0);                                         // x_offset
        custom_png_save_uint_32(fctl_data + 16, 0);                                         // y_offset

        custom_png_save_uint_16(fctl_data + 20, delay_num);
        custom_png_save_uint_16(fctl_data + 22, delay_den);
        fctl_data[24] = PNG_DISPOSE_OP_NONE;
        fctl_data[25] = PNG_BLEND_OP_SOURCE;

        if (!write_png_chunk(encoder->dst, "fcTL", fctl_data, 26)) {
            goto error;
        }

        // Write fdAT chunk for this frame
        png_byte fdat_prefix[4];
        custom_png_save_uint_32(fdat_prefix, (png_uint_32)(encoder->ctx->current_frame_index + 1)); // Sequence number for fdAT

        png_bytep fdat_data = NULL;
        if (full_zlib_size > SDL_SIZE_MAX - 4) {
            SDL_SetError("fdAT data size would overflow");
            goto error;
        }
        fdat_data = (png_bytep)SDL_malloc(4 + full_zlib_size);
        if (!fdat_data) {
            SDL_SetError("Out of memory for fdAT data");
            goto error;
        }
        SDL_memcpy(fdat_data, fdat_prefix, 4);
        SDL_memcpy(fdat_data + 4, full_zlib_data, full_zlib_size);
        if (!write_png_chunk(encoder->dst, "fdAT", fdat_data, 4 + full_zlib_size)) {
            goto error;
        }
        SDL_free(fdat_data);
        fdat_data = NULL;
        SDL_free(full_zlib_data);
        full_zlib_data = NULL;

        encoder->ctx->current_frame_index += 2; // Increment by 2 (one for fcTL and one for fdAT) for the next fcTL sequence number
    }

    if (current_frame_for_processing && current_frame_for_processing != frame) {
        SDL_DestroySurface(current_frame_for_processing);
    }
    if (final_frame_for_compression && final_frame_for_compression != current_frame_for_processing && final_frame_for_compression != frame) {
        SDL_DestroySurface(final_frame_for_compression);
    }
    return true;

error:
    if (full_zlib_data) {
        SDL_free(full_zlib_data);
    }
    if (current_frame_for_processing && current_frame_for_processing != frame) {
        SDL_DestroySurface(current_frame_for_processing);
    }
    if (final_frame_for_compression && final_frame_for_compression != current_frame_for_processing && final_frame_for_compression != frame) {
        SDL_DestroySurface(final_frame_for_compression);
    }
    return false;
}

static bool SaveAPNGAnimationEnd(IMG_AnimationEncoder *encoder)
{
    if (!encoder->ctx) {
        // bogus call, not initialized
        SDL_SetError("APNG animation context not initialized.");
        return false;
    }

    if (!encoder->ctx->png_write_ptr) {
        // bogus call, not initialized
        SDL_SetError("APNG animation write not in progress.");
        return false;
    }

    Sint64 current_pos = SDL_TellIO(encoder->dst);
    if (current_pos < 0) {
        SDL_SetError("Failed to get current stream position: %s", SDL_GetError());
        goto error;
    }

    if (SDL_SeekIO(encoder->dst, encoder->ctx->acTL_chunk_start_pos, SDL_IO_SEEK_SET) < 0) {
        SDL_SetError("Failed to seek to acTL chunk: %s", SDL_GetError());
        goto error;
    }

    png_byte actl_data[8];
    // Write the actual total number of frames pushed (which is current_frame_index + 1 / 2 or directly 0 if current_frame_index is 0)
    custom_png_save_uint_32(actl_data, (png_uint_32)(encoder->ctx->current_frame_index == 0 ? 0 : (encoder->ctx->current_frame_index + 1) / 2));

    // num_plays
    png_uint_32 numplays = 0;
    if (IMG_HasMetadata(encoder->ctx->metadata)) {
        numplays = (png_uint_32)SDL_max(SDL_GetNumberProperty(encoder->ctx->metadata, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, 0), 0);
    }

    custom_png_save_uint_32(actl_data + 4, numplays);

    // Re-write the updated acTL chunk. write_png_chunk will recalculate its CRC.
    if (!write_png_chunk(encoder->dst, "acTL", actl_data, 8)) {
        goto error;
    }

    if (SDL_SeekIO(encoder->dst, current_pos, SDL_IO_SEEK_SET) < 0) {
        SDL_SetError("Failed to seek back to end of stream: %s", SDL_GetError());
        goto error;
    }

    // Write the IEND chunk to finalize the PNG file
    if (!write_png_chunk(encoder->dst, "IEND", NULL, 0)) {
        goto error;
    }

    lib.png_destroy_write_struct(&encoder->ctx->png_write_ptr, &encoder->ctx->info_write_ptr);
    if (encoder->ctx->apng_palette_ptr) {
        SDL_DestroyPalette(encoder->ctx->apng_palette_ptr);
    }

    if (encoder->ctx->metadata) {
        SDL_DestroyProperties(encoder->ctx->metadata);
        encoder->ctx->metadata = 0;
    }

    SDL_free(encoder->ctx);
    encoder->ctx = NULL;
    return true;

error:
    if (encoder->ctx->metadata) {
        SDL_DestroyProperties(encoder->ctx->metadata);
        encoder->ctx->metadata = 0;
    }
    if (encoder->ctx->png_write_ptr) {
        lib.png_destroy_write_struct(&encoder->ctx->png_write_ptr, &encoder->ctx->info_write_ptr);
    }
    if (encoder->ctx->apng_palette_ptr) {
        SDL_DestroyPalette(encoder->ctx->apng_palette_ptr);
    }
    SDL_free(encoder->ctx);
    encoder->ctx = NULL;
    return false;
}

bool IMG_CreateAPNGAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    if (!IMG_InitPNG()) {
        return false;
    }

    IMG_AnimationEncoderContext *ctx = (IMG_AnimationEncoderContext *)SDL_calloc(1, sizeof(*encoder->ctx));
    if (!ctx) {
        return false;
    }

    encoder->ctx = ctx;

    encoder->AddFrame = SaveAPNGAnimationPushFrame;
    encoder->Close = SaveAPNGAnimationEnd;

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    if (!ignoreProps) {
        ctx->metadata = SDL_CreateProperties();
        if (!ctx->metadata) {
            SDL_SetError("Failed to create properties for APNG metadata");
            SDL_free(ctx);
            return false;
        }

        if (!SDL_CopyProperties(props, ctx->metadata)) {
            SDL_DestroyProperties(ctx->metadata);
            ctx->metadata = 0;
            SDL_free(ctx);
            return false;
        }
    }

    encoder->start = SDL_TellIO(encoder->dst);

    // Calculate compression level based on quality (1-9)
    ctx->compression_level = (int)(encoder->quality / 100.0f * 8.0f) + 1;
    if (ctx->compression_level < 1)
        ctx->compression_level = 1;
    if (ctx->compression_level > 9)
        ctx->compression_level = 9;

    ctx->png_write_ptr = lib.png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!ctx->png_write_ptr) {
        SDL_SetError("Couldn't allocate memory for PNG write struct");
        goto error;
    }
    ctx->info_write_ptr = lib.png_create_info_struct(ctx->png_write_ptr);
    if (!ctx->info_write_ptr) {
        SDL_SetError("Couldn't create image information for PNG file");
        goto error;
    }

#ifndef LIBPNG_VERSION_12
    if (setjmp(*lib.png_set_longjmp_fn(ctx->png_write_ptr, longjmp, sizeof(jmp_buf))))
#else
    if (setjmp(ctx->png_write_ptr->jmpbuf))
#endif
    {
        SDL_SetError("Error during APNG write setup");
        goto error;
    }

    // png_io_context io_context = { apng_write_ctx.dst_stream };
    lib.png_set_write_fn(ctx->png_write_ptr, encoder->dst, png_write_data, png_flush_data);

    // Write PNG signature (8 bytes)
    if (SDL_WriteIO(encoder->dst, png_sig, 8) != 8) {
        SDL_SetError("Failed to write PNG signature");
        goto error;
    }

    return true;

error:
    if (ctx->png_write_ptr) {
        lib.png_destroy_write_struct(&ctx->png_write_ptr, &ctx->info_write_ptr);
    }
    if (ctx->apng_palette_ptr) {
        SDL_DestroyPalette(ctx->apng_palette_ptr);
    }
    SDL_free(ctx);
    return false;
}

#else /* SAVE_PNG */

bool IMG_CreateAPNGAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    return SDL_SetError("SDL_image built without PNG save support");
}

#endif /* !SAVE_PNG */

#else /* SDL_IMAGE_LIBPNG */

bool IMG_CreateAPNGAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    return SDL_SetError("SDL_image not built against libpng.");
}

bool IMG_CreateAPNGAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    return SDL_SetError("SDL_image not built against libpng.");
}

#endif /* SDL_IMAGE_LIBPNG */
