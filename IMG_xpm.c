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

/* This is an XPM image file loading framework */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "SDL_image.h"

#ifdef LOAD_XPM

/* See if an image is contained in a data source */
int IMG_isXPM(SDL_RWops *src)
{
	int is_XPM;
	char magic[10];

	is_XPM = 0;
	if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
		if ( strncmp(magic, "/* XPM */", 9) == 0 ) {
			is_XPM = 1;
		}
	}
	return(is_XPM);
}

static char *my_strdup(const char *string)
{
	char *newstring;

	newstring = (char *)malloc(strlen(string)+1);
	if ( newstring ) {
		strcpy(newstring, string);
	}
	return(newstring);
}

/* Not exactly the same semantics as strncasecmp(), but portable */
static int my_strncasecmp(const char *str1, const char *str2, int len)
{
	if ( len == 0 ) {
		len = strlen(str2);
		if ( len != strlen(str1) ) {
			return(-1);
		}
	}
	while ( len-- > 0 ) {
		if ( tolower(*str1++) != tolower(*str2++) ) {
			return(-1);
		}
	}
	return(0);
}

static char *SDL_RWgets(char *string, int maxlen, SDL_RWops *src)
{
	int i;

	for ( i=0; i<(maxlen-1); ++i ) {
		if ( SDL_RWread(src, &string[i], 1, 1) <= 0 ) {
			/* EOF or error */
			if ( i == 0 ) {
				/* Hmm, EOF on initial read, return NULL */
				string = NULL;
			}
			break;
		}
		/* In this case it's okay to use either '\r' or '\n'
		   as line separators because blank lines are just
		   ignored by the XPM format.
		*/
		if ( (string[i] == '\r') || (string[i] == '\n') ) {
			break;
		}
	}
	if ( string ) {
		string[i] = '\0';
	}
	return(string);
}

/* Hash table to look up colors from pixel strings */
#define HASH_SIZE	256
struct color_hash {
	struct hash_entry {
		int keylen;
		char *key;
		Uint32 color;
		struct hash_entry *next;
	} *entries[HASH_SIZE];
};

static int hash_key(const char *key, int cpp)
{
	int hash;

	hash = 0;
	while ( cpp-- > 0 ) {
		hash += *key++;
	}
	return(hash%HASH_SIZE);
}

static struct color_hash *create_colorhash(void)
{
	struct color_hash *hash;

	hash = (struct color_hash *)malloc(sizeof *hash);
	if ( hash ) {
		memset(hash, 0, (sizeof *hash));
	}
	return(hash);
}

static int add_colorhash(struct color_hash *hash,
                         const char *key, int cpp, Uint32 color)
{
	int hash_index;
	struct hash_entry *prev, *entry;

	/* Create the hash entry */
	entry = (struct hash_entry *)malloc(sizeof *entry);
	if ( ! entry ) {
		return(0);
	}
	entry->keylen = cpp;
	entry->key = my_strdup(key);
	if ( ! entry->key ) {
		free(entry);
		return(0);
	}
	entry->color = color;
	entry->next = NULL;

	/* Add it to the hash table */
	hash_index = hash_key(key, cpp);
	for ( prev = hash->entries[hash_index];
	      prev && prev->next; prev = prev->next ) {
		/* Go to the end of the list */ ;
	}
	if ( prev ) {
		prev->next = entry;
	} else {
		hash->entries[hash_index] = entry;
	}
	return(1);
}

static int get_colorhash(struct color_hash *hash,
                         const char *key, int cpp, Uint32 *color)
{
	int hash_index;
	struct hash_entry *entry;

	hash_index = hash_key(key, cpp);
	for ( entry = hash->entries[hash_index]; entry; entry = entry->next ) {
		if ( strncmp(key, entry->key, entry->keylen) == 0 ) {
			*color = entry->color;
			return(1);
		}
	}
	return(0);
}

static void free_colorhash(struct color_hash *hash)
{
	int i;
	struct hash_entry *entry, *freeable;

	for ( i=0; i<HASH_SIZE; ++i ) {
		entry = hash->entries[i];
		while ( entry ) {
			freeable = entry;
			entry = entry->next;
			free(freeable->key);
			free(freeable);
		}
	}
	free(hash);
}

static int color_to_rgb(const char *colorspec, int *r, int *g, int *b)
{
	char rbuf[3];
	char gbuf[3];
	char bbuf[3];

	/* Handle monochrome black and white */
	if ( my_strncasecmp(colorspec, "black", 0) == 0 ) {
		*r = 0;
		*g = 0;
		*b = 0;
		return(1);
	}
	if ( my_strncasecmp(colorspec, "white", 0) == 0 ) {
		*r = 255;
		*g = 255;
		*b = 255;
		return(1);
	}

	/* Normal hexidecimal color */
	switch (strlen(colorspec)) {
		case 3:
			rbuf[0] = colorspec[0];
			rbuf[1] = colorspec[0];
			gbuf[0] = colorspec[1];
			gbuf[1] = colorspec[1];
			bbuf[0] = colorspec[2];
			bbuf[1] = colorspec[2];
			break;
		case 6:
			rbuf[0] = colorspec[0];
			rbuf[1] = colorspec[1];
			gbuf[0] = colorspec[2];
			gbuf[1] = colorspec[3];
			bbuf[0] = colorspec[4];
			bbuf[1] = colorspec[5];
			break;
		case 12:
			rbuf[0] = colorspec[0];
			rbuf[1] = colorspec[1];
			gbuf[0] = colorspec[4];
			gbuf[1] = colorspec[5];
			bbuf[0] = colorspec[8];
			bbuf[1] = colorspec[9];
			break;
		default:
			return(0);
	}
	rbuf[2] = '\0';
	*r = (int)strtol(rbuf, NULL, 16);
	gbuf[2] = '\0';
	*g = (int)strtol(gbuf, NULL, 16);
	bbuf[2] = '\0';
	*b = (int)strtol(bbuf, NULL, 16);
	return(1);
}

/* Load a XPM type image from an SDL datasource */
SDL_Surface *IMG_LoadXPM_RW(SDL_RWops *src)
{
	SDL_Surface *image;
	char line[1024];
	char *here, *stop;
	int index;
	int i, x, y;
	int w, h, ncolors, cpp;
	int found;
	int r, g, b;
	Uint32 colorkey;
	char *colorkey_string;
	int pixels_len;
	char *pixels;
	int indexed;
	Uint8 *dst;
	Uint32 pixel;
	struct color_hash *colors;

	/* Skip to the first string, which describes the image */
	image = NULL;
	do {
		here = SDL_RWgets(line, sizeof(line), src);
		if ( ! here ) {
			IMG_SetError("Premature end of data");
			return(NULL);
		}
		if ( *here == '"' ) {
			++here;
			/* Skip to width */
			while ( isspace(*here) ) ++here;
			w = atoi(here);
			while ( ! isspace(*here) ) ++here;
			/* Skip to height */
			while ( isspace(*here) ) ++here;
			h = atoi(here);
			while ( ! isspace(*here) ) ++here;
			/* Skip to number of colors */
			while ( isspace(*here) ) ++here;
			ncolors = atoi(here);
			while ( ! isspace(*here) ) ++here;
			/* Skip to characters per pixel */
			while ( isspace(*here) ) ++here;
			cpp = atoi(here);
			while ( ! isspace(*here) ) ++here;

			/* Verify the parameters */
			if ( !w || !h || !ncolors || !cpp ) {
				IMG_SetError("Invalid format description");
				return(NULL);
			}
			pixels_len = 1+w*cpp+1+1;
			pixels = (char *)malloc(pixels_len);
			if ( ! pixels ) {
				IMG_SetError("Out of memory");
				return(NULL);
			}

			/* Create the new surface */
			if ( ncolors <= 256 ) {
				indexed = 1;
				image = SDL_CreateRGBSurface(SDL_SWSURFACE,
							w, h, 8, 0, 0, 0, 0);
			} else {
				int rmask, gmask, bmask;
				indexed = 0;
				if ( SDL_BYTEORDER == SDL_BIG_ENDIAN ) {
					rmask = 0x000000ff;
					gmask = 0x0000ff00;
					bmask = 0x00ff0000;
				} else {
					rmask = 0x00ff0000;
					gmask = 0x0000ff00;
					bmask = 0x000000ff;
				}
				image = SDL_CreateRGBSurface(SDL_SWSURFACE,
							w, h, 32,
							rmask, gmask, bmask, 0);
			}
			if ( ! image ) {
				/* Hmm, some SDL error (out of memory?) */
				free(pixels);
				return(NULL);
			}
		}
	} while ( ! image );

	/* Read the colors */
	colors = create_colorhash();
	if ( ! colors ) {
		SDL_FreeSurface(image);
		free(pixels);
		IMG_SetError("Out of memory");
		return(NULL);
	}
	colorkey_string = NULL;
	for ( index=0; index < ncolors; ++index ) {
		here = SDL_RWgets(line, sizeof(line), src);
		if ( ! here ) {
			SDL_FreeSurface(image);
			image = NULL;
			IMG_SetError("Premature end of data");
			goto done;
		}
		if ( *here == '"' ) {
			const char *key;
			++here;
			/* Grab the pixel key */
			key = here;
			for ( i=0; i<cpp; ++i ) {
				if ( ! *here++ ) {
					/* Parse error */
					continue;
				}
			}
			if ( *here ) {
				*here++ = '\0';
			}
			/* Find the color identifier */
			found = 0;
			while ( *here && ! found ) {
				while ( isspace(*here) ) ++here;
				if ( (*here != 'c') &&
				     (*here != 'g') &&
				     (*here != 'm') ) {
					/* Skip color type */
					while ( *here && !isspace(*here) )
						++here;
					/* Skip color name */
					while ( isspace(*here) ) ++here;
					while ( *here && !isspace(*here) )
						++here;
					continue;
				}
				++here;
				while ( isspace(*here) ) ++here;
				if ( my_strncasecmp(here, "None", 4) == 0 ) {
					colorkey_string = my_strdup(key);
					if ( indexed ) {
						colorkey = (Uint32)index;
					} else {
						colorkey = 0xFFFFFFFF;
					}
					found = 1;
					continue;
				}
				if ( *here == '#' ) {
					++here;
				}
				while ( isspace(*here) ) ++here;
				for ( stop=here; isalnum(*stop); ++stop ) {
					/* Skip the pixel color */;
				}
				*stop++ = '\0';
				found = color_to_rgb(here, &r, &g, &b);
				if ( found ) {
					if ( indexed ) {
						SDL_Color *color;
						color = &image->format->palette->colors[index];
						color->r = (Uint8)r;
						color->g = (Uint8)g;
						color->b = (Uint8)b;
						pixel = index;
					} else {
						pixel = (r<<16)|(g<<8)|b;
					}
					add_colorhash(colors, key, cpp, pixel);
				}
				*here = '\0';
			}
			if ( ! found ) {
				/* Hum, couldn't parse a color.. */;
			}
		}
	}

	/* Read the pixels */
	for ( y=0; y < h; ) {
		here = SDL_RWgets(pixels, pixels_len, src);
		if ( ! here ) {
			SDL_FreeSurface(image);
			image = NULL;
			IMG_SetError("Premature end of data");
			goto done;
		}
		if ( *here == '"' ) {
			++here;
			dst = (Uint8 *)image->pixels + y*image->pitch;
			for ( x=0; x<w; ++x ) {
				pixel = 0;
				if ( colorkey_string &&
				     (strncmp(here,colorkey_string,cpp)==0) ) {
					pixel = colorkey;
				} else {
					get_colorhash(colors, here,cpp, &pixel);
				}
				if ( indexed ) {
					*dst++ = pixel;
				} else {
					*((Uint32 *)dst)++ = pixel;
				}
				for ( i=0; *here && i<cpp; ++i ) {
					++here;
				}
			}
			++y;
		}
	}
	if ( colorkey_string ) {
        	SDL_SetColorKey(image, SDL_SRCCOLORKEY, colorkey);
	}
done:
	free(pixels);
	free_colorhash(colors);
	if ( colorkey_string ) {
		free(colorkey_string);
	}
	return(image);
}

#else

/* See if an image is contained in a data source */
int IMG_isXPM(SDL_RWops *src)
{
	return(0);
}

/* Load a XPM type image from an SDL datasource */
SDL_Surface *IMG_LoadXPM_RW(SDL_RWops *src)
{
	return(NULL);
}

#endif /* LOAD_XPM */
