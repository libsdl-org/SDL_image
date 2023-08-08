/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

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

#if (!defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND)) || !defined(BMP_USES_IMAGEIO)

/* This is a BMP image file loading framework
 *
 * ICO/CUR file support is here as well since it uses similar internal
 * representation
 *
 * A good test suite of BMP images is available at:
 * http://entropymine.com/jason/bmpsuite/bmpsuite/html/bmpsuite.html
 */

#include <SDL3_image/SDL_image.h>
#include "IMG.h"

#ifdef LOAD_BMP

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
    Sint64 start;
    int is_BMP;
    char magic[2];

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_BMP = 0;
    if (SDL_RWread(src, magic, sizeof(magic)) == sizeof(magic)) {
        if (SDL_strncmp(magic, "BM", 2) == 0) {
            is_BMP = 1;
        }
    }
    SDL_RWseek(src, start, SDL_RW_SEEK_SET);
    return(is_BMP);
}

static int IMG_isICOCUR(SDL_RWops *src, int type)
{
    Sint64 start;
    int is_ICOCUR;

    /* The Win32 ICO file header (14 bytes) */
    Uint16 bfReserved;
    Uint16 bfType;
    Uint16 bfCount;

    if (!src) {
        return 0;
    }
    start = SDL_RWtell(src);
    is_ICOCUR = 0;
    if (SDL_ReadU16LE(src, &bfReserved) &&
        SDL_ReadU16LE(src, &bfType) &&
        SDL_ReadU16LE(src, &bfCount) &&
        (bfReserved == 0) && (bfType == type) && (bfCount != 0)) {
        is_ICOCUR = 1;
    }
    SDL_RWseek(src, start, SDL_RW_SEEK_SET);

    return (is_ICOCUR);
}

int IMG_isICO(SDL_RWops *src)
{
    return IMG_isICOCUR(src, 1);
}

int IMG_isCUR(SDL_RWops *src)
{
    return IMG_isICOCUR(src, 2);
}

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_endian.h>

/* Compression encodings for BMP files */
#ifndef BI_RGB
#define BI_RGB      0
#define BI_RLE8     1
#define BI_RLE4     2
#define BI_BITFIELDS    3
#endif

static SDL_Surface *LoadBMP_RW (SDL_RWops *src, SDL_bool freesrc)
{
    return SDL_LoadBMP_RW(src, freesrc);
}

static SDL_Surface *
LoadICOCUR_RW(SDL_RWops * src, int type, SDL_bool freesrc)
{
    SDL_bool was_error = SDL_TRUE;
    Sint64 fp_offset = 0;
    int bmpPitch;
    int i,j, pad;
    SDL_Surface *surface = NULL;
    /*
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    */
    Uint8 *bits;
    int ExpandBMP;
    Uint8 maxCol = 0;
    Uint32 icoOfs = 0;
    Uint32 palette[256];

    /* The Win32 ICO file header (14 bytes) */
    Uint16 bfReserved;
    Uint16 bfType;
    Uint16 bfCount;

    /* The Win32 BITMAPINFOHEADER struct (40 bytes) */
    Uint32 biSize;
    Sint32 biWidth;
    Sint32 biHeight;
    /* Uint16 biPlanes; */
    Uint16 biBitCount;
    Uint32 biCompression;
    /*
    Uint32 biSizeImage;
    Sint32 biXPelsPerMeter;
    Sint32 biYPelsPerMeter;
    Uint32 biClrImportant;
    */
    Uint32 biClrUsed;

    /* Make sure we are passed a valid data source */
    if (src == NULL) {
        goto done;
    }

    /* Read in the ICO file header */
    fp_offset = SDL_RWtell(src);

    if (!SDL_ReadU16LE(src, &bfReserved) ||
        !SDL_ReadU16LE(src, &bfType) ||
        !SDL_ReadU16LE(src, &bfCount) ||
        (bfReserved != 0) || (bfType != type) || (bfCount == 0)) {
        IMG_SetError("File is not a Windows %s file", type == 1 ? "ICO" : "CUR");
        goto done;
    }

    /* Read the Win32 Icon Directory */
    for (i = 0; i < bfCount; i++) {
        /* Icon Directory Entries */
        Uint8 bWidth;       /* Uint8, but 0 = 256 ! */
        Uint8 bHeight;      /* Uint8, but 0 = 256 ! */
        Uint8 bColorCount;  /* Uint8, but 0 = 256 ! */
        /*
        Uint8 bReserved;
        Uint16 wPlanes;
        Uint16 wBitCount;
        Uint32 dwBytesInRes;
        */
        Uint32 dwImageOffset;
        int nWidth, nHeight, nColorCount;

        if (!SDL_ReadU8(src, &bWidth) ||
            !SDL_ReadU8(src, &bHeight) ||
            !SDL_ReadU8(src, &bColorCount) ||
            !SDL_ReadU8(src, NULL /* bReserved */) ||
            !SDL_ReadU16LE(src, NULL /* wPlanes */) ||
            !SDL_ReadU16LE(src, NULL /* wBitCount */) ||
            !SDL_ReadU32LE(src, NULL /* dwBytesInRes */) ||
            !SDL_ReadU32LE(src, &dwImageOffset)) {
            goto done;
        }

        if (bWidth) {
            nWidth = bWidth;
        } else {
            nWidth = 256;
        }
        if (bHeight) {
            nHeight = bHeight;
        } else {
            nHeight = 256;
        }
        if (bColorCount) {
            nColorCount = bColorCount;
        } else {
            nColorCount = 256;
        }

        //SDL_Log("%dx%d@%d - %08x\n", nWidth, nHeight, nColorCount, dwImageOffset);
        (void)nWidth;
        (void)nHeight;
        if (nColorCount > maxCol) {
            maxCol = nColorCount;
            icoOfs = dwImageOffset;
            //SDL_Log("marked\n");
        }
    }

    /* Advance to the DIB Data */
    if (SDL_RWseek(src, icoOfs, SDL_RW_SEEK_SET) < 0) {
        goto done;
    }

    /* Read the Win32 BITMAPINFOHEADER */
    if (!SDL_ReadU32LE(src, &biSize)) {
        goto done;
    }
    if (biSize == 40) {
        if (!SDL_ReadS32LE(src, &biWidth) ||
            !SDL_ReadS32LE(src, &biHeight) ||
            !SDL_ReadU16LE(src, NULL /* biPlanes */) ||
            !SDL_ReadU16LE(src, &biBitCount) ||
            !SDL_ReadU32LE(src, &biCompression) ||
            !SDL_ReadU32LE(src, NULL /* biSizeImage */) ||
            !SDL_ReadU32LE(src, NULL /* biXPelsPerMeter */) ||
            !SDL_ReadU32LE(src, NULL /* biYPelsPerMeter */) ||
            !SDL_ReadU32LE(src, &biClrUsed) ||
            !SDL_ReadU32LE(src, NULL /* biClrImportant */)) {
            goto done;
        }
    } else {
        IMG_SetError("Unsupported ICO bitmap format");
        goto done;
    }

    /* We don't support any BMP compression right now */
    switch (biCompression) {
    case BI_RGB:
        /* Default values for the BMP format */
        switch (biBitCount) {
        case 1:
        case 4:
            ExpandBMP = biBitCount;
            break;
        case 8:
            ExpandBMP = 8;
            break;
        case 24:
            ExpandBMP = 24;
            break;
        case 32:
            /*
            Rmask = 0x00FF0000;
            Gmask = 0x0000FF00;
            Bmask = 0x000000FF;
            */
            ExpandBMP = 0;
            break;
        default:
            IMG_SetError("ICO file with unsupported bit count");
            goto done;
        }
        break;
    default:
        IMG_SetError("Compressed ICO files not supported");
        goto done;
    }

    /* sanity check image size, so we don't overflow integers, etc. */
    if ((biWidth < 0) || (biWidth > 0xFFFFFF) ||
        (biHeight < 0) || (biHeight > 0xFFFFFF)) {
        IMG_SetError("Unsupported or invalid ICO dimensions");
        goto done;
    }

    /* Create a RGBA surface */
    biHeight = biHeight >> 1;
    //printf("%d x %d\n", biWidth, biHeight);
    surface = SDL_CreateSurface(biWidth, biHeight, SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL) {
        goto done;
    }

    /* Load the palette, if any */
    //printf("bc %d bused %d\n", biBitCount, biClrUsed);
    if (biBitCount <= 8) {
        if (biClrUsed == 0) {
            biClrUsed = 1 << biBitCount;
        }
        if (biClrUsed > SDL_arraysize(palette)) {
            IMG_SetError("Unsupported or incorrect biClrUsed field");
            goto done;
        }
        for (i = 0; i < (int) biClrUsed; ++i) {
            if (SDL_RWread(src, &palette[i], 4) != 4) {
                goto done;
            }
        }
    }

    /* Read the surface pixels.  Note that the bmp image is upside down */
    bits = (Uint8 *) surface->pixels + (surface->h * surface->pitch);
    switch (ExpandBMP) {
    case 1:
        bmpPitch = (biWidth + 7) >> 3;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    case 4:
        bmpPitch = (biWidth + 1) >> 1;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    case 8:
        bmpPitch = biWidth;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    case 24:
        bmpPitch = biWidth * 3;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    default:
        bmpPitch = biWidth * 4;
        pad = 0;
        break;
    }
    while (bits > (Uint8 *) surface->pixels) {
        bits -= surface->pitch;
        switch (ExpandBMP) {
        case 1:
        case 4:
        case 8:
            {
                Uint8 pixel = 0;
                int shift = (8 - ExpandBMP);
                for (i = 0; i < surface->w; ++i) {
                    if (i % (8 / ExpandBMP) == 0) {
                        if (SDL_RWread(src, &pixel, 1) != 1) {
                            goto done;
                        }
                    }
                    *((Uint32 *) bits + i) = (palette[pixel >> shift]);
                    pixel <<= ExpandBMP;
                }
            }
            break;
        case 24:
            {
                Uint32 pixel;
                Uint8 channel;
                for (i = 0; i < surface->w; ++i) {
                    pixel = 0;
                    for (j = 0; j < 3; ++j) {
                        /* Load each color channel into pixel */
                        if (SDL_RWread(src, &channel, 1) != 1) {
                            goto done;
                        }
                        pixel |= (channel << (j * 8));
                    }
                    *((Uint32 *) bits + i) = pixel;
                }
            }
            break;

        default:
            if (SDL_RWread(src, bits, surface->pitch) != (size_t)surface->pitch) {
                goto done;
            }
            break;
        }
        /* Skip padding bytes, ugh */
        if (pad) {
            Uint8 padbyte;
            for (i = 0; i < pad; ++i) {
                if (SDL_RWread(src, &padbyte, 1) != 1) {
                    goto done;
                }
            }
        }
    }
    /* Read the mask pixels.  Note that the bmp image is upside down */
    bits = (Uint8 *) surface->pixels + (surface->h * surface->pitch);
    ExpandBMP = 1;
    bmpPitch = (biWidth + 7) >> 3;
    pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
    while (bits > (Uint8 *) surface->pixels) {
        Uint8 pixel = 0;
        int shift = (8 - ExpandBMP);

        bits -= surface->pitch;
        for (i = 0; i < surface->w; ++i) {
            if (i % (8 / ExpandBMP) == 0) {
                if (SDL_RWread(src, &pixel, 1) != 1) {
                    goto done;
                }
            }
            *((Uint32 *) bits + i) |= ((pixel >> shift) ? 0 : 0xFF000000);
            pixel <<= ExpandBMP;
        }
        /* Skip padding bytes, ugh */
        if (pad) {
            Uint8 padbyte;
            for (i = 0; i < pad; ++i) {
                if (SDL_RWread(src, &padbyte, 1) != 1) {
                    goto done;
                }
            }
        }
    }

    was_error = SDL_FALSE;

done:
    if (freesrc && src) {
        SDL_RWclose(src);
    }
    if (was_error) {
        if (src && !freesrc) {
            SDL_RWseek(src, fp_offset, SDL_RW_SEEK_SET);
        }
        if (surface) {
            SDL_DestroySurface(surface);
        }
        surface = NULL;
    }
    return surface;
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
    return(LoadBMP_RW(src, SDL_FALSE));
}

/* Load a ICO type image from an SDL datasource */
SDL_Surface *IMG_LoadICO_RW(SDL_RWops *src)
{
    return(LoadICOCUR_RW(src, 1, SDL_FALSE));
}

/* Load a CUR type image from an SDL datasource */
SDL_Surface *IMG_LoadCUR_RW(SDL_RWops *src)
{
    return(LoadICOCUR_RW(src, 2, SDL_FALSE));
}

#else

#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
    return(0);
}

int IMG_isICO(SDL_RWops *src)
{
    return(0);
}

int IMG_isCUR(SDL_RWops *src)
{
    return(0);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
    return(NULL);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadCUR_RW(SDL_RWops *src)
{
    return(NULL);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadICO_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_BMP */

#endif /* !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND) */
