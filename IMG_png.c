/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

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

#if !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND)

/* This is a PNG image file loading framework */

#include <stdlib.h>
#include <stdio.h>

#include "SDL_image.h"

#ifdef LOAD_PNG

/* This code was originally written by Philippe Lavoie (2 November 1998) */

#include "SDL_endian.h"

#ifdef macintosh
#define MACOS
#endif
#include <png.h>

/* Check for the older version of libpng */
#if (PNG_LIBPNG_VER_MAJOR == 1)
#if (PNG_LIBPNG_VER_MINOR < 5)
#define LIBPNG_VERSION_12
typedef png_bytep png_const_bytep;
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
#ifndef LIBPNG_VERSION_12
	jmp_buf* (*png_set_longjmp_fn) (png_structrp, png_longjmp_ptr, size_t);
#endif
} lib;

#ifdef LOAD_PNG_DYNAMIC
int IMG_InitPNG()
{
	if ( lib.loaded == 0 ) {
		lib.handle = SDL_LoadObject(LOAD_PNG_DYNAMIC);
		if ( lib.handle == NULL ) {
			return -1;
		}
		lib.png_create_info_struct =
			(png_infop (*) (png_noconst15_structrp))
			SDL_LoadFunction(lib.handle, "png_create_info_struct");
		if ( lib.png_create_info_struct == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_create_read_struct =
			(png_structrp (*) (png_const_charp, png_voidp, png_error_ptr, png_error_ptr))
			SDL_LoadFunction(lib.handle, "png_create_read_struct");
		if ( lib.png_create_read_struct == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_destroy_read_struct =
			(void (*) (png_structpp, png_infopp, png_infopp))
			SDL_LoadFunction(lib.handle, "png_destroy_read_struct");
		if ( lib.png_destroy_read_struct == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_get_IHDR =
			(png_uint_32 (*) (png_noconst15_structrp, png_noconst15_inforp, png_uint_32 *, png_uint_32 *, int *, int *, int *, int *, int *))
			SDL_LoadFunction(lib.handle, "png_get_IHDR");
		if ( lib.png_get_IHDR == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_get_channels =
			(png_byte (*) (png_const_structrp, png_const_inforp))
			SDL_LoadFunction(lib.handle, "png_get_channels");
		if ( lib.png_get_channels == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_get_io_ptr =
			(png_voidp (*) (png_noconst15_structrp))
			SDL_LoadFunction(lib.handle, "png_get_io_ptr");
		if ( lib.png_get_io_ptr == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_get_PLTE =
			(png_uint_32 (*) (png_const_structrp, png_noconst16_inforp, png_colorp *, int *))
			SDL_LoadFunction(lib.handle, "png_get_PLTE");
		if ( lib.png_get_PLTE == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_get_tRNS =
			(png_uint_32 (*) (png_const_structrp, png_inforp, png_bytep *, int *, png_color_16p *))
			SDL_LoadFunction(lib.handle, "png_get_tRNS");
		if ( lib.png_get_tRNS == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_get_valid =
			(png_uint_32 (*) (png_const_structrp, png_const_inforp, png_uint_32))
			SDL_LoadFunction(lib.handle, "png_get_valid");
		if ( lib.png_get_valid == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_read_image =
			(void (*) (png_structrp, png_bytepp))
			SDL_LoadFunction(lib.handle, "png_read_image");
		if ( lib.png_read_image == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_read_info =
			(void (*) (png_structrp, png_inforp))
			SDL_LoadFunction(lib.handle, "png_read_info");
		if ( lib.png_read_info == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_read_update_info =
			(void (*) (png_structrp, png_inforp))
			SDL_LoadFunction(lib.handle, "png_read_update_info");
		if ( lib.png_read_update_info == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_set_expand =
			(void (*) (png_structrp))
			SDL_LoadFunction(lib.handle, "png_set_expand");
		if ( lib.png_set_expand == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_set_gray_to_rgb =
			(void (*) (png_structrp))
			SDL_LoadFunction(lib.handle, "png_set_gray_to_rgb");
		if ( lib.png_set_gray_to_rgb == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_set_packing =
			(void (*) (png_structrp))
			SDL_LoadFunction(lib.handle, "png_set_packing");
		if ( lib.png_set_packing == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_set_read_fn =
			(void (*) (png_structrp, png_voidp, png_rw_ptr))
			SDL_LoadFunction(lib.handle, "png_set_read_fn");
		if ( lib.png_set_read_fn == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_set_strip_16 =
			(void (*) (png_structrp))
			SDL_LoadFunction(lib.handle, "png_set_strip_16");
		if ( lib.png_set_strip_16 == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_set_interlace_handling =
			(int (*) (png_structrp))
			SDL_LoadFunction(lib.handle, "png_set_interlace_handling");
		if ( lib.png_set_interlace_handling == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
		lib.png_sig_cmp =
			(int (*) (png_const_bytep, png_size_t, png_size_t))
			SDL_LoadFunction(lib.handle, "png_sig_cmp");
		if ( lib.png_sig_cmp == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
#ifndef LIBPNG_VERSION_12
		lib.png_set_longjmp_fn =
			(jmp_buf * (*) (png_structrp, png_longjmp_ptr, size_t))
			SDL_LoadFunction(lib.handle, "png_set_longjmp_fn");
		if ( lib.png_set_longjmp_fn == NULL ) {
			SDL_UnloadObject(lib.handle);
			return -1;
		}
#endif
	}
	++lib.loaded;

	return 0;
}
void IMG_QuitPNG()
{
	if ( lib.loaded == 0 ) {
		return;
	}
	if ( lib.loaded == 1 ) {
		SDL_UnloadObject(lib.handle);
	}
	--lib.loaded;
}
#else
int IMG_InitPNG()
{
	if ( lib.loaded == 0 ) {
		lib.png_create_info_struct = (png_info * (*)(png_struct *))png_create_info_struct;
		lib.png_create_read_struct = png_create_read_struct;
		lib.png_destroy_read_struct = png_destroy_read_struct;
		lib.png_get_IHDR = (png_uint_32 (*)(png_struct *, png_info *, png_uint_32 *, png_uint_32 *, int *, int *, int *, int *, int *))png_get_IHDR;
		lib.png_get_channels = (png_byte (*)(png_struct *, png_info *))png_get_channels;
		lib.png_get_io_ptr = (void * (*)(png_struct *))png_get_io_ptr;
		lib.png_get_PLTE = (png_uint_32 (*)(png_struct *, png_info *, png_color **, int *))png_get_PLTE;
		lib.png_get_tRNS = (png_uint_32 (*)(png_struct *, png_info *, png_byte **, int *, png_color_16 **))png_get_tRNS;
		lib.png_get_valid = (png_uint_32 (*)(png_struct *, png_info *, png_uint_32))png_get_valid;
		lib.png_read_image = png_read_image;
		lib.png_read_info = png_read_info;
		lib.png_read_update_info = png_read_update_info;
		lib.png_set_expand = png_set_expand;
		lib.png_set_gray_to_rgb = png_set_gray_to_rgb;
		lib.png_set_packing = png_set_packing;
		lib.png_set_read_fn = png_set_read_fn;
		lib.png_set_strip_16 = png_set_strip_16;
		lib.png_set_interlace_handling = png_set_interlace_handling;
		lib.png_sig_cmp = (int (*)(png_byte *, png_size_t,  png_size_t))png_sig_cmp;
#ifndef LIBPNG_VERSION_12
		lib.png_set_longjmp_fn = png_set_longjmp_fn;
#endif
	}
	++lib.loaded;

	return 0;
}
void IMG_QuitPNG()
{
	if ( lib.loaded == 0 ) {
		return;
	}
	if ( lib.loaded == 1 ) {
	}
	--lib.loaded;
}
#endif /* LOAD_PNG_DYNAMIC */

/* See if an image is contained in a data source */
int IMG_isPNG(SDL_RWops *src)
{
	int start;
	int is_PNG;
	Uint8 magic[4];

	if ( !src ) {
		return 0;
	}
	start = SDL_RWtell(src);
	is_PNG = 0;
	if ( SDL_RWread(src, magic, 1, sizeof(magic)) == sizeof(magic) ) {
		if ( magic[0] == 0x89 &&
		     magic[1] == 'P' &&
		     magic[2] == 'N' &&
		     magic[3] == 'G' ) {
			is_PNG = 1;
		}
	}
	SDL_RWseek(src, start, RW_SEEK_SET);
	return(is_PNG);
}

/* Load a PNG type image from an SDL datasource */
static void png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
	SDL_RWops *src;

	src = (SDL_RWops *)lib.png_get_io_ptr(ctx);
	SDL_RWread(src, area, size, 1);
}

struct loadpng_vars {
	const char *error;
	SDL_Surface *surface;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers;
};

static void LIBPNG_LoadPNG_RW(SDL_RWops *src, struct loadpng_vars *vars)
{
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, num_channels;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	SDL_Palette *palette;
	int row, i;
	int ckey = -1;
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
#ifndef LIBPNG_VERSION_12
	if (setjmp(*lib.png_set_longjmp_fn(vars->png_ptr, longjmp, sizeof(jmp_buf))))
#else
	if (setjmp(vars->png_ptr->jmpbuf))
#endif
	{
		vars->error = "Error reading the PNG file.";
		return;
	}

	/* Set up the input control */
	lib.png_set_read_fn(vars->png_ptr, src, png_read_data);

	/* Read PNG header info */
	lib.png_read_info(vars->png_ptr, vars->info_ptr);
	lib.png_get_IHDR(vars->png_ptr, vars->info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL);

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	lib.png_set_strip_16(vars->png_ptr) ;

	/* tell libpng to de-interlace (if the image is interlaced) */
	lib.png_set_interlace_handling(vars->png_ptr) ;

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	lib.png_set_packing(vars->png_ptr);

	/* scale greyscale values to the range 0..255 */
	if(color_type == PNG_COLOR_TYPE_GRAY)
		lib.png_set_expand(vars->png_ptr);

	/* For images with a single "transparent colour", set colour key;
	   if more than one index has transparency, or if partially transparent
	   entries exist, use full alpha channel */
	if (lib.png_get_valid(vars->png_ptr, vars->info_ptr, PNG_INFO_tRNS)) {
	        int num_trans;
		Uint8 *trans;
		lib.png_get_tRNS(vars->png_ptr, vars->info_ptr, &trans, &num_trans,
			     &transv);
		if(color_type == PNG_COLOR_TYPE_PALETTE) {
		    /* Check if all tRNS entries are opaque except one */
		    int j, t = -1;
		    for(j = 0; j < num_trans; j++) {
			if(trans[j] == 0) {
			    if (t >= 0) {
				break;
			    }
			    t = j;
			} else if(trans[j] != 255) {
			    break;
			}
		    }
		    if(j == num_trans) {
			/* exactly one transparent index */
			ckey = t;
		    } else {
			/* more than one transparent index, or translucency */
			lib.png_set_expand(vars->png_ptr);
		    }
		} else
		    ckey = 0; /* actual value will be set later */
	}

	if ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
		lib.png_set_gray_to_rgb(vars->png_ptr);

	lib.png_read_update_info(vars->png_ptr, vars->info_ptr);

	lib.png_get_IHDR(vars->png_ptr, vars->info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL);

	/* Allocate the SDL surface to hold the image */
	Rmask = Gmask = Bmask = Amask = 0 ;
	num_channels = lib.png_get_channels(vars->png_ptr, vars->info_ptr);
	if ( color_type != PNG_COLOR_TYPE_PALETTE ) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		Rmask = 0x000000FF;
		Gmask = 0x0000FF00;
		Bmask = 0x00FF0000;
		Amask = (num_channels == 4) ? 0xFF000000 : 0;
#else
		int s = (num_channels == 4) ? 0 : 8;
		Rmask = 0xFF000000 >> s;
		Gmask = 0x00FF0000 >> s;
		Bmask = 0x0000FF00 >> s;
		Amask = 0x000000FF >> s;
#endif
	}
	vars->surface = SDL_AllocSurface(SDL_SWSURFACE, width, height,
				bit_depth*num_channels, Rmask,Gmask,Bmask,Amask);
	if (vars->surface == NULL) {
		vars->error = "Out of memory";
		return;
	}

	if (ckey != -1) {
		if(color_type != PNG_COLOR_TYPE_PALETTE)
			/* FIXME: Should these be truncated or shifted down? */
			ckey = SDL_MapRGB(vars->surface->format,
					  (Uint8)transv->red,
					  (Uint8)transv->green,
					  (Uint8)transv->blue);
		SDL_SetColorKey(vars->surface, SDL_SRCCOLORKEY, ckey);
	}

	/* Create the array of pointers to image data */
	vars->row_pointers = (png_bytep*) malloc(sizeof(png_bytep)*height);
	if (vars->row_pointers == NULL) {
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

	lib.png_read_end(vars->png_ptr, vars->info_ptr);
	*/

	/* Load the palette, if any */
	palette = vars->surface->format->palette;
	if ( palette ) {
	    int png_num_palette;
	    png_colorp png_palette;
	    lib.png_get_PLTE(vars->png_ptr, vars->info_ptr, &png_palette, &png_num_palette);
	    if(color_type == PNG_COLOR_TYPE_GRAY) {
		palette->ncolors = 256;
		for(i = 0; i < 256; i++) {
		    palette->colors[i].r = (Uint8)i;
		    palette->colors[i].g = (Uint8)i;
		    palette->colors[i].b = (Uint8)i;
		}
	    } else if (png_num_palette > 0 ) {
		palette->ncolors = png_num_palette;
		for( i=0; i<png_num_palette; ++i ) {
		    palette->colors[i].b = png_palette[i].blue;
		    palette->colors[i].g = png_palette[i].green;
		    palette->colors[i].r = png_palette[i].red;
		}
	    }
	}
}

SDL_Surface *IMG_LoadPNG_RW(SDL_RWops *src)
{
	int start;
	struct loadpng_vars vars;

	if ( !src ) {
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}

	if ( (IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0 ) {
		return NULL;
	}

	start = SDL_RWtell(src);
	memset(&vars, 0, sizeof(vars));

	LIBPNG_LoadPNG_RW(src, &vars);

	if (vars.png_ptr) {
		lib.png_destroy_read_struct(&vars.png_ptr,
					vars.info_ptr ? &vars.info_ptr : (png_infopp)0,
					(png_infopp)0);
	}
	if (vars.row_pointers) {
		free(vars.row_pointers);
	}
	if (vars.error) {
		SDL_RWseek(src, start, RW_SEEK_SET);
		if (vars.surface) {
			SDL_FreeSurface(vars.surface);
			vars.surface = NULL;
		}
		IMG_SetError(vars.error);
	}
	return vars.surface;
}

#else

int IMG_InitPNG()
{
	IMG_SetError("PNG images are not supported");
	return(-1);
}

void IMG_QuitPNG()
{
}

/* See if an image is contained in a data source */
int IMG_isPNG(SDL_RWops *src)
{
	return(0);
}

/* Load a PNG type image from an SDL datasource */
SDL_Surface *IMG_LoadPNG_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_PNG */

#endif /* !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND) */
