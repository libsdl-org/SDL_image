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

/* This is a PPM image file loading framework */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "SDL_image.h"

#ifdef LOAD_PPM

/* See if an image is contained in a data source */
int IMG_isPPM(SDL_RWops *src)
{
	int is_PPM;
	char magic[2];

	is_PPM = 0;
	if ( SDL_RWread(src, magic, 2, 1) ) {
		if ( (strncmp(magic, "P6", 2) == 0) ||
		     (strncmp(magic, "P3", 2) == 0) ) {
			is_PPM = 1;
		}
		/* P3 is the ASCII PPM format, which is not yet supported */
		if ( strncmp(magic, "P3", 2) == 0 ) {
			is_PPM = 0;
		}
	}
	return(is_PPM);
}

/* Load a PPM type image from an SDL datasource */
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
					return(0);
				}
			} while ( (ch != '\r') && (ch != '\n') );
		}
	} while ( isspace(ch) );

	/* Add up the number */
	do {
		number *= 10;
		number += ch-'0';

		if ( ! SDL_RWread(src, &ch, 1, 1) ) {
			return(0);
		}
	} while ( isdigit(ch) );

	return(number);
}

SDL_Surface *IMG_LoadPPM_RW(SDL_RWops *src)
{
	SDL_Surface *surface;
	int width, height;
	int maxval, x, y;
	Uint8 *row;
	Uint8 rgb[3];

	/* Initialize the data we will clean up when we're done */
	surface = NULL;

	/* Check to make sure we have something to do */
	if ( ! src ) {
		goto done;
	}

	/* Read the magic header */
	SDL_RWread(src, rgb, 2, 1);

	/* Read the width and height */
	width = ReadNumber(src);
	height = ReadNumber(src);
	if ( ! width || ! height ) {
		IMG_SetError("Unable to read image width and height");
		goto done;
	}

	/* Read the maximum color component value, and skip to image data */
	maxval = ReadNumber(src);
	do {
		if ( ! SDL_RWread(src, rgb, 1, 1) ) {
			IMG_SetError("Unable to read max color component");
			goto done;
		}
	} while ( isspace(rgb[0]) );
	SDL_RWseek(src, -1, SEEK_CUR);

	/* Create the surface of the appropriate type */
	surface = SDL_AllocSurface(SDL_SWSURFACE, width, height, 24,
				0x00FF0000,0x0000FF00,0x000000FF,0);
	if ( surface == NULL ) {
		IMG_SetError("Out of memory");
		goto done;
	}

	/* Read the image into the surface */
	if(!SDL_RWread(src, surface->pixels, 3, surface->h*surface->w)){
		SDL_FreeSurface(surface);
		IMG_SetError("Error reading PPM data");
		surface = NULL;
	}
done:
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
