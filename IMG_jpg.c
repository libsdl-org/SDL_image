/*
    SDL_image:  An example image loading library for use with SDL
    Copyright (C) 1999-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* $Id$ */

/* This is a JPEG image file loading framework */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include "SDL_image.h"

#ifdef LOAD_JPG

#include <jpeglib.h>

/* Define this for fast loading and not as good image quality */
/*#define FAST_JPEG*/

/* See if an image is contained in a data source */
int IMG_isJPG(SDL_RWops *src)
{
	int is_JPG;
	Uint8 magic[4];

	is_JPG = 0;
	if ( SDL_RWread(src, magic, 2, 1) ) {
		if ( (magic[0] == 0xFF) && (magic[1] == 0xD8) ) {
			SDL_RWread(src, magic, 4, 1);
			SDL_RWread(src, magic, 4, 1);
			if ( memcmp((char *)magic, "JFIF", 4) == 0 ||
			     memcmp((char *)magic, "Exif", 4) == 0 ) {
				is_JPG = 1;
			}
		}
	}
	return(is_JPG);
}

#define INPUT_BUFFER_SIZE	4096
typedef struct {
	struct jpeg_source_mgr pub;

	SDL_RWops *ctx;
	Uint8 buffer[INPUT_BUFFER_SIZE];
} my_source_mgr;

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */
static void init_source (j_decompress_ptr cinfo)
{
	/* We don't actually need to do anything */
	return;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 */
static int fill_input_buffer (j_decompress_ptr cinfo)
{
	my_source_mgr * src = (my_source_mgr *) cinfo->src;
	int nbytes;

	nbytes = SDL_RWread(src->ctx, src->buffer, 1, INPUT_BUFFER_SIZE);
	if (nbytes <= 0) {
		/* Insert a fake EOI marker */
		src->buffer[0] = (Uint8) 0xFF;
		src->buffer[1] = (Uint8) JPEG_EOI;
		nbytes = 2;
	}
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;

	return TRUE;
}


/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */
static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_source_mgr * src = (my_source_mgr *) cinfo->src;

	/* Just a dumb implementation for now.	Could use fseek() except
	 * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) src->pub.fill_input_buffer(cinfo);
			/* note we assume that fill_input_buffer will never
			 * return FALSE, so suspension need not be handled.
			 */
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.
 */
static void term_source (j_decompress_ptr cinfo)
{
	/* We don't actually need to do anything */
	return;
}

/*
 * Prepare for input from a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */
static void jpeg_SDL_RW_src (j_decompress_ptr cinfo, SDL_RWops *ctx)
{
  my_source_mgr *src;

  /* The source object and input buffer are made permanent so that a series
   * of JPEG images can be read from the same file by calling jpeg_stdio_src
   * only before the first one.  (If we discarded the buffer at the end of
   * one image, we'd likely lose the start of the next one.)
   * This makes it unsafe to use this manager and a different source
   * manager serially with the same JPEG object.  Caveat programmer.
   */
  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(my_source_mgr));
    src = (my_source_mgr *) cinfo->src;
  }

  src = (my_source_mgr *) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = term_source;
  src->ctx = ctx;
  src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->pub.next_input_byte = NULL; /* until buffer loaded */
}

struct my_error_mgr {
	struct jpeg_error_mgr errmgr;
	jmp_buf escape;
};

static void my_error_exit(j_common_ptr cinfo)
{
	struct my_error_mgr *err = (struct my_error_mgr *)cinfo->err;
	longjmp(err->escape, 1);
}

static void output_no_message(j_common_ptr cinfo)
{
	/* do nothing */
}

/* Load a JPEG type image from an SDL datasource */
SDL_Surface *IMG_LoadJPG_RW(SDL_RWops *src)
{
	struct jpeg_decompress_struct cinfo;
	JSAMPROW rowptr[1];
	SDL_Surface *volatile surface = NULL;
	struct my_error_mgr jerr;

	if ( !src ) {
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}

	/* Create a decompression structure and load the JPEG header */
	cinfo.err = jpeg_std_error(&jerr.errmgr);
	jerr.errmgr.error_exit = my_error_exit;
	jerr.errmgr.output_message = output_no_message;
	if(setjmp(jerr.escape)) {
		/* If we get here, libjpeg found an error */
		jpeg_destroy_decompress(&cinfo);
		IMG_SetError("JPEG loading error");
		SDL_FreeSurface(surface);
		return NULL;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_SDL_RW_src(&cinfo, src);
	jpeg_read_header(&cinfo, TRUE);

	/* Set 24-bit RGB output */
	cinfo.out_color_space = JCS_RGB;
	cinfo.quantize_colors = FALSE;
#ifdef FAST_JPEG
	cinfo.scale_num   = 1;
	cinfo.scale_denom = 1;
	cinfo.dct_method = JDCT_FASTEST;
	cinfo.do_fancy_upsampling = FALSE;
#endif
	jpeg_calc_output_dimensions(&cinfo);

	/* Allocate an output surface to hold the image */
	surface = SDL_AllocSurface(SDL_SWSURFACE,
	               cinfo.output_width, cinfo.output_height, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	                           0x0000FF, 0x00FF00, 0xFF0000,
#else
	                           0xFF0000, 0x00FF00, 0x0000FF,
#endif
				   0);
	if ( surface == NULL ) {
		IMG_SetError("Out of memory");
		goto done;
	}

	/* Decompress the image */
	jpeg_start_decompress(&cinfo);
	while ( cinfo.output_scanline < cinfo.output_height ) {
		rowptr[0] = (JSAMPROW)(Uint8 *)surface->pixels +
		                    cinfo.output_scanline * surface->pitch;
		jpeg_read_scanlines(&cinfo, rowptr, (JDIMENSION) 1);
	}
	jpeg_finish_decompress(&cinfo);

	/* Clean up and return */
done:
	jpeg_destroy_decompress(&cinfo);
	return(surface);
}

#else

/* See if an image is contained in a data source */
int IMG_isJPG(SDL_RWops *src)
{
	return(0);
}

/* Load a JPEG type image from an SDL datasource */
SDL_Surface *IMG_LoadJPG_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_JPG */
