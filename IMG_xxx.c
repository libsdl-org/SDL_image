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

/* This is a generic "format not supported" image framework */

#include <stdio.h>

#include "SDL_image.h"

#ifdef LOAD_XXX

/* See if an image is contained in a data source */
int IMG_isXXX(SDL_RWops *src)
{
	int is_XXX;

	is_XXX = 0;

	return(is_XXX);
}

/* Load a XXX type image from an SDL datasource */
SDL_Surface *IMG_LoadXXX_RW(SDL_RWops *src)
{
	if ( !src ) {
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}
	return(NULL);
}

#else

/* See if an image is contained in a data source */
int IMG_isXXX(SDL_RWops *src)
{
	return(0);
}

/* Load a XXX type image from an SDL datasource */
SDL_Surface *IMG_LoadXXX_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_XXX */
