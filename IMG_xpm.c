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
		if(memcmp(magic, "/* XPM */", 9) == 0) {
			is_XPM = 1;
		}
	}
	return(is_XPM);
}

static char *SDL_RWgets(char *string, int maxlen, SDL_RWops *src)
{
	int i;

	for ( i=0; i<(maxlen-1); ++i ) {
		if ( SDL_RWread(src, &string[i], 1, 1) <= 0 ) {
			/* EOF or error */
			if ( i == 0 ) {
				/* Hmm, EOF on initial read, return NULL */
				return NULL;
			}
			break;
		}
		/* In this case it's okay to use either '\r' or '\n'
		   as line separators because blank lines are just
		   ignored by the XPM format.
		*/
		if ( (string[i] == '\n') || (string[i] == '\r') ) {
			break;
		}
	}
	string[i] = '\0';
	return(string);
}

/* Hash table to look up colors from pixel strings */
#define STARTING_HASH_SIZE 256

struct hash_entry {
	char *key;
	Uint32 color;
	struct hash_entry *next;
};

struct color_hash {
	struct hash_entry **table;
	struct hash_entry *entries; /* array of all entries */
	struct hash_entry *next_free;
	int size;
	int maxnum;
};

static int hash_key(const char *key, int cpp, int size)
{
	int hash;

	hash = 0;
	while ( cpp-- > 0 ) {
		hash = hash * 33 + *key++;
	}
	return hash & (size - 1);
}

static struct color_hash *create_colorhash(int maxnum)
{
	int bytes, s;
	struct color_hash *hash;

	/* we know how many entries we need, so we can allocate
	   everything here */
	hash = malloc(sizeof *hash);
	if(!hash)
		return NULL;

	/* use power-of-2 sized hash table for decoding speed */
	for(s = STARTING_HASH_SIZE; s < maxnum; s <<= 1)
		;
	hash->size = s;
	hash->maxnum = maxnum;
	bytes = hash->size * sizeof(struct hash_entry **);
	hash->entries = NULL;	/* in case malloc fails */
	hash->table = malloc(bytes);
	if(!hash->table)
		return NULL;
	memset(hash->table, 0, bytes);
	hash->entries = malloc(maxnum * sizeof(struct hash_entry));
	if(!hash->entries)
		return NULL;
	hash->next_free = hash->entries;
	return hash;
}

static int add_colorhash(struct color_hash *hash,
                         char *key, int cpp, Uint32 color)
{
	int index = hash_key(key, cpp, hash->size);
	struct hash_entry *e = hash->next_free++;
	e->color = color;
	e->key = key;
	e->next = hash->table[index];
	hash->table[index] = e;
	return 1;
}

/* fast lookup that works if cpp == 1 */
#define QUICK_COLORHASH(hash, key) ((hash)->table[*(Uint8 *)(key)]->color)

static Uint32 get_colorhash(struct color_hash *hash, const char *key, int cpp)
{
	struct hash_entry *entry = hash->table[hash_key(key, cpp, hash->size)];
	while(entry) {
		if(memcmp(key, entry->key, cpp) == 0)
			return entry->color;
		entry = entry->next;
	}
	return 0;		/* garbage in - garbage out */
}

static void free_colorhash(struct color_hash *hash)
{
	if(hash && hash->table) {
		free(hash->table);
		free(hash->entries);
		free(hash);
	}
}

#define ARRAYSIZE(a) (int)(sizeof(a) / sizeof((a)[0]))

/*
 * convert colour spec to RGB (in 0xrrggbb format).
 * return 1 if successful. may scribble on the colorspec buffer.
 */
static int color_to_rgb(char *spec, Uint32 *rgb)
{
	/* poor man's rgb.txt */
	static struct { char *name; Uint32 rgb; } known[] = {
		{"none",  0xffffffff},
		{"black", 0x00000000},
		{"white", 0x00ffffff},
		{"red",   0x00ff0000},
		{"green", 0x0000ff00},
		{"blue",  0x000000ff}
	};

	if(spec[0] == '#') {
		char buf[7];
		++spec;
		switch(strlen(spec)) {
		case 3:
			buf[0] = buf[1] = spec[0];
			buf[2] = buf[3] = spec[1];
			buf[4] = buf[5] = spec[2];
			break;
		case 6:
			memcpy(buf, spec, 6);
			break;
		case 12:
			buf[0] = spec[0];
			buf[1] = spec[1];
			buf[2] = spec[4];
			buf[3] = spec[5];
			buf[4] = spec[8];
			buf[5] = spec[9];
			break;
		}
		buf[6] = '\0';
		*rgb = strtol(buf, NULL, 16);
		return 1;
	} else {
		int i;
		for(i = 0; i < ARRAYSIZE(known); i++)
			if(IMG_string_equals(known[i].name, spec)) {
				*rgb = known[i].rgb;
				return 1;
			}
		return 0;
	}
}

static char *skipspace(char *p)
{
	while(isspace((unsigned char)*p))
	      ++p;
	return p;
}

static char *skipnonspace(char *p)
{
	while(!isspace((unsigned char)*p) && *p)
		++p;
	return p;
}

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Load a XPM type image from an SDL datasource */
SDL_Surface *IMG_LoadXPM_RW(SDL_RWops *src)
{
	SDL_Surface *image;
	char line[1024];
	char *here;
	int index;
	int x, y;
	int w, h, ncolors, cpp;
	int pixels_len;
	char *pixels = NULL;
	int indexed;
	Uint8 *dst;
	struct color_hash *colors;
	SDL_Color *im_colors = NULL;
	char *keystrings, *nextkey;
	char *error = NULL;

	/* Skip to the first string, which describes the image */
	do {
	        here = SDL_RWgets(line, sizeof(line), src);
		if ( !here ) {
			IMG_SetError("Premature end of data");
			return(NULL);
		}
		here = skipspace(here);
	} while(*here != '"');
	/*
	 * The header string of an XPMv3 image has the format
	 *
	 * <width> <height> <ncolors> <cpp> [ <hotspot_x> <hotspot_y> ]
	 *
	 * where the hotspot coords are intended for mouse cursors.
	 * Right now we don't use the hotspots but it should be handled
	 * one day.
	 */
	if(sscanf(here + 1, "%d %d %d %d", &w, &h, &ncolors, &cpp) != 4
	   || w <= 0 || h <= 0 || ncolors <= 0 || cpp <= 0) {
		IMG_SetError("Invalid format description");
		return(NULL);
	}

	keystrings = malloc(ncolors * cpp);
	if(!keystrings) {
		IMG_SetError("Out of memory");
		free(pixels);
		return NULL;
	}
	nextkey = keystrings;

	/* Create the new surface */
	if(ncolors <= 256) {
		indexed = 1;
		image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8,
					     0, 0, 0, 0);
		im_colors = image->format->palette->colors;
		image->format->palette->ncolors = ncolors;
	} else {
		indexed = 0;
		image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
					     0xff0000, 0x00ff00, 0x0000ff, 0);
	}
	if(!image) {
		/* Hmm, some SDL error (out of memory?) */
		free(pixels);
		return(NULL);
	}

	/* Read the colors */
	colors = create_colorhash(ncolors);
	if ( ! colors ) {
		error = "Out of memory";
		goto done;
	}
	for(index = 0; index < ncolors; ++index ) {
		char *key;
		int len;

		do {
			here = SDL_RWgets(line, sizeof(line), src);
			if(!here) {
				error = "Premature end of data";
				goto done;
			}
			here = skipspace(here);
		} while(*here != '"');

		++here;
		len = strlen(here);
		if(len < cpp + 7)
			continue;	/* cannot be a valid line */

		key = here;
		key[cpp] = '\0';
		here += cpp + 1;

		/* parse a colour definition */
		for(;;) {
			char nametype;
			char *colname;
			char delim;
			Uint32 rgb;

			here = skipspace(here);
			nametype = *here;
			here = skipnonspace(here);
			here = skipspace(here);
			colname = here;
			while(*here && !isspace((unsigned char)*here)
			      && *here != '"')
				here++;
			if(!*here) {
				error = "color parse error";
				goto done;
			}
			if(nametype == 's')
				continue;      /* skip symbolic colour names */

			delim = *here;
			*here = '\0';
			if(delim)
			    here++;

			if(!color_to_rgb(colname, &rgb))
				continue;

			memcpy(nextkey, key, cpp);
			if(indexed) {
				SDL_Color *c = im_colors + index;
				c->r = rgb >> 16;
				c->g = rgb >> 8;
				c->b = rgb;
				add_colorhash(colors, nextkey, cpp, index);
			} else
				add_colorhash(colors, nextkey, cpp, rgb);
			nextkey += cpp;
			if(rgb == 0xffffffff)
				SDL_SetColorKey(image, SDL_SRCCOLORKEY,
						indexed ? index : rgb);
			break;
		}
	}

	/* Read the pixels */
	pixels_len = w * cpp;
	pixels = malloc(MAX(pixels_len + 5, 20));
	if(!pixels) {
		error = "Out of memory";
		goto done;
	}
	dst = image->pixels;
	for (y = 0; y < h; ) {
		char *s;
		char c;
		do {
			if(SDL_RWread(src, &c, 1, 1) <= 0) {
				error = "Premature end of data";
				goto done;
			}
		} while(c == ' ');
		if(c != '"') {
			/* comment or empty line, skip it */
			while(c != '\n' && c != '\r') {
				if(SDL_RWread(src, &c, 1, 1) <= 0) {
					error = "Premature end of data";
					goto done;
				}
			}
			continue;
		}
		if(SDL_RWread(src, pixels, pixels_len + 3, 1) <= 0) {
			error = "Premature end of data";
			goto done;
		}
		s = pixels;
		if(indexed) {
			/* optimization for some common cases */
			if(cpp == 1)
				for(x = 0; x < w; x++)
					dst[x] = QUICK_COLORHASH(colors,
								 s + x);
			else
				for(x = 0; x < w; x++)
					dst[x] = get_colorhash(colors,
							       s + x * cpp,
							       cpp);
		} else {
			for (x = 0; x < w; x++)
				((Uint32*)dst)[x] = get_colorhash(colors,
								  s + x * cpp,
								  cpp);
		}
		dst += image->pitch;
		y++;
	}

done:
	if(error) {
		if(image)
			SDL_FreeSurface(image);
		image = NULL;
		IMG_SetError(error);
	}
	free(pixels);
	free(keystrings);
	free_colorhash(colors);
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
