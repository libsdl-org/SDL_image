/*
    IMGLIB:  An example image loading library for use with SDL
    Copyright (C) 1999  Sam Lantinga

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
    5635-34 Springhouse Dr.
    Pleasanton, CA 94588 (USA)
    slouken@devolution.com
*/

/*
 * PPM (portable pixmap) image loader:
 *
 * Supports: ASCII (P3) and binary (P6) formats
 * Does not support: maximum component value > 255
 *
 * TODO: add PGM (greyscale) and PBM (monochrome bitmap) support.
 *       Should be easy since almost all code is already in place
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "SDL_image.h"

#ifdef LOAD_PPM

/* See if an image is contained in a data source */
int IMG_isPPM(SDL_RWops *src)
{
	char magic[2];

	/* "P3" is the ASCII PPM format, "P6" the binary PPM format */
	return (SDL_RWread(src, magic, 2, 1)
		&& magic[0] == 'P' && (magic[1] == '3' || magic[1] == '6'));
}

/* read a non-negative integer from the source. return -1 upon error */
static int ReadNumber(SDL_RWops *src)
{
	int number;
	unsigned char ch;

	/* Initialize return value */
	number = 0;

	/* Skip leading whitespace */
	do {
		if ( ! SDL_RWread(src, &ch, 1, 1) ) {
			return(0);
		}
		/* Eat comments as whitespace */
		if ( ch == '#' ) {  /* Comment is '#' to end of line */
			do {
				if ( ! SDL_RWread(src, &ch, 1, 1) ) {
					return -1;
				}
			} while ( (ch != '\r') && (ch != '\n') );
		}
	} while ( isspace(ch) );

	/* Add up the number */
	do {
		number *= 10;
		number += ch-'0';

		if ( !SDL_RWread(src, &ch, 1, 1) ) {
			return -1;
		}
	} while ( isdigit(ch) );

	return(number);
}

SDL_Surface *IMG_LoadPPM_RW(SDL_RWops *src)
{
	SDL_Surface *surface = NULL;
	int width, height;
	int maxval, y, bpl;
	Uint8 *row;
	char *error = NULL;
	Uint8 magic[2];
	int ascii;

	if ( ! src ) {
		goto done;
	}

	SDL_RWread(src, magic, 2, 1);
	ascii = (magic[1] == '3');

	width = ReadNumber(src);
	height = ReadNumber(src);
	if(width <= 0 || height <= 0) {
		error = "Unable to read image width and height";
		goto done;
	}

	maxval = ReadNumber(src);
	if(maxval <= 0 || maxval > 255) {
		error = "unsupported ppm format";
		goto done;
	}
	/* binary PPM allows just a single character of whitespace after
	   the maxval, and we've already consumed it */

	/* 24-bit surface in R,G,B byte order */
	surface = SDL_AllocSurface(SDL_SWSURFACE, width, height, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				   0x000000ff, 0x0000ff00, 0x00ff0000,
#else
				   0x00ff0000, 0x0000ff00, 0x000000ff,
#endif
				   0);
	if ( surface == NULL ) {
		error = "Out of memory";
		goto done;
	}

	/* Read the image into the surface */
	row = surface->pixels;
	bpl = width * 3;
	for(y = 0; y < height; y++) {
		if(ascii) {
			int i;
			for(i = 0; i < bpl; i++) {
				int c;
				c = ReadNumber(src);
				if(c < 0) {
					error = "file truncated";
					goto done;
				}
				row[i] = c;
			}
		} else {
			if(!SDL_RWread(src, row, bpl, 1)) {
				error = "file truncated";
				goto done;
			}
		}
		if(maxval < 255) {
			/* scale up to full dynamic range (slow) */
			int i;
			for(i = 0; i < bpl; i++)
				row[i] = row[i] * 255 / maxval;
		}
		row += surface->pitch;
	}
done:
	if(error) {
		SDL_FreeSurface(surface);
		IMG_SetError(error);
		surface = NULL;
	}
	return(surface);
}

#else

/* See if an image is contained in a data source */
int IMG_isPPM(SDL_RWops *src)
{
	return(0);
}

/* Load a PPM type image from an SDL datasource */
SDL_Surface *IMG_LoadPPM_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_PPM */
