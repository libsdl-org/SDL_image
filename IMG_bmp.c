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

/* This is a BMP image file loading framework */

#include <stdio.h>
#include <string.h>

#include "SDL_image.h"

#ifdef LOAD_BMP

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
	int is_BMP;
	char magic[2];

	is_BMP = 0;
	if ( SDL_RWread(src, magic, 2, 1) ) {
		if ( strncmp(magic, "BM", 2) == 0 ) {
			is_BMP = 1;
		}
	}
	return(is_BMP);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
	return(SDL_LoadBMP_RW(src, 0));
}

#else

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
	return(0);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_BMP */
