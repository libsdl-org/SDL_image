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

/* This is a PCX image file loading framework */

#include <stdio.h>

#include "SDL_endian.h"

#include "SDL_image.h"

#ifdef LOAD_PCX

struct PCXheader {
	Uint8 Manufacturer;
	Uint8 Version;
	Uint8 Encoding;
	Uint8 BitsPerPixel;
	Sint16 Xmin, Ymin, Xmax, Ymax;
	Sint16 HDpi, VDpi;
	Uint8 Colormap[48];
	Uint8 Reserved;
	Uint8 NPlanes;
	Sint16 BytesPerLine;
	Sint16 PaletteInfo;
	Sint16 HscreenSize;
	Sint16 VscreenSize;
	Uint8 Filler[54];
};

/* See if an image is contained in a data source */
int IMG_isPCX(SDL_RWops *src)
{
	int is_PCX;
	const int ZSoft_Manufacturer = 10;
	const int PC_Paintbrush_Version = 5;
	const int PCX_RunLength_Encoding = 1;
	struct PCXheader pcxh;

	is_PCX = 0;
	if ( SDL_RWread(src, &pcxh, sizeof(pcxh), 1) == 1 ) {
		if ( (pcxh.Manufacturer == ZSoft_Manufacturer) &&
		     (pcxh.Version == PC_Paintbrush_Version) &&
		     (pcxh.Encoding == PCX_RunLength_Encoding) ) {
			is_PCX = 1;
		}
	}
	return(is_PCX);
}

/* Load a PCX type image from an SDL datasource */
SDL_Surface *IMG_LoadPCX_RW(SDL_RWops *src)
{
	struct PCXheader pcxh;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	SDL_Surface *surface;
	int width, height;
	int i, index, x, y;
	int count;
	Uint8 *row, ch;
	int read_error;

	/* Initialize the data we will clean up when we're done */
	surface = NULL;
	read_error = 0;

	/* Check to make sure we have something to do */
	if ( ! src ) {
		goto done;
	}

	/* Read and convert the header */
	if ( ! SDL_RWread(src, &pcxh, sizeof(pcxh), 1) ) {
		goto done;
	}
	pcxh.Xmin = SDL_SwapLE16(pcxh.Xmin);
	pcxh.Ymin = SDL_SwapLE16(pcxh.Ymin);
	pcxh.Xmax = SDL_SwapLE16(pcxh.Xmax);
	pcxh.Ymax = SDL_SwapLE16(pcxh.Ymax);
	pcxh.BytesPerLine = SDL_SwapLE16(pcxh.BytesPerLine);

	/* Create the surface of the appropriate type */
	width = (pcxh.Xmax - pcxh.Xmin) + 1;
	height = (pcxh.Ymax - pcxh.Ymin) + 1;
	Rmask = Gmask = Bmask = Amask = 0 ; 
	if ( pcxh.BitsPerPixel * pcxh.NPlanes > 16 ) {
		if ( SDL_BYTEORDER == SDL_LIL_ENDIAN ) {
			Rmask = 0x000000FF;
			Gmask = 0x0000FF00;
			Bmask = 0x00FF0000;
		} else {
		        int s = (pcxh.NPlanes == 4) ? 0 : 8;
			Rmask = 0xFF0000;
			Gmask = 0x00FF00;
			Bmask = 0x0000FF;
		}
	}
	surface = SDL_AllocSurface(SDL_SWSURFACE, width, height,
			pcxh.BitsPerPixel*pcxh.NPlanes,Rmask,Gmask,Bmask,Amask);
	if ( surface == NULL ) {
		IMG_SetError("Out of memory");
		goto done;
	}

	/* Decode the image to the surface */
	for ( y=0; y<surface->h; ++y ) {
		for ( i=0; i<pcxh.NPlanes; ++i ) {
			row = (Uint8 *)surface->pixels + y*surface->pitch;
			index = i;
			for ( x=0; x<pcxh.BytesPerLine; ) {
				if ( ! SDL_RWread(src, &ch, 1, 1) ) {
					read_error = 1;
					goto done;
				}
				if ( (ch & 0xC0) == 0xC0 ) {
					count = ch & 0x3F;
					SDL_RWread(src, &ch, 1, 1);
				} else {
					count = 1;
				}
				while ( count-- ) {
					row[index] = ch;
					++x;
					index += pcxh.NPlanes;
				}
			}
		}
	}

	/* Look for the palette, if necessary */
	switch (surface->format->BitsPerPixel) {
	    case 1: {
		SDL_Color *colors = surface->format->palette->colors;

		colors[0].r = 0x00;
		colors[0].g = 0x00;
		colors[0].b = 0x00;
		colors[1].r = 0xFF;
		colors[1].g = 0xFF;
		colors[1].b = 0xFF;
	    }
	    break;

	    case 8: {
		SDL_Color *colors = surface->format->palette->colors;

		/* Look for the palette */
		do {
			if ( ! SDL_RWread(src, &ch, 1, 1) ) {
				read_error = 1;
				goto done;
			}
		} while ( ch != 12 );

		/* Set the image surface palette */
		for ( i=0; i<256; ++i ) {
			SDL_RWread(src, &colors[i].r, 1, 1);
			SDL_RWread(src, &colors[i].g, 1, 1);
			SDL_RWread(src, &colors[i].b, 1, 1);
		}
	    }
	    break;

	    default: {
		/* Don't do anything... */;
	    }
	    break;
	}

done:
	if ( read_error ) {
		SDL_FreeSurface(surface);
		IMG_SetError("Error reading PCX data");
		surface = NULL;
	}
	return(surface);
}

#else

/* See if an image is contained in a data source */
int IMG_isPCX(SDL_RWops *src)
{
	return(0);
}

/* Load a PCX type image from an SDL datasource */
SDL_Surface *IMG_LoadPCX_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_PCX */
