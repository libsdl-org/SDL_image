/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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

// We will have the saving BMP feature by default
#if !defined(SAVE_BMP)
#define SAVE_BMP 1
#endif

#ifndef LOAD_BMP
#undef SAVE_BMP
#define SAVE_BMP 0
#endif

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

#ifdef LOAD_BMP

#define RIFF_FOURCC(c0, c1, c2, c3)                 \
    ((Uint32)(Uint8)(c0) | ((Uint32)(Uint8)(c1) << 8) | \
     ((Uint32)(Uint8)(c2) << 16) | ((Uint32)(Uint8)(c3) << 24))

#define ICON_TYPE_ICO   1
#define ICON_TYPE_CUR   2

#pragma pack(push, 1)

typedef struct
{
    Uint8 bWidth;
    Uint8 bHeight;
    Uint8 bColorCount;
    Uint8 bReserved;
    Uint16 xHotspot;
    Uint16 yHotspot;
    Uint32 dwImageSize;
    Uint32 dwImageOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    Uint16 idReserved;
    Uint16 idType;
    Uint16 idCount;
} CURSORICONFILEDIR;

#pragma pack(pop)

typedef struct
{
    Sint64 offset;
    int width;
    int height;
    int ncolors;
    int hot_x;
    int hot_y;
    SDL_Surface *surface;
} IconEntry;

typedef struct
{
    int num_entries;
    int max_entries;
    IconEntry *entries;
} IconEntries;


/* See if an image is contained in a data source */
bool IMG_isBMP(SDL_IOStream *src)
{
    Sint64 start;
    bool is_BMP;
    char magic[2];

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_BMP = false;
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic)) {
        if (SDL_strncmp(magic, "BM", 2) == 0) {
            is_BMP = true;
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_BMP;
}

static bool IMG_isICOCUR(SDL_IOStream *src, int type)
{
    Sint64 start;
    bool is_ICOCUR;

    /* The Win32 ICO file header (12 bytes) */
    Uint16 bfReserved;
    Uint16 bfType;
    Uint16 bfCount;

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_ICOCUR = false;
    if (SDL_ReadU16LE(src, &bfReserved) &&
        SDL_ReadU16LE(src, &bfType) &&
        SDL_ReadU16LE(src, &bfCount) &&
        (bfReserved == 0) && (bfType == type) && (bfCount != 0)) {
        is_ICOCUR = true;
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);

    return is_ICOCUR;
}

bool IMG_isICO(SDL_IOStream *src)
{
    return IMG_isICOCUR(src, ICON_TYPE_ICO);
}

bool IMG_isCUR(SDL_IOStream *src)
{
    return IMG_isICOCUR(src, ICON_TYPE_CUR);
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

static SDL_Surface *LoadBMP_IO(SDL_IOStream *src, bool closeio)
{
    return SDL_LoadBMP_IO(src, closeio);
}

static bool GetBMPIconInfo(SDL_IOStream *src, int *width, int *height, int *ncolors)
{
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

    if (!SDL_ReadU32LE(src, &biSize) ||
        !SDL_ReadS32LE(src, &biWidth) ||
        !SDL_ReadS32LE(src, &biHeight) ||
        !SDL_ReadU16LE(src, NULL /* biPlanes */) ||
        !SDL_ReadU16LE(src, &biBitCount) ||
        !SDL_ReadU32LE(src, &biCompression) ||
        !SDL_ReadU32LE(src, NULL /* biSizeImage */) ||
        !SDL_ReadU32LE(src, NULL /* biXPelsPerMeter */) ||
        !SDL_ReadU32LE(src, NULL /* biYPelsPerMeter */) ||
        !SDL_ReadU32LE(src, &biClrUsed) ||
        !SDL_ReadU32LE(src, NULL /* biClrImportant */)) {
        return false;
    }

    biHeight >>= 1;

    if (biBitCount <= 8) {
        if (biClrUsed == 0) {
            biClrUsed = 1 << biBitCount;
        }
    } else {
        biClrUsed = 256;
    }

    *width = biWidth;
    *height = biHeight;
    *ncolors = biClrUsed;
    return true;
}

static SDL_Surface *GetBMPSurface(SDL_IOStream *src)
{
    bool was_error = true;
    int bmpPitch;
    int i, j, pad;
    SDL_Surface *surface = NULL;
    Uint8 *bits;
    int ExpandBMP;
    Uint32 palette[256];

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

    if (!SDL_ReadU32LE(src, &biSize) ||
        !SDL_ReadS32LE(src, &biWidth) ||
        !SDL_ReadS32LE(src, &biHeight) ||
        !SDL_ReadU16LE(src, NULL /* biPlanes */) ||
        !SDL_ReadU16LE(src, &biBitCount) ||
        !SDL_ReadU32LE(src, &biCompression) ||
        !SDL_ReadU32LE(src, NULL /* biSizeImage */) ||
        !SDL_ReadU32LE(src, NULL /* biXPelsPerMeter */) ||
        !SDL_ReadU32LE(src, NULL /* biYPelsPerMeter */) ||
        !SDL_ReadU32LE(src, &biClrUsed) ||
        !SDL_ReadU32LE(src, NULL /* biClrImportant */)) {
        return NULL;
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
            SDL_SetError("ICO file with unsupported bit count");
            goto done;
        }
        break;
    default:
        SDL_SetError("Compressed ICO files not supported");
        goto done;
    }

    /* sanity check image size, so we don't overflow integers, etc. */
    if ((biWidth < 0) || (biWidth > 0xFFFFFF) ||
        (biHeight < 0) || (biHeight > 0xFFFFFF)) {
        SDL_SetError("Unsupported or invalid ICO dimensions");
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
            SDL_SetError("Unsupported or incorrect biClrUsed field");
            goto done;
        }
        for (i = 0; i < (int) biClrUsed; ++i) {
            if (SDL_ReadIO(src, &palette[i], 4) != 4) {
                goto done;
            }

            /* Since biSize == 40, we know alpha is reserved and should be zero, meaning opaque */
            if ((palette[i] & 0xFF000000) == 0) {
                palette[i] |= 0xFF000000;
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
                Uint8 pixelvalue = 0;
                int shift = (8 - ExpandBMP);
                for (i = 0; i < surface->w; ++i) {
                    if (i % (8 / ExpandBMP) == 0) {
                        if (SDL_ReadIO(src, &pixelvalue, 1) != 1) {
                            goto done;
                        }
                    }
                    *((Uint32 *) bits + i) = (palette[pixelvalue >> shift]);
                    pixelvalue <<= ExpandBMP;
                }
            }
            break;
        case 24:
            {
                Uint32 pixelvalue;
                Uint8 channel;
                for (i = 0; i < surface->w; ++i) {
                    pixelvalue = 0xFF000000;
                    for (j = 0; j < 3; ++j) {
                        /* Load each color channel into pixel */
                        if (SDL_ReadIO(src, &channel, 1) != 1) {
                            goto done;
                        }
                        pixelvalue |= (channel << (j * 8));
                    }
                    *((Uint32 *) bits + i) = pixelvalue;
                }
            }
            break;

        default:
            if (SDL_ReadIO(src, bits, surface->pitch) != (size_t)surface->pitch) {
                goto done;
            }
            break;
        }
        /* Skip padding bytes, ugh */
        if (pad) {
            Uint8 padbyte;
            for (i = 0; i < pad; ++i) {
                if (SDL_ReadIO(src, &padbyte, 1) != 1) {
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
        Uint8 pixelvalue = 0;
        int shift = (8 - ExpandBMP);

        bits -= surface->pitch;
        for (i = 0; i < surface->w; ++i) {
            if (i % (8 / ExpandBMP) == 0) {
                if (SDL_ReadIO(src, &pixelvalue, 1) != 1) {
                    goto done;
                }
            }
            *((Uint32 *) bits + i) &= ((pixelvalue >> shift) ? 0 : 0xFFFFFFFF);
            pixelvalue <<= ExpandBMP;
        }
        /* Skip padding bytes, ugh */
        if (pad) {
            Uint8 padbyte;
            for (i = 0; i < pad; ++i) {
                if (SDL_ReadIO(src, &padbyte, 1) != 1) {
                    goto done;
                }
            }
        }
    }

    was_error = false;

done:
    if (was_error) {
        SDL_DestroySurface(surface);
        return NULL;
    }
    return surface;
}

static bool GetPNGIconInfo(SDL_IOStream *src, int *width, int *height, int *ncolors)
{
    Uint8 magic[16];
    Uint32 unWidth = 0;
    Uint32 unHeight = 0;

    if (SDL_ReadIO(src, magic, sizeof(magic)) != sizeof(magic) ||
        !SDL_ReadU32BE(src, &unWidth) || unWidth > SDL_MAX_SINT32 ||
        !SDL_ReadU32BE(src, &unHeight) || unHeight > SDL_MAX_SINT32) {
        return false;
    }

    *width = (int)unWidth;
    *height = (int)unHeight;
    *ncolors = 256;
    return true;
}

static bool GetIconInfo(SDL_IOStream *src, Sint64 offset, int *width, int *height, int *ncolors)
{
    bool result = false;
    Uint32 biSize;
    Sint64 start = SDL_TellIO(src);

    if (SDL_SeekIO(src, offset, SDL_IO_SEEK_SET) < 0) {
        return false;
    }

    if (!SDL_ReadU32LE(src, &biSize) ||
        SDL_SeekIO(src, -4, SDL_IO_SEEK_CUR) < 0) {
        goto done;
    }
    if (biSize == 40) {
        result = GetBMPIconInfo(src, width, height, ncolors);
    } else if (biSize == RIFF_FOURCC(0x89, 'P', 'N', 'G')) {
        result = GetPNGIconInfo(src, width, height, ncolors);
    } else {
        SDL_SetError("Unsupported ICO bitmap format");
    }

done:
    (void)SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return result;
}

static bool AddIconEntry(IconEntries *entries, Sint64 offset, int width, int height, int ncolors, int hot_x, int hot_y)
{
    int i;

    for (i = 0; i < entries->num_entries; ++i) {
        IconEntry *entry = &entries->entries[i];
        if (width == entry->width && height == entry->height) {
            if (ncolors > entry->ncolors) {
                // Replace the existing entry
                entry->offset = offset;
                entry->ncolors = ncolors;
                entry->hot_x = hot_x;
                entry->hot_y = hot_y;
            } else {
                // The existing entry is better
            }
            return true;
        }
    }

    if (entries->num_entries == entries->max_entries) {
        int max_entries = entries->max_entries + 8;
        IconEntry *new_entries = (IconEntry *)SDL_realloc(entries->entries, max_entries * sizeof(*entries->entries));
        if (!new_entries) {
            return false;
        }
        entries->entries = new_entries;
        entries->max_entries = max_entries;
    }

    IconEntry *entry = &entries->entries[entries->num_entries++];
    entry->offset = offset;
    entry->width = width;
    entry->height = height;
    entry->ncolors = ncolors;
    entry->hot_x = hot_x;
    entry->hot_y = hot_y;
    return true;
}

static SDL_Surface *GetIconSurface(SDL_IOStream *src, Sint64 offset, int type, int hot_x, int hot_y)
{
    SDL_Surface *surface = NULL;
    Uint32 biSize;

    if (SDL_SeekIO(src, offset, SDL_IO_SEEK_SET) < 0) {
        return false;
    }

    if (!SDL_ReadU32LE(src, &biSize) ||
        SDL_SeekIO(src, -4, SDL_IO_SEEK_CUR) < 0) {
        goto done;
    }
    if (biSize == 40) {
        surface = GetBMPSurface(src);
    } else if (biSize == RIFF_FOURCC(0x89, 'P', 'N', 'G')) {
        surface = IMG_LoadPNG_IO(src);
    } else {
        SDL_SetError("Unsupported ICO bitmap format");
    }

    if (type == ICON_TYPE_CUR) {
        SDL_PropertiesID props = SDL_GetSurfaceProperties(surface);
        SDL_SetNumberProperty(props, SDL_PROP_SURFACE_HOTSPOT_X_NUMBER, hot_x);
        SDL_SetNumberProperty(props, SDL_PROP_SURFACE_HOTSPOT_Y_NUMBER, hot_y);
    }

done:
    return surface;
}

static SDL_Surface *LoadICOCUR_IO(SDL_IOStream *src, int type, bool closeio)
{
    bool was_error = true;
    Sint64 start = 0;
    int i;
    IconEntries entries = { 0, 0, NULL };
    SDL_Surface *surface = NULL;

    /* The Win32 ICO file header (14 bytes) */
    Uint16 bfReserved;
    Uint16 bfType;
    Uint16 bfCount;

    /* Make sure we are passed a valid data source */
    if (src == NULL) {
        goto done;
    }

    /* Read in the ICO file header */
    start = SDL_TellIO(src);

    if (!SDL_ReadU16LE(src, &bfReserved) ||
        !SDL_ReadU16LE(src, &bfType) ||
        !SDL_ReadU16LE(src, &bfCount) ||
        (bfReserved != 0) || (bfType != type) || (bfCount == 0)) {
        SDL_SetError("File is not a Windows %s file", type == 1 ? "ICO" : "CUR");
        goto done;
    }

    /* Read the Win32 Icon Directory */
#ifdef DEBUG_ICONDIR
    SDL_Log("Icon directory: %d cursors", bfCount);
#endif
    for (i = 0; i < bfCount; ++i) {
        /* Icon Directory Entries */
        Uint8 bWidth;       /* Uint8, but 0 = 256 ! */
        Uint8 bHeight;      /* Uint8, but 0 = 256 ! */
        Uint8 bColorCount;  /* Uint8, but 0 = 256 ! */
        Uint8 bReserved;
        Uint16 wPlanes;
        Uint16 wBitCount;
        Uint32 dwBytesInRes;
        Uint32 dwImageOffset;
        int nWidth, nHeight, nColorCount;
        int nHotX, nHotY;

        if (!SDL_ReadU8(src, &bWidth) ||
            !SDL_ReadU8(src, &bHeight) ||
            !SDL_ReadU8(src, &bColorCount) ||
            !SDL_ReadU8(src, &bReserved) ||
            !SDL_ReadU16LE(src, &wPlanes) ||
            !SDL_ReadU16LE(src, &wBitCount) ||
            !SDL_ReadU32LE(src, &dwBytesInRes) ||
            !SDL_ReadU32LE(src, &dwImageOffset)) {
            goto done;
        }

        if (type == ICON_TYPE_CUR) {
            nHotX = wPlanes;
            nHotY = wBitCount;
        } else {
            nHotX = 0;
            nHotY = 0;
        }

        nWidth = nHeight = nColorCount = 0;
        if (!GetIconInfo(src, start + dwImageOffset, &nWidth, &nHeight, &nColorCount)) {
            continue;
        }

#ifdef DEBUG_ICONDIR
        SDL_Log("%dx%d@%d - %d, hot: %d,%d", nWidth, nHeight, nColorCount, (int)dwImageOffset, nHotX, nHotY);
#endif
        if (!AddIconEntry(&entries, start + dwImageOffset, nWidth, nHeight, nColorCount, nHotX, nHotY)) {
            continue;
        }
    }

    if (entries.num_entries == 0) {
        SDL_SetError("Couldn't find any valid icons");
        goto done;
    }

    /* Load the icon surfaces */
    for (i = 0; i < entries.num_entries; ++i) {
        IconEntry *entry = &entries.entries[i];
        entry->surface = GetIconSurface(src, entry->offset, type, entry->hot_x, entry->hot_y);
        if (!entry->surface) {
            goto done;
        }

        if (i == 0) {
            surface = entry->surface;
            ++surface->refcount;
        } else if (type == ICON_TYPE_CUR && entry->surface->w == 32 && entry->surface->h == 32) {
            // Windows defaults to 32x32 pixel cursors, adjusting for DPI scale, so use that as the primary surface
            --surface->refcount;
            surface = entry->surface;
            ++surface->refcount;
        }
    }
    for (i = 0; i < entries.num_entries; ++i) {
        IconEntry *entry = &entries.entries[i];
        if (entry->surface != surface) {
            SDL_AddSurfaceAlternateImage(surface, entry->surface);
        }
    }

    /* All done! */
    was_error = false;

done:
    if (closeio && src) {
        SDL_CloseIO(src);
    }

    for (i = 0; i < entries.num_entries; ++i) {
        IconEntry *entry = &entries.entries[i];
        SDL_DestroySurface(entry->surface);
    }
    SDL_free(entries.entries);

    if (was_error) {
        if (src && !closeio) {
            SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
        }
        if (surface) {
            SDL_DestroySurface(surface);
        }
        surface = NULL;
    }
    return surface;
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_IO(SDL_IOStream *src)
{
    return LoadBMP_IO(src, false);
}

/* Load a ICO type image from an SDL datasource */
SDL_Surface *IMG_LoadICO_IO(SDL_IOStream *src)
{
    return LoadICOCUR_IO(src, ICON_TYPE_ICO, false);
}

/* Load a CUR type image from an SDL datasource */
SDL_Surface *IMG_LoadCUR_IO(SDL_IOStream *src)
{
    return LoadICOCUR_IO(src, ICON_TYPE_CUR, false);
}

#else

/* See if an image is contained in a data source */
bool IMG_isBMP(SDL_IOStream *src)
{
    (void)src;
    return false;
}

bool IMG_isICO(SDL_IOStream *src)
{
    (void)src;
    return false;
}

bool IMG_isCUR(SDL_IOStream *src)
{
    (void)src;
    return false;
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_IO(SDL_IOStream *src)
{
    (void)src;
    SDL_SetError("SDL_image built without BMP support");
    return NULL;
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadCUR_IO(SDL_IOStream *src)
{
    (void)src;
    SDL_SetError("SDL_image built without BMP support");
    return NULL;
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadICO_IO(SDL_IOStream *src)
{
    (void)src;
    SDL_SetError("SDL_image built without BMP support");
    return NULL;
}

#endif /* LOAD_BMP */

#endif /* !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND) */


#if SAVE_BMP

bool IMG_SaveBMP_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    return SDL_SaveBMP_IO(surface, dst, closeio);
}

bool IMG_SaveBMP(SDL_Surface *surface, const char *file)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return IMG_SaveBMP_IO(surface, dst, true);
    } else {
        return false;
    }
}

static bool FillIconEntry(CURSORICONFILEDIRENTRY *entry, SDL_Surface *surface, int type, Uint32 dwImageSize, Uint32 dwImageOffset)
{
    int hot_x = 0, hot_y = 0;

    if (type == ICON_TYPE_CUR) {
        SDL_PropertiesID props = SDL_GetSurfaceProperties(surface);
        hot_x = (int)SDL_GetNumberProperty(props, SDL_PROP_SURFACE_HOTSPOT_X_NUMBER, hot_x);
        hot_y = (int)SDL_GetNumberProperty(props, SDL_PROP_SURFACE_HOTSPOT_Y_NUMBER, hot_y);
    }

    SDL_zerop(entry);
    entry->bWidth = surface->w < 256 ? surface->w : 0;  // 0 means a width of 256
    entry->bHeight = surface->h < 256 ? surface->h : 0; // 0 means a height of 256
    entry->xHotspot = hot_x;
    entry->yHotspot = hot_y;
    entry->dwImageSize = dwImageSize;
    entry->dwImageOffset = dwImageOffset;
    return true;
}

static bool WriteIconSurface(SDL_Surface *surface, SDL_IOStream *dst)
{
    // We'll use PNG format, for simplicity
    if (!IMG_SavePNG_IO(surface, dst, false)) {
        return false;
    }

    // Image data offsets must be WORD aligned
    Sint64 offset = SDL_TellIO(dst);
    if (offset & 1) {
        if (!SDL_WriteU8(dst, 0)) {
            return false;
        }
    }
    return true;
}

static bool SaveICOCUR(SDL_Surface *surface, SDL_IOStream *dst, bool closeio, int type)
{
    bool result = false;
    int count = 0;
    SDL_Surface **surfaces = NULL;
    Sint64 start = -1;

    if (!surface) {
        SDL_InvalidParamError("surface");
        goto done;
    }
    if (!dst) {
        SDL_InvalidParamError("dst");
        goto done;
    }

    start = SDL_TellIO(dst);
    if (start < 0) {
        // We need to be able to seek in the stream
        goto done;
    }

    surfaces = SDL_GetSurfaceImages(surface, &count);
    if (!surfaces) {
        goto done;
    }

    // Raymond Chen has more insight into this format at:
    // https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513
    CURSORICONFILEDIR dir;
    dir.idReserved = 0;
    dir.idType = type;
    dir.idCount = count;
    result = (SDL_WriteIO(dst, &dir, sizeof(dir)) == sizeof(dir));

    Uint32 entries_size = count * sizeof(CURSORICONFILEDIRENTRY);
    CURSORICONFILEDIRENTRY *entries = (CURSORICONFILEDIRENTRY *)SDL_malloc(entries_size);
    if (!entries) {
        result = false;
        goto done;
    }
    result &= (SDL_WriteIO(dst, entries, entries_size) == entries_size);

    Sint64 image_offset = SDL_TellIO(dst);
    for (int i = 0; i < count; ++i) {
        result &= WriteIconSurface(surfaces[i], dst);

        Sint64 next_offset = SDL_TellIO(dst);
        Uint32 dwImageSize = (Uint32)(next_offset - image_offset);
        Uint32 dwImageOffset = (Uint32)(image_offset - start);

        result &= FillIconEntry(&entries[i], surfaces[i], type, dwImageSize, dwImageOffset);

        image_offset = next_offset;
    }

    // Now that we have the icon entries filled out, rewrite them
    result &= (SDL_SeekIO(dst, start + sizeof(dir), SDL_IO_SEEK_SET) >= 0);
    result &= (SDL_WriteIO(dst, entries, entries_size) == entries_size);
    result &= (SDL_SeekIO(dst, image_offset, SDL_IO_SEEK_SET) >= 0);
    SDL_free(entries);

done:
    SDL_free(surfaces);

    if (!result && !closeio && start != -1) {
        SDL_SeekIO(dst, start, SDL_IO_SEEK_SET);
    }

    if (closeio) {
        SDL_CloseIO(dst);
    }

    return result;
}

bool IMG_SaveCUR_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    return SaveICOCUR(surface, dst, closeio, ICON_TYPE_CUR);
}

bool IMG_SaveCUR(SDL_Surface *surface, const char *file)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return SaveICOCUR(surface, dst, true, ICON_TYPE_CUR);
    } else {
        return false;
    }
}

bool IMG_SaveICO_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    return SaveICOCUR(surface, dst, closeio, ICON_TYPE_ICO);
}

bool IMG_SaveICO(SDL_Surface *surface, const char *file)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return SaveICOCUR(surface, dst, true, ICON_TYPE_ICO);
    } else {
        return false;
    }
}

#else // !SAVE_BMP

bool IMG_SaveBMP_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    return SDL_SetError("SDL_image built without BMP save support");
}

bool IMG_SaveBMP(SDL_Surface *surface, const char *file)
{
    (void)surface;
    (void)file;
    return SDL_SetError("SDL_image built without BMP save support");
}

bool IMG_SaveCUR_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    return SDL_SetError("SDL_image built without BMP save support");
}

bool IMG_SaveCUR(SDL_Surface *surface, const char *file)
{
    (void)surface;
    (void)file;
    return SDL_SetError("SDL_image built without BMP save support");
}

bool IMG_SaveICO_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    return SDL_SetError("SDL_image built without BMP save support");
}

bool IMG_SaveICO(SDL_Surface *surface, const char *file)
{
    (void)surface;
    (void)file;
    return SDL_SetError("SDL_image built without BMP save support");
}

#endif // SAVE_BMP

