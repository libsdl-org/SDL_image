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
 
/* This is a ILBM image file loading framework
   Load IFF pictures, PBM & ILBM packing methods, with or without stencil
   Written by Daniel Morais ( Daniel@Morais.com ) in September 2001
*/

#include <stdio.h>
#include <stdlib.h>

#include "SDL_endian.h"
#include "SDL_image.h"

#ifdef LOAD_LBM


//===========================================================================
// DEFINES
//===========================================================================

#define MAXCOLORS 256

//===========================================================================
// STRUCTURES
//===========================================================================

// Structure for an IFF picture ( BMHD = Bitmap Header )

typedef struct
{
   Uint16 w, h;		// width & height of the bitmap in pixels
   Sint16 x, y;      // screen coordinates of the bitmap
   Uint8 planes;     // number of planes of the bitmap
   Uint8 mask;       // mask type ( 0 => no mask )
   Uint8 tcomp;      // compression type
   Uint8 pad1;       // dummy value, for padding
   Uint16 tcolor;    // transparent color
   Uint8 xAspect,    // pixel aspect ratio
         yAspect;
   Sint16  Lpage;		// width of the screen in pixels
   Sint16  Hpage;		// height of the screen in pixels

} BMHD;

//===========================================================================
// See if an image is contained in a data source

int IMG_isLBM( SDL_RWops *src )
{
	int   is_LBM;
	Uint8 magic[4+4+4];

	is_LBM = 0;
	if ( SDL_RWread( src, magic, 4+4+4, 1 ) ) 
	{
		if ( !memcmp( magic, "FORM", 4 ) && 
			( !memcmp( magic + 8, "PBM ", 4 ) || 
			  !memcmp( magic + 8, "ILBM", 4 ) ) ) 
		{
			is_LBM = 1;
		}
	}
	return( is_LBM );
}

//===========================================================================
// Load a IFF type image from an SDL datasource

SDL_Surface *IMG_LoadLBM_RW( SDL_RWops *src )
{
	SDL_Surface *Image;
	Uint8       id[4], pbm, colormap[MAXCOLORS*3], *MiniBuf, *ptr, count, color, msk;
   Uint32      size, bytesloaded, nbcolors;
	Uint32      i, j, bytesperline, nbplanes, plane, h;
	Uint32      remainingbytes;
	int         width;
	BMHD	      bmhd;
	char        *error;

	Image   = NULL;
	error   = NULL;
	MiniBuf = NULL;

	if ( src == NULL ) goto done;

	if ( !SDL_RWread( src, id, 4, 1 ) ) 
	{
	   error="error reading IFF chunk";
		goto done;
	}

	if ( !SDL_RWread( src, &size, 4, 1 ) ) // Should be the size of the file minus 4+4 ( 'FORM'+size )
	{
	   error="error reading IFF chunk size";
		goto done;
	}

	// As size is not used here, no need to swap it
	
	if ( memcmp( id, "FORM", 4 ) != 0 ) 
	{
	   error="not a IFF file";
		goto done;
	}

	if ( !SDL_RWread( src, id, 4, 1 ) ) 
	{
	   error="error reading IFF chunk";
		goto done;
	}

	pbm = 0;

	if ( !memcmp( id, "PBM ", 4 ) ) pbm = 1; // File format : PBM=Packed Bitmap, ILBM=Interleaved Bitmap
	else if ( memcmp( id, "ILBM", 4 ) )
	{
	   error="not a IFF picture";
		goto done;
	}

	nbcolors = 0;

	memset( &bmhd, 0, sizeof( BMHD ) );

	while ( memcmp( id, "BODY", 4 ) != 0 ) 
	{
		if ( !SDL_RWread( src, id, 4, 1 ) ) 
		{
	      error="error reading IFF chunk";
			goto done;
		}

		if ( !SDL_RWread( src, &size, 4, 1 ) ) 
		{
	      error="error reading IFF chunk size";
			goto done;
		}

		bytesloaded = 0;

		size = SDL_SwapBE32( size );
		
		if ( !memcmp( id, "BMHD", 4 ) ) // Bitmap header
		{
			if ( !SDL_RWread( src, &bmhd, sizeof( BMHD ), 1 ) )
			{
			   error="error reading BMHD chunk";
				goto done;
			}

			bytesloaded = sizeof( BMHD );

			bmhd.w 		= SDL_SwapBE16( bmhd.w );
			bmhd.h 		= SDL_SwapBE16( bmhd.h );
			bmhd.x 		= SDL_SwapBE16( bmhd.x );
			bmhd.y 		= SDL_SwapBE16( bmhd.y );
			bmhd.tcolor = SDL_SwapBE16( bmhd.tcolor );
			bmhd.Lpage 	= SDL_SwapBE16( bmhd.Lpage );
			bmhd.Hpage 	= SDL_SwapBE16( bmhd.Hpage );
		}

		if ( !memcmp( id, "CMAP", 4 ) ) // palette ( Color Map )
		{
			if ( !SDL_RWread( src, &colormap, size, 1 ) )
			{
			   error="error reading CMAP chunk";
				goto done;
			}

			bytesloaded = size;
			nbcolors = size / 3;
		}

		if ( memcmp( id, "BODY", 4 ) )
		{
		   if ( size & 1 )	++size;  	// padding !
			size -= bytesloaded;
			if ( size )	SDL_RWseek( src, size, SEEK_CUR ); // skip the remaining bytes of this chunk
		}
	}

	// compute some usefull values, based on the bitmap header

	width = ( bmhd.w + 15 ) & 0xFFFFFFF0;        // Width in pixels modulo 16

	bytesperline = ( ( bmhd.w + 15 ) / 16 ) * 2;	// Number of bytes per line

	nbplanes = bmhd.planes;                      // Number of planes

	if ( pbm )                                   // File format : 'Packed Bitmap'
	{
	   bytesperline *= 8;
		nbplanes = 1;
	}

	if ( bmhd.mask ) ++nbplanes;                 // There is a mask ( 'stencil' )

	// Allocate memory for a temporary buffer ( used for decompression/deinterleaving )

	if ( ( MiniBuf = (void *)malloc( bytesperline * nbplanes ) ) == NULL )
	{
	   error="no enough memory for temporary buffer";
		goto done;
	}

	// Create the surface

	if ( ( Image = SDL_CreateRGBSurface( SDL_SWSURFACE, width, bmhd.h, 8, 0, 0, 0, 0 ) ) == NULL )
	   goto done;

	// Update palette informations

	Image->format->palette->ncolors = nbcolors;

	ptr = &colormap[0];

	for ( i=0; i<nbcolors; i++ )
	{
	   Image->format->palette->colors[i].r = *ptr++;
	   Image->format->palette->colors[i].g = *ptr++;
	   Image->format->palette->colors[i].b = *ptr++;
	}

	// Get the bitmap

	for ( h=0; h < bmhd.h; h++ )
	{
		// uncompress the datas of each planes
			  
	   for ( plane=0; plane < nbplanes; plane++ )
		{
		   ptr = MiniBuf + ( plane * bytesperline );
	
			remainingbytes = bytesperline;
	
			if ( bmhd.tcomp == 1 )			// Datas are compressed
			{
			   do
				{
				   if ( !SDL_RWread( src, &count, 1, 1 ) )
					{
					   error="error reading BODY chunk";
						goto done;
					}
	
					if ( count & 0x80 )
					{
					   count ^= 0xFF;
						count += 2; // now it
						
						if ( !SDL_RWread( src, &color, 1, 1 ) )
						{
						   error="error reading BODY chunk";
							goto done;
						}
						memset( ptr, color, count );
					}
					else
					{
					   ++count;

						if ( !SDL_RWread( src, ptr, count, 1 ) )
						{
						   error="error reading BODY chunk";
							goto done;
						}
					}
	
					ptr += count;
					remainingbytes -= count;
	
				} while ( remainingbytes > 0 );
			}
			else	
			{
			   if ( !SDL_RWread( src, ptr, bytesperline, 1 ) )
				{
				   error="error reading BODY chunk";
					goto done;
				}
			}
		}
	
		// One line has been read, store it !

		ptr = Image->pixels;
		ptr += h * width;
	
		if ( pbm )                 // File format : 'Packed Bitmap'
		{
		   memcpy( ptr, MiniBuf, width );
		}
		else							   // We have to un-interlace the bits !
		{
		   size = ( width + 7 ) / 8;
	
			for ( i=0; i < size; i++ )
			{
			   memset( ptr, 0, 8 );
	
				for ( plane=0; plane < nbplanes; plane++ )
				{
				   color = *( MiniBuf + i + ( plane * bytesperline ) );
					msk = 0x80;
	
					for ( j=0; j<8; j++ )
					{
					   if ( ( plane + j ) <= 7 ) ptr[j] |= (Uint8)( color & msk ) >> ( 7 - plane - j );
						else 	                    ptr[j] |= (Uint8)( color & msk ) << ( plane + j - 7 );
	
						msk >>= 1;
					}
				}
				ptr += 8;
			}
		}
	}

done:

	if ( MiniBuf ) free( MiniBuf );

	if ( error ) 
	{
		IMG_SetError( error );
	   SDL_FreeSurface( Image );
		Image = NULL;
	}

	return( Image );
}

#else /* LOAD_LBM */

/* See if an image is contained in a data source */
int IMG_isLBM(SDL_RWops *src)
{
	return(0);
}

/* Load an IFF type image from an SDL datasource */
SDL_Surface *IMG_LoadLBM_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_LBM */
