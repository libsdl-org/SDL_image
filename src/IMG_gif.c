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

/* This is a GIF image file loading framework */

#include <SDL3_image/SDL_image.h>

#include "IMG_gif.h"
#include "IMG_anim_encoder.h"
#include "IMG_anim_decoder.h"

// We will have the saving GIF feature by default
#if !defined(SAVE_GIF)
#define SAVE_GIF 1
#endif

// By default, non-indexed surfaces will be converted to indexed pixels using octree quantization.
#if SAVE_GIF
    #ifndef SAVE_GIF_OCTREE
        #define SAVE_GIF_OCTREE 1
    #endif
#endif

#ifdef LOAD_GIF

/*        Some parts of the code have been adapted from XPaint:          */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993 David Koblas.                          | */
/* | Copyright 1996 Torsten Martinsen.                                 | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

/* Adapted for use in SDL by Sam Lantinga -- 7/20/98 */
/* Changes to work with SDL:

   Include SDL header file
   Use SDL_Surface rather than xpaint Image structure
   Define SDL versions of RWSetMsg(), ImageNewCmap() and ImageSetCmap()
*/

/* Has been overwhelmingly modified by Xen (@lordofxen) and Sam Lantinga (@slouken) as of 11/9/2025. */

#include <SDL3/SDL.h>

#define Image           SDL_Surface
#define RWSetMsg        SDL_SetError
#define ImageNewCmap(w, h, s)   SDL_CreateSurface(w, h, SDL_PIXELFORMAT_INDEX8)
#define ImageSetCmap(s, i, R, G, B) do { \
                palette->colors[i].r = R; \
                palette->colors[i].g = G; \
                palette->colors[i].b = B; \
            } while (0)
/* * * * * */

#define GIF_DISPOSE_NA                  0   /* No disposal specified */
#define GIF_DISPOSE_NONE                1   /* Do not dispose */
#define GIF_DISPOSE_RESTORE_BACKGROUND  2   /* Restore to background */
#define GIF_DISPOSE_RESTORE_PREVIOUS    3   /* Restore to previous */

#define MAXCOLORMAPSIZE     256

#define TRUE    1
#define FALSE   0

#define CM_RED      0
#define CM_GREEN    1
#define CM_BLUE     2

#define MAX_LWZ_BITS        12

#define INTERLACE       0x40
#define LOCALCOLORMAP   0x80
#define BitSet(byte, bit)   (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (SDL_ReadIO(file, buffer, len) == len)

#define LM_to_uint(a,b)         (((b)<<8)|(a))

typedef struct {
    struct {
        unsigned int Width;
        unsigned int Height;
        unsigned char ColorMap[3][MAXCOLORMAPSIZE];
        unsigned int BitPixel;
        unsigned int ColorResolution;
        unsigned int Background;
        unsigned int AspectRatio;
        int GrayScale;
    } GifScreen;

    struct {
        int transparent;
        int delayTime;
        int inputFlag;
        int disposal;
    } Gif89;

    unsigned char buf[280];
    int curbit, lastbit, done, last_byte;

    int fresh;
    int code_size, set_code_size;
    int max_code, max_code_size;
    int firstcode, oldcode;
    int clear_code, end_code;
    int table[2][(1 << MAX_LWZ_BITS)];
    int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;

    int ZeroDataBlock;
} State_t;

typedef struct
{
    Image *image;
    int x, y;
    int disposal;
    int delay;
} Frame_t;

typedef struct
{
    int count;
    Frame_t *frames;
} Anim_t;

static int ReadColorMap(SDL_IOStream * src, int number,
			unsigned char buffer[3][MAXCOLORMAPSIZE], int *flag);
static int DoExtension(SDL_IOStream * src, int label, State_t * state);
static int GetDataBlock(SDL_IOStream * src, unsigned char *buf, State_t * state);
static int GetCode(SDL_IOStream * src, int code_size, int flag, State_t * state);
static int LWZReadByte(SDL_IOStream * src, int flag, int input_code_size, State_t * state);
static Image *ReadImage(SDL_IOStream *src, int len, int height, int cmapSize,
          unsigned char cmap[3][MAXCOLORMAPSIZE],
          int gray, int interlace, int ignore, State_t *state);

static int
ReadColorMap(SDL_IOStream *src, int number,
             unsigned char buffer[3][MAXCOLORMAPSIZE], int *gray)
{
    int i;
    unsigned char rgb[3];
    int flag;

    flag = 1;

    for (i = 0; i < number; ++i) {
        if (!ReadOK(src, rgb, sizeof(rgb))) {
            RWSetMsg("bad colormap");
            return 1;
        }
        buffer[CM_RED][i] = rgb[0];
        buffer[CM_GREEN][i] = rgb[1];
        buffer[CM_BLUE][i] = rgb[2];
        flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

#if 0
    if (flag)
        *gray = (number == 2) ? PBM_TYPE : PGM_TYPE;
    else
        *gray = PPM_TYPE;
#else
    (void) flag;
    *gray = 0;
#endif

    return FALSE;
}

static int
DoExtension(SDL_IOStream *src, int label, State_t * state)
{
    unsigned char buf[256];

    switch (label) {
    case 0x01:          /* Plain Text Extension */
        break;
    case 0xff:          /* Application Extension */
        break;
    case 0xfe:          /* Comment Extension */
        while (GetDataBlock(src, buf, state) > 0)
            ;
        return FALSE;
    case 0xf9:          /* Graphic Control Extension */
        (void) GetDataBlock(src, buf, state);
        state->Gif89.disposal = (buf[0] >> 2) & 0x7;
        state->Gif89.inputFlag = (buf[0] >> 1) & 0x1;
        state->Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
        if ((buf[0] & 0x1) != 0)
            state->Gif89.transparent = buf[3];

        while (GetDataBlock(src, buf, state) > 0)
            ;
        return FALSE;
    default:
        break;
    }

    while (GetDataBlock(src, buf, state) > 0)
        ;

    return FALSE;
}

static int
GetDataBlock(SDL_IOStream *src, unsigned char *buf, State_t * state)
{
    unsigned char count;

    if (!ReadOK(src, &count, 1)) {
        /* pm_message("error in getting DataBlock size" ); */
        return -1;
    }
    state->ZeroDataBlock = count == 0;

    if ((count != 0) && (!ReadOK(src, buf, count))) {
        /* pm_message("error in reading DataBlock" ); */
        return -1;
    }
    return count;
}

static int
GetCode(SDL_IOStream *src, int code_size, int flag, State_t * state)
{
    int i, j, ret;
    unsigned char count;

    if (flag) {
        state->curbit = 0;
        state->lastbit = 0;
        state->done = FALSE;
        return 0;
    }
    if ((state->curbit + code_size) >= state->lastbit) {
        if (state->done) {
            if (state->curbit >= state->lastbit)
                RWSetMsg("ran off the end of my bits");
            return -1;
        }
        state->buf[0] = state->buf[state->last_byte - 2];
        state->buf[1] = state->buf[state->last_byte - 1];

        if ((ret = GetDataBlock(src, &state->buf[2], state)) > 0)
            count = (unsigned char) ret;
        else {
            count = 0;
            state->done = TRUE;
        }

        state->last_byte = 2 + count;
        state->curbit = (state->curbit - state->lastbit) + 16;
        state->lastbit = (2 + count) * 8;
    }
    ret = 0;
    for (i = state->curbit, j = 0; j < code_size; ++i, ++j)
        ret |= ((state->buf[i / 8] & (1 << (i % 8))) != 0) << j;

    state->curbit += code_size;

    return ret;
}

static int
LWZReadByte(SDL_IOStream *src, int flag, int input_code_size, State_t * state)
{
    int i, code, incode;

    /* Fixed buffer overflow found by Michael Skladnikiewicz */
    if (input_code_size > MAX_LWZ_BITS)
        return -1;

    if (flag) {
        state->set_code_size = input_code_size;
        state->code_size = state->set_code_size + 1;
        state->clear_code = 1 << state->set_code_size;
        state->end_code = state->clear_code + 1;
        state->max_code_size = 2 * state->clear_code;
        state->max_code = state->clear_code + 2;

        GetCode(src, 0, TRUE, state);

        state->fresh = TRUE;

        for (i = 0; i < state->clear_code; ++i) {
            state->table[0][i] = 0;
            state->table[1][i] = i;
        }
        state->table[1][0] = 0;
        for (; i < (1 << MAX_LWZ_BITS); ++i)
            state->table[0][i] = 0;

        state->sp = state->stack;

        return 0;
    } else if (state->fresh) {
        state->fresh = FALSE;
        do {
            state->firstcode = state->oldcode = GetCode(src, state->code_size, FALSE, state);
        } while (state->firstcode == state->clear_code);
        return state->firstcode;
    }
    if (state->sp > state->stack)
        return *--state->sp;

    while ((code = GetCode(src, state->code_size, FALSE, state)) >= 0) {
        if (code == state->clear_code) {
            for (i = 0; i < state->clear_code; ++i) {
                state->table[0][i] = 0;
                state->table[1][i] = i;
            }
            for (; i < (1 << MAX_LWZ_BITS); ++i)
                state->table[0][i] = state->table[1][i] = 0;
            state->code_size = state->set_code_size + 1;
            state->max_code_size = 2 * state->clear_code;
            state->max_code = state->clear_code + 2;
            state->sp = state->stack;
            state->firstcode = state->oldcode = GetCode(src, state->code_size, FALSE, state);
            return state->firstcode;
        } else if (code == state->end_code) {
            int count;
            unsigned char buf[260];

            if (state->ZeroDataBlock)
                return -2;

            while ((count = GetDataBlock(src, buf, state)) > 0)
                ;

            if (count != 0) {
            /*
             * pm_message("missing EOD in data stream (common occurrence)");
             */
            }
            return -2;
        }
        incode = code;

        if (code >= state->max_code) {
            *state->sp++ = state->firstcode;
            code = state->oldcode;
        }
        while (code >= state->clear_code) {
            /* Guard against buffer overruns */
            if (code < 0 || code >= (1 << MAX_LWZ_BITS)) {
                RWSetMsg("invalid LWZ data");
                return -3;
            }
            *state->sp++ = state->table[1][code];
            if (code == state->table[0][code]) {
                RWSetMsg("circular table entry BIG ERROR");
                return -3;
            }
            code = state->table[0][code];
        }

        /* Guard against buffer overruns */
        if (code < 0 || code >= (1 << MAX_LWZ_BITS)) {
            RWSetMsg("invalid LWZ data");
            return -4;
        }
        *state->sp++ = state->firstcode = state->table[1][code];

        if ((code = state->max_code) < (1 << MAX_LWZ_BITS)) {
            state->table[0][code] = state->oldcode;
            state->table[1][code] = state->firstcode;
            ++state->max_code;
            if ((state->max_code >= state->max_code_size) &&
                (state->max_code_size < (1 << MAX_LWZ_BITS))) {
                state->max_code_size *= 2;
                ++state->code_size;
            }
        }
        state->oldcode = incode;

        if (state->sp > state->stack)
            return *--state->sp;
    }
    return code;
}

static Image *
ReadImage(SDL_IOStream * src, int len, int height, int cmapSize,
          unsigned char cmap[3][MAXCOLORMAPSIZE],
          int gray, int interlace, int ignore, State_t * state)
{
    Image *image;
    SDL_Palette *palette;
    unsigned char c;
    int i, v;
    int xpos = 0, ypos = 0, pass = 0;

    (void) gray; /* unused */

    /*
    **  Initialize the compression routines
     */
    if (!ReadOK(src, &c, 1)) {
        RWSetMsg("EOF / read error on image data");
        return NULL;
    }
    if (LWZReadByte(src, TRUE, c, state) < 0) {
        RWSetMsg("error reading image");
        return NULL;
    }
    /*
    **  If this is an "uninteresting picture" ignore it.
     */
    if (ignore) {
        while (LWZReadByte(src, FALSE, c, state) >= 0)
            ;
        return NULL;
    }
    image = ImageNewCmap(len, height, cmapSize);
    if (!image) {
        return NULL;
    }

    palette = SDL_CreateSurfacePalette(image);
    if (!palette) {
        return NULL;
    }
    if (cmapSize > palette->ncolors) {
        cmapSize = palette->ncolors;
    }
    palette->ncolors = cmapSize;
    for (i = 0; i < cmapSize; i++) {
        ImageSetCmap(image, i, cmap[CM_RED][i], cmap[CM_GREEN][i], cmap[CM_BLUE][i]);
    }

    if (state->Gif89.transparent >= 0 && state->Gif89.transparent < cmapSize) {
        SDL_SetSurfaceColorKey(image, true, state->Gif89.transparent);
    }

    while ((v = LWZReadByte(src, FALSE, c, state)) >= 0) {
        ((Uint8 *)image->pixels)[xpos + ypos * image->pitch] = (Uint8)v;
        ++xpos;
        if (xpos == len) {
            xpos = 0;
            if (interlace) {
                switch (pass) {
                case 0:
                case 1:
                    ypos += 8;
                    break;
                case 2:
                    ypos += 4;
                    break;
                case 3:
                    ypos += 2;
                    break;
                }

                if (ypos >= height) {
                    ++pass;
                    switch (pass) {
                    case 1:
                        ypos = 4;
                        break;
                    case 2:
                        ypos = 2;
                        break;
                    case 3:
                        ypos = 1;
                        break;
                    default:
                        goto fini;
                    }
                }
            } else {
                ++ypos;
            }
        }
        if (ypos >= height)
            break;
    }

  fini:

    return image;
}

struct IMG_AnimationDecoderContext
{
    State_t state;                /* GIF decoding state */

    unsigned char buf[256];      /* Buffer for reading chunks */
    char version[4];             /* GIF version */

    int width;                   /* Width of the GIF */
    int height;                  /* Height of the GIF */

    SDL_Surface *canvas;         /* Canvas for compositing frames */
    SDL_Surface *prev_canvas;    /* Previous canvas for DISPOSE_PREVIOUS */

    int frame_count;             /* Total number of frames seen */
    int current_frame;           /* Current frame index */

    bool got_header;             /* Whether we've read the GIF header */
    bool got_eof;                /* Whether we've reached the end of the GIF */

    /* Current frame info */
    int current_disposal;        /* Disposal method for current frame */
    int current_delay;           /* Delay time for current frame */
    int transparent_index;       /* Transparent color index for current frame */

    /* Global color map */
    unsigned char global_colormap[3][MAXCOLORMAPSIZE];
    int global_colormap_size;
    bool has_global_colormap;
    bool global_grayscale;

    /* Frame info */
    Uint64 last_duration;        /* The duration of the previous frame */
    int last_disposal;           /* Disposal method from previous frame */
    int restore_frame;           /* Frame to restore when using DISPOSE_PREVIOUS */

    bool ignore_props;
};

static bool IMG_AnimationDecoderGetGIFHeader(IMG_AnimationDecoder *decoder, char**comment, int *loopCount)
{
    if (comment) {
        *comment = NULL;
    }

    if (loopCount) {
        *loopCount = 0;
    }

    IMG_AnimationDecoderContext *ctx = decoder->ctx;
    SDL_IOStream *src = decoder->src;
    if (!ctx->got_header) {
        if (!ReadOK(src, ctx->buf, 6)) {
            return SDL_SetError("Error reading GIF magic number");
        }

        if (SDL_strncmp((char *)ctx->buf, "GIF", 3) != 0) {
            return SDL_SetError("Not a GIF file");
        }

        SDL_memcpy(ctx->version, (char *)ctx->buf + 3, 3);
        ctx->version[3] = '\0';

        if ((SDL_strcmp(ctx->version, "87a") != 0) && (SDL_strcmp(ctx->version, "89a") != 0)) {
            return SDL_SetError("Bad version number, not '87a' or '89a'");
        }

        if (!ReadOK(src, ctx->buf, 7)) {
            return SDL_SetError("Failed to read screen descriptor");
        }

        ctx->width = LM_to_uint(ctx->buf[0], ctx->buf[1]);
        ctx->height = LM_to_uint(ctx->buf[2], ctx->buf[3]);
        ctx->state.GifScreen.Width = ctx->width;
        ctx->state.GifScreen.Height = ctx->height;
        ctx->state.GifScreen.BitPixel = 2 << (ctx->buf[4] & 0x07);
        ctx->state.GifScreen.ColorResolution = (((ctx->buf[4] & 0x70) >> 3) + 1);
        ctx->state.GifScreen.Background = ctx->buf[5];
        ctx->state.GifScreen.AspectRatio = ctx->buf[6];

        ctx->has_global_colormap = BitSet(ctx->buf[4], LOCALCOLORMAP);

        if (ctx->has_global_colormap) {
            ctx->global_colormap_size = ctx->state.GifScreen.BitPixel;
            int g = ctx->global_grayscale ? 1 : 0;
            if (ReadColorMap(src, ctx->global_colormap_size, ctx->global_colormap, &g)) {
                return SDL_SetError("Error reading global colormap");
            }
            SDL_memcpy(ctx->state.GifScreen.ColorMap, ctx->global_colormap, sizeof(ctx->global_colormap));
            ctx->state.GifScreen.GrayScale = ctx->global_grayscale;
        }

        if (!ctx->ignore_props) {
            Uint64 stream_pos = SDL_TellIO(src);
            bool processing_extensions = true;

            while (processing_extensions) {
                uint8_t block_type;
                if (!ReadOK(src, &block_type, 1)) {
                    return SDL_SetError("Error reading GIF block type");
                }

                switch (block_type) {
                case 0x21: // Extension Introducer
                {
                    uint8_t extension_label;
                    if (!ReadOK(src, &extension_label, 1)) {
                        return SDL_SetError("Error reading GIF extension label");
                    }

                    switch (extension_label) {
                    case 0xFF: // Application Extension
                    {
                        // Read the application block size first (should be 11 for "NETSCAPE2.0")
                        Uint8 app_block_size;
                        if (!ReadOK(src, &app_block_size, 1)) {
                            return SDL_SetError("Error reading application extension block size");
                        }

                        Uint8 app_data[256];
                        if (!ReadOK(src, app_data, app_block_size)) {
                            return SDL_SetError("Error reading GIF application extension block");
                        }

                        // Check for NETSCAPE2.0 extension (loop count)
                        if (app_block_size == 11 &&
                            SDL_strncmp((char *)app_data, "NETSCAPE2.0", 11) == 0) {

                            Uint8 sub_block_size;
                            if (!ReadOK(src, &sub_block_size, 1)) {
                                return SDL_SetError("Error reading Netscape sub-block size");
                            }
                            if (sub_block_size == 3) {
                                Uint8 sub_block_data[3];
                                if (!ReadOK(src, sub_block_data, 3)) {
                                    return SDL_SetError("Error reading Netscape sub-block data");
                                }
                                if (sub_block_data[0] == 0x01 && loopCount) {
                                    *loopCount = LM_to_uint(sub_block_data[1], sub_block_data[2]);
                                }
                                // Terminator
                                if (!ReadOK(src, &sub_block_size, 1) || sub_block_size != 0x00) {
                                    return SDL_SetError("Netscape extension block not terminated correctly");
                                }
                            } else {
                                // Skip unexpected sub-block sizes
                                SDL_SeekIO(src, sub_block_size, SDL_IO_SEEK_CUR);
                                Uint8 terminator;
                                if (!ReadOK(src, &terminator, 1) || terminator != 0) {
                                    return SDL_SetError("Extension block not terminated correctly");
                                }
                            }
                        } else {
                            // Skip all sub-blocks for non-Netscape extensions
                            Uint8 sub_block_size;
                            while (ReadOK(src, &sub_block_size, 1) && sub_block_size > 0) {
                                SDL_SeekIO(src, sub_block_size, SDL_IO_SEEK_CUR);
                            }
                        }
                    } break;

                    case 0xFE: // Comment Extension
                    {
                        if (comment) {
                            char *c = NULL;
                            Uint8 sub_block_size;
                            size_t current_len = 0;
                            while (ReadOK(src, &sub_block_size, 1) && sub_block_size > 0) {
                                size_t new_len = current_len + sub_block_size;
                                char *temp_comment = SDL_realloc(c, new_len + 1);
                                if (!temp_comment) {
                                    return SDL_SetError("Failed to allocate memory for GIF comment");
                                }
                                c = temp_comment;
                                if (!ReadOK(src, c + current_len, sub_block_size)) {
                                    return SDL_SetError("Error reading GIF comment data");
                                }
                                current_len = new_len;
                            }
                            if (c && current_len > 0) {
                                c[current_len] = '\0';
                            }

                            *comment = c;
                        }
                    } break;

                    default: // Other extensions
                    {
                        Uint8 sub_block_size;
                        while (ReadOK(src, &sub_block_size, 1) && sub_block_size > 0) {
                            SDL_SeekIO(src, sub_block_size, SDL_IO_SEEK_CUR);
                        }
                    } break;
                    }
                } break;

                case 0x2C: // Image Descriptor
                    processing_extensions = false;
                    break;

                case 0x3B: // Trailer
                    return SDL_SetError("GIF file contains no images");

                default: // Unknown block type
                    return SDL_SetError("Unknown GIF block type: 0x%02X", block_type);
                }
            }
            SDL_SeekIO(src, stream_pos, SDL_IO_SEEK_SET);
        }

        if (!ctx->canvas) {
            ctx->canvas = SDL_CreateSurface(ctx->width, ctx->height, SDL_PIXELFORMAT_RGBA32);
            if (!ctx->canvas) {
                return SDL_SetError("Failed to create canvas surface");
            }

            if (!SDL_FillSurfaceRect(ctx->canvas, NULL, 0)) {
                SDL_DestroySurface(ctx->canvas);
                ctx->canvas = NULL;
                return SDL_SetError("Failed to fill canvas surface with transparent color");
            }
        }

        if (!ctx->prev_canvas) {
            ctx->prev_canvas = SDL_CreateSurface(ctx->width, ctx->height, SDL_PIXELFORMAT_RGBA32);
            if (!ctx->prev_canvas) {
                return SDL_SetError("Failed to create previous canvas surface");
            }

            if (!SDL_FillSurfaceRect(ctx->prev_canvas, NULL, 0)) {
                SDL_DestroySurface(ctx->prev_canvas);
                ctx->prev_canvas = NULL;
                return SDL_SetError("Failed to fill previous canvas surface with transparent color");
            }
        }

        ctx->got_header = true;
    }
    return true;
}

static bool IMG_AnimationDecoderReset_Internal(IMG_AnimationDecoder *decoder)
{
    IMG_AnimationDecoderContext* ctx = decoder->ctx;
    if (SDL_SeekIO(decoder->src, decoder->start, SDL_IO_SEEK_SET) != decoder->start) {
        return SDL_SetError("Failed to seek to beginning of GIF file");
    }

    SDL_memset(&ctx->state, 0, sizeof(ctx->state));
    ctx->state.Gif89.transparent = -1;
    ctx->state.Gif89.delayTime = -1;
    ctx->state.Gif89.inputFlag = -1;
    ctx->state.Gif89.disposal = GIF_DISPOSE_NA;

    ctx->current_frame = 0;
    ctx->current_disposal = GIF_DISPOSE_NA;
    ctx->current_delay = 100;
    ctx->transparent_index = -1;
    ctx->got_header = false;
    ctx->got_eof = false;
    ctx->last_disposal = GIF_DISPOSE_NONE;
    ctx->restore_frame = 0;

    // We don't care about metadata when resetting to re-read.
    ctx->ignore_props = true;

    if (ctx->canvas) {
        SDL_FillSurfaceRect(ctx->canvas, NULL, 0);
    }

    if (ctx->prev_canvas) {
        SDL_FillSurfaceRect(ctx->prev_canvas, NULL, 0);
    }

    return IMG_AnimationDecoderGetGIFHeader(decoder, NULL, NULL);
}

static bool IMG_AnimationDecoderGetNextFrame_Internal(IMG_AnimationDecoder *decoder, SDL_Surface **frame, Uint64 *duration)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;
    SDL_IOStream *src = decoder->src;
    unsigned char c;
    int framesLoaded = 0;

    if (ctx->got_eof) {
        decoder->status = IMG_DECODER_STATUS_COMPLETE;
        return false;
    }

    if (!IMG_AnimationDecoderGetGIFHeader(decoder, NULL, NULL)) {
        return false;
    }

    int framesToLoad = 1;
    SDL_Surface *retval = NULL;
    while (framesLoaded < framesToLoad) {
        if (!ReadOK(src, &c, 1)) {
            ctx->got_eof = true;
            break;
        }

        if (c == ';') {
            ctx->got_eof = true;
            break;
        }

        if (c == '!') {
            if (!ReadOK(src, &c, 1)) {
                ctx->got_eof = true;
                break;
            }
            DoExtension(src, c, &ctx->state);
            continue;
        }

        if (c != ',') {
            continue;
        }

        if (!ReadOK(src, ctx->buf, 9)) {
            ctx->got_eof = true;
            break;
        }

        int useGlobalColormap = !BitSet(ctx->buf[8], LOCALCOLORMAP);
        int bitPixel = 1 << ((ctx->buf[8] & 0x07) + 1);
        int left = LM_to_uint(ctx->buf[0], ctx->buf[1]);
        int top = LM_to_uint(ctx->buf[2], ctx->buf[3]);
        int width = LM_to_uint(ctx->buf[4], ctx->buf[5]);
        int height = LM_to_uint(ctx->buf[6], ctx->buf[7]);

        unsigned char localColorMap[3][MAXCOLORMAPSIZE];
        int grayScale = 0;

        if (!useGlobalColormap) {
            if (ReadColorMap(src, bitPixel, localColorMap, &grayScale)) {
                ctx->got_eof = true;
                break;
            }
        }

        switch (ctx->last_disposal) {
        case GIF_DISPOSE_NONE:
            /* Leave canvas as is */
            break;

        case GIF_DISPOSE_RESTORE_BACKGROUND:
        {
            SDL_Rect rect = { 0, 0, ctx->width, ctx->height };
            if (!SDL_FillSurfaceRect(ctx->canvas, &rect, 0)) {
                return SDL_SetError("Failed to fill canvas with background color");
            }
        } break;

        case GIF_DISPOSE_RESTORE_PREVIOUS:
            /* Restore canvas to previous state */
            if (ctx->prev_canvas) {
                if (!SDL_BlitSurface(ctx->prev_canvas, NULL, ctx->canvas, NULL)) {
                    return SDL_SetError("Failed to restore previous canvas");
                }
            }
            break;

        default:
            /* Default is to do nothing */
            break;
        }

        /* If current disposal method is RESTORE_PREVIOUS, save current canvas */
        if (ctx->state.Gif89.disposal == GIF_DISPOSE_RESTORE_PREVIOUS) {
            if (!SDL_BlitSurface(ctx->canvas, NULL, ctx->prev_canvas, NULL)) {
                return SDL_SetError("Failed to save current canvas for restoration");
            }
        }
        Image *image;
        if (!useGlobalColormap) {
            image = ReadImage(src, width, height, bitPixel, localColorMap, grayScale,
                              BitSet(ctx->buf[8], INTERLACE), 0, &ctx->state);
        } else {
            image = ReadImage(src, width, height, ctx->state.GifScreen.BitPixel,
                              ctx->state.GifScreen.ColorMap, ctx->state.GifScreen.GrayScale,
                              BitSet(ctx->buf[8], INTERLACE), 0, &ctx->state);
        }

      if (!image) {
          // Incorrect animation is harder to detect than a direct failure,
          // so it's better to fail than try to animate a GIF without a,
          // full set of frames it has in the file.

          // Only set the error if ReadImage did not do it.
          if (SDL_GetError()[0] == '\0') {
              return SDL_SetError("Failed to decode frame.");
          }

          return false;
      }

        /* Composite the frame onto the canvas */
        SDL_Rect dest = { left, top, width, height };
        if (!SDL_BlitSurface(image, NULL, ctx->canvas, &dest)) {
            SDL_DestroySurface(image);
            return SDL_SetError("Failed to blit frame onto canvas");
        }

        /* Store the frame in the output array */
        retval = SDL_DuplicateSurface(ctx->canvas);
        if (!retval) {
            SDL_DestroySurface(image);
            return SDL_SetError("Failed to duplicate frame surface");
        }

        if (ctx->state.Gif89.delayTime < 0 && ctx->last_duration) {
            *duration = ctx->last_duration;
        } else if (ctx->state.Gif89.delayTime < 2) {
            /* Default animation delay, matching browser and Qt */
            *duration = IMG_GetDecoderDuration(decoder, 10, 100);
        } else {
            *duration = IMG_GetDecoderDuration(decoder, ctx->state.Gif89.delayTime, 100);
        }
        ctx->last_duration = *duration;

        ctx->last_disposal = ctx->state.Gif89.disposal;

        SDL_DestroySurface(image);

        ctx->state.Gif89.transparent = -1;
        ctx->state.Gif89.delayTime = -1;
        ctx->state.Gif89.inputFlag = -1;
        ctx->state.Gif89.disposal = GIF_DISPOSE_NA;

        framesLoaded++;
        ctx->current_frame++;
        ctx->frame_count++;
    }

    if (framesLoaded == 0) {
        if (ctx->got_eof) {
            decoder->status = IMG_DECODER_STATUS_COMPLETE;
            return false;
        }
        return SDL_SetError("Failed to load any frames");
    }

    *frame = retval;
    return true;
}

static bool IMG_AnimationDecoderClose_Internal(IMG_AnimationDecoder *decoder)
{
    IMG_AnimationDecoderContext* ctx = decoder->ctx;
    if (ctx->canvas) {
        SDL_DestroySurface(ctx->canvas);
    }

    if (ctx->prev_canvas) {
        SDL_DestroySurface(ctx->prev_canvas);
    }

    SDL_free(ctx);
    decoder->ctx = NULL;

    return true;
}

bool IMG_CreateGIFAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    IMG_AnimationDecoderContext *ctx = (IMG_AnimationDecoderContext*)SDL_calloc(1, sizeof(IMG_AnimationDecoderContext));
    if (!ctx) {
        return SDL_SetError("Out of memory for GIF decoder context");
    }

    ctx->state.Gif89.transparent = -1;
    ctx->state.Gif89.delayTime = -1;
    ctx->state.Gif89.inputFlag = -1;
    ctx->state.Gif89.disposal = GIF_DISPOSE_NA;

    ctx->transparent_index = -1;
    ctx->got_header = false;
    ctx->got_eof = false;
    ctx->current_frame = 0;
    ctx->frame_count = 0;
    ctx->current_delay = 100;
    ctx->current_disposal = GIF_DISPOSE_NA;
    ctx->last_disposal = GIF_DISPOSE_NONE;
    ctx->restore_frame = 0;

    decoder->ctx = ctx;
    decoder->Reset = IMG_AnimationDecoderReset_Internal;
    decoder->GetNextFrame = IMG_AnimationDecoderGetNextFrame_Internal;
    decoder->Close = IMG_AnimationDecoderClose_Internal;

    char *comment = NULL;
    int loop_count = 0;
    if (!IMG_AnimationDecoderGetGIFHeader(decoder, &comment, &loop_count)) {
        return false;
    }

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    ctx->ignore_props = ignoreProps;
    if (!ignoreProps) {
        // Set well-defined properties.
        SDL_SetNumberProperty(decoder->props, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, (Sint64)loop_count);

        // Get other well-defined properties and set them in our props.
        if (comment) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_DESCRIPTION_STRING, comment);
        }
    }
    SDL_free(comment);

    return true;
}

#else

bool IMG_CreateGIFAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    (void)decoder;
    (void)props;
    SDL_SetError("SDL_image built without GIF support");
    return false;
}

#endif /* LOAD_GIF */

#if !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND)

#ifdef LOAD_GIF

/* See if an image is contained in a data source */
bool IMG_isGIF(SDL_IOStream *src)
{
    Sint64 start;
    bool is_GIF;
    char magic[6];

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_GIF = false;
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic) ) {
        if ( (SDL_strncmp(magic, "GIF", 3) == 0) &&
             ((SDL_memcmp(magic + 3, "87a", 3) == 0) ||
              (SDL_memcmp(magic + 3, "89a", 3) == 0)) ) {
            is_GIF = true;
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_GIF;
}

/* Load a GIF type image from an SDL datasource */
SDL_Surface *IMG_LoadGIF_IO(SDL_IOStream *src)
{
    IMG_AnimationDecoder *decoder = IMG_CreateAnimationDecoder_IO(src, false, "gif");
    if (!decoder) {
        return NULL;
    }

    Uint64 pts = 0;
    SDL_Surface *frame = NULL;
    IMG_GetAnimationDecoderFrame(decoder, &frame, &pts);
    IMG_CloseAnimationDecoder(decoder);

    return frame;
}

#else

/* See if an image is contained in a data source */
bool IMG_isGIF(SDL_IOStream *src)
{
    (void)src;
    return false;
}

/* Load a GIF type image from an SDL datasource */
SDL_Surface *IMG_LoadGIF_IO(SDL_IOStream *src)
{
    (void)src;
    SDL_SetError("SDL_image built without GIF support");
    return NULL;
}

#endif /* LOAD_GIF */

#endif /* !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND) */

#if SAVE_GIF
#pragma pack(push,1)
// GIF Header (6 bytes) + Logical Screen Descriptor (7 bytes)
// Total 13 bytes
typedef struct
{
    char signature[3]; // "GIF"
    char version[3];   // "89a"
    uint16_t screenWidth;
    uint16_t screenHeight;
    uint8_t globalColorTableInfo; // Packed field
    uint8_t backgroundColorIndex;
    uint8_t pixelAspectRatio;
} GifHeaderAndLSD;

// Graphics Control Extension (8 bytes)
typedef struct
{
    uint8_t blockSeparator; // 0x21
    uint8_t label;          // 0xF9
    uint8_t blockSize;      // 0x04
    uint8_t packedFields;   // Packed field (disposal method, user input, transparency)
    uint16_t delayTime;     // Delay in 1/100ths of a second
    uint8_t transparentColorIndex;
    uint8_t blockTerminator; // 0x00
} GraphicsControlExtension;

// Image Descriptor (10 bytes)
typedef struct
{
    uint8_t imageSeparator; // 0x2C
    uint16_t imageLeft;
    uint16_t imageTop;
    uint16_t imageWidth;
    uint16_t imageHeight;
    uint8_t packedFields; // Packed field (local color table, interlace, sort, reserved)
} ImageDescriptor;

// Netscape Application Extension (19 bytes + 1 byte terminator)
typedef struct
{
    uint8_t blockSeparator;                // 0x21
    uint8_t label;                         // 0xFF
    uint8_t blockSize;                     // 0x0B
    char applicationIdentifier[8];         // "NETSCAPE"
    char applicationAuthenticationCode[3]; // "2.0"
    uint8_t subBlockSize;                  // 0x03
    uint8_t loopCountExtension;            // 0x01
    uint16_t loopCount;                    // 0 for infinite loop
    uint8_t blockTerminator;               // 0x00
} NetscapeExtension;
#pragma pack(pop)


// Function to write a byte to the SDL_IOStream
static bool writeByte(SDL_IOStream *io, uint8_t byte)
{
    return SDL_WriteIO(io, &byte, 1) == 1;
}

// Function to write a 16-bit word (little-endian) to the SDL_IOStream
static bool writeWord(SDL_IOStream *io, uint16_t word)
{
    uint8_t bytes[2];
    bytes[0] = word & 0xFF;        // Low byte
    bytes[1] = (word >> 8) & 0xFF; // High byte
    return SDL_WriteIO(io, bytes, 2) == 2;
}

struct IMG_AnimationEncoderContext
{
    uint16_t width;
    uint16_t height;
    uint8_t globalColorTable[256][3];
    uint16_t numGlobalColors;
    int transparentColorIndex;
    bool firstFrame;
    uint8_t colorMapLUT[32][32][32];
    bool lut_initialized;
    bool use_lut;
    SDL_PropertiesID metadata;
};

#define LZW_MAX_CODES 4096
#define LZW_MAX_BITS  12

typedef struct
{
    int prefix;
    uint8_t suffix;
} LZWEntry;

typedef struct
{
    LZWEntry entries[LZW_MAX_CODES];
    int nextCode;
    int initialCodeSize;
    int currentCodeSize;
    int clearCode;
    int eoiCode;
} LZWDictionary;

typedef struct
{
    uint8_t *buffer;
    size_t currentByte;
    int currentBit;
    size_t allocatedSize;
} BitStream;

static bool BitStream_Init(BitStream *bs)
{
    bs->buffer = (uint8_t *)SDL_calloc(1, bs->allocatedSize);
    if (!bs->buffer) {
        return false;
    }
    bs->currentByte = 0;
    bs->currentBit = 0;

    return true;
}

static bool BitStream_WriteCode(BitStream *bs, int code, int numBits)
{
    if (!bs->buffer) {
        SDL_SetError("BitStream buffer not initialized.");
        return false;
    }

    size_t requiredBytes = bs->currentByte + ((bs->currentBit + numBits + 7) / 8) + 1;
    if (requiredBytes >= bs->allocatedSize) {
        // TODO: For now we basically allocate 2x the current size, but we could implement a more sophisticated resizing strategy for large images.

        size_t newSize = bs->allocatedSize * 2;
        if (newSize < requiredBytes) {
            newSize = requiredBytes + 256;
        }
        uint8_t *newBuffer = (uint8_t *)SDL_realloc(bs->buffer, newSize);
        if (!newBuffer) {
            SDL_SetError("Failed to reallocate BitStream buffer.");
            return false;
        }

        SDL_memset(newBuffer + bs->allocatedSize, 0, newSize - bs->allocatedSize);
        bs->buffer = newBuffer;
        bs->allocatedSize = newSize;
    }

    for (int i = 0; i < numBits; i++) {
        if (code & (1 << i)) {
            bs->buffer[bs->currentByte] |= (1 << bs->currentBit);
        }

        bs->currentBit++;
        if (bs->currentBit == 8) {
            bs->currentBit = 0;
            bs->currentByte++;
            bs->buffer[bs->currentByte] = 0;
        }
    }

    return true;
}

static size_t BitStream_Flush(BitStream *bs)
{
    if (bs->currentBit > 0) {
        bs->currentByte++;
    }
    return bs->currentByte;
}

static void BitStream_Free(BitStream *bs)
{
    if (bs->buffer) {
        SDL_free(bs->buffer);
        bs->buffer = NULL;
    }
}

static int lzwCompress(const uint8_t *indexedPixels, uint16_t width, uint16_t height,
                       uint8_t minCodeSize, uint8_t **compressedData, size_t *compressedSize,
                       int quality)
{
    if (!indexedPixels || !compressedData || !compressedSize) {
        SDL_SetError("Invalid NULL pointer passed to lzwCompress");
        return -1;
    }

    // Validate minCodeSize is in reasonable range
    if (minCodeSize < 2 || minCodeSize >= LZW_MAX_BITS) {
        SDL_SetError("Invalid minCodeSize %d (must be between 2 and %d)",
                     minCodeSize, LZW_MAX_BITS - 1);
        return -1;
    }

    const int HASH_TABLE_SIZE = 32771;

    // Please do not lower the threshold for <50, lowering threshold too much will result in corrupted frames after lzw compression.
    const int qualityThreshold = (quality < 50) ? 3072 : (quality < 75) ? 3584
                                                                        : 4096;

    BitStream bs;
    bs.allocatedSize = width * height;
    if (bs.allocatedSize < 4096) {
        bs.allocatedSize = 4096;
    }

    if (!BitStream_Init(&bs)) {
        SDL_SetError("Failed to allocate bitstream buffer");
        return -1;
    }

    typedef struct
    {
        int prefix;
        uint8_t suffix;
        int next_in_chain;
    } DictEntry;

    DictEntry *dict = (DictEntry *)SDL_calloc(LZW_MAX_CODES, sizeof(DictEntry));
    if (!dict) {
        BitStream_Free(&bs);
        SDL_SetError("Failed to allocate LZW dictionary");
        return -1;
    }

    int *hashTable = (int *)SDL_calloc(HASH_TABLE_SIZE, sizeof(int));
    if (!hashTable) {
        SDL_free(dict);
        BitStream_Free(&bs);
        SDL_SetError("Failed to allocate hash table");
        return -1;
    }

    const int clearCode = 1 << minCodeSize;
    const int eoiCode = clearCode + 1;
    int nextCode;
    int curCodeSize;
    const size_t pixelCount = (size_t)width * height;

#define HASH_FUNC(prefix, suffix)                                                       \
    (((((unsigned int)(prefix) << 13) ^ ((unsigned int)(suffix) << 5)) * 2654435761u) ^ \
     (((unsigned int)(prefix) >> 7) * 16777619u)) %                                     \
        HASH_TABLE_SIZE

    SDL_memset(hashTable, 0xFF, HASH_TABLE_SIZE * sizeof(int));
    nextCode = eoiCode + 1;
    curCodeSize = minCodeSize + 1;

    if (!BitStream_WriteCode(&bs, clearCode, curCodeSize)) {
        SDL_free(dict);
        SDL_free(hashTable);
        BitStream_Free(&bs);
        return -1;
    }

    if (pixelCount > 0) {
        int curString = indexedPixels[0];

        for (size_t i = 1; i < pixelCount; i++) {
            const uint8_t pixel = indexedPixels[i];
            int hashKey = HASH_FUNC(curString, pixel);
            int code = hashTable[hashKey];

            bool found = false;
            while (code != -1) {
                if (dict[code].prefix == curString && dict[code].suffix == pixel) {
                    curString = code;
                    found = true;
                    break;
                }
                code = dict[code].next_in_chain;
            }

            if (!found) {
                if (!BitStream_WriteCode(&bs, curString, curCodeSize)) {
                    SDL_free(dict);
                    SDL_free(hashTable);
                    BitStream_Free(&bs);
                    return -1;
                }

                if (nextCode < qualityThreshold) {
                    dict[nextCode].prefix = curString;
                    dict[nextCode].suffix = pixel;
                    dict[nextCode].next_in_chain = hashTable[hashKey];
                    hashTable[hashKey] = nextCode;

                    if (nextCode == (1 << curCodeSize) && curCodeSize < LZW_MAX_BITS) {
                        curCodeSize++;
                    }
                    nextCode++;
                } else {
                    // Dictionary reset is controlled by quality threshold
                    if (!BitStream_WriteCode(&bs, clearCode, curCodeSize)) {
                        SDL_free(dict);
                        SDL_free(hashTable);
                        BitStream_Free(&bs);
                        return -1;
                    }
                    curCodeSize = minCodeSize + 1;
                    nextCode = eoiCode + 1;
                    SDL_memset(hashTable, 0xFF, HASH_TABLE_SIZE * sizeof(int));
                }
                curString = pixel;
            }
        }
        if (!BitStream_WriteCode(&bs, curString, curCodeSize)) {
            SDL_free(dict);
            SDL_free(hashTable);
            BitStream_Free(&bs);
            return -1;
        }
    }

    if (!BitStream_WriteCode(&bs, eoiCode, curCodeSize)) {
        SDL_free(dict);
        SDL_free(hashTable);
        BitStream_Free(&bs);
        return -1;
    }

    *compressedData = bs.buffer;
    *compressedSize = BitStream_Flush(&bs);

    SDL_free(dict);
    SDL_free(hashTable);

    return 0;

#undef HASH_FUNC
}

#if SAVE_GIF_OCTREE
#ifndef OCTREE_MAX_LEVELS
    #define OCTREE_MAX_LEVELS 8
#endif

typedef struct OctreeNode
{
    struct OctreeNode *children[8];
    struct OctreeNode *parent;
    uint32_t pixelCount;
    uint64_t rSum, gSum, bSum;
    int paletteIndex;
    int level;
    bool isLeaf;
    bool inLeafList;
    struct OctreeNode *next_leaf;
    struct OctreeNode *prev_leaf;
} OctreeNode;

typedef struct
{
    OctreeNode *root;
    OctreeNode *head_leaf_list[OCTREE_MAX_LEVELS];
    OctreeNode *tail_leaf_list[OCTREE_MAX_LEVELS];
    uint32_t leafCount;
    uint32_t maxColors;
    uint8_t *palette;
    uint32_t paletteSize;
} Octree;

static OctreeNode *OctreeNode_Create(int level, OctreeNode *parent)
{
    OctreeNode *node = (OctreeNode *)SDL_calloc(1, sizeof(OctreeNode));
    if (node) {
        node->level = level;
        node->parent = parent;
        node->isLeaf = true;
        node->inLeafList = false;
        node->paletteIndex = -1;
        node->next_leaf = NULL;
        node->prev_leaf = NULL;
    }
    return node;
}

static void OctreeNode_Free(OctreeNode *node)
{
    if (!node)
        return;

    for (int i = 0; i < 8; ++i) {
        if (node->children[i]) {
            OctreeNode_Free(node->children[i]);
        }
    }
    SDL_free(node);
}

static bool Octree_Init(Octree *octree, uint32_t maxColors)
{
    octree->root = OctreeNode_Create(0, NULL);
    if (!octree->root) {
        SDL_SetError("Failed to create Octree root node.");
        return false;
    }
    octree->leafCount = 0;
    octree->maxColors = maxColors;
    octree->palette = NULL;
    octree->paletteSize = 0;
    for (int i = 0; i < OCTREE_MAX_LEVELS; ++i) {
        octree->head_leaf_list[i] = NULL;
        octree->tail_leaf_list[i] = NULL;
    }
    return true;
}

static void Octree_AddLeaf(Octree *octree, OctreeNode *node)
{
    if (node->inLeafList) {
        return;
    }

    int level = node->level;
    node->next_leaf = NULL;
    node->prev_leaf = octree->tail_leaf_list[level];
    if (octree->tail_leaf_list[level]) {
        octree->tail_leaf_list[level]->next_leaf = node;
    } else {
        octree->head_leaf_list[level] = node;
    }
    octree->tail_leaf_list[level] = node;
    node->inLeafList = true;
    octree->leafCount++;
}

static void Octree_RemoveLeaf(Octree *octree, OctreeNode *node)
{
    if (!node->inLeafList) {
        return;
    }

    int level = node->level;
    if (node->prev_leaf) {
        node->prev_leaf->next_leaf = node->next_leaf;
    } else {
        octree->head_leaf_list[level] = node->next_leaf;
    }
    if (node->next_leaf) {
        node->next_leaf->prev_leaf = node->prev_leaf;
    } else {
        octree->tail_leaf_list[level] = node->prev_leaf;
    }
    node->next_leaf = NULL;
    node->prev_leaf = NULL;
    node->inLeafList = false;
    octree->leafCount--;
}

static void Octree_InsertColor(Octree *octree, OctreeNode *node, uint8_t r, uint8_t g, uint8_t b, int level)
{
    if (node->isLeaf && level < OCTREE_MAX_LEVELS - 1) {
        if (node->pixelCount > 0) {
            Octree_RemoveLeaf(octree, node);
        }
        node->isLeaf = false;
    }

    node->pixelCount++;
    node->rSum += r;
    node->gSum += g;
    node->bSum += b;

    if (level == OCTREE_MAX_LEVELS - 1) {
        Octree_AddLeaf(octree, node);
        return;
    }

    int index = 0;
    if (r & (1 << (7 - level)))
        index |= 4;
    if (g & (1 << (7 - level)))
        index |= 2;
    if (b & (1 << (7 - level)))
        index |= 1;

    if (!node->children[index]) {
        node->children[index] = OctreeNode_Create(level + 1, node);
        if (!node->children[index]) {
            // If allocation fails, revert to leaf node
            node->isLeaf = true;
            Octree_AddLeaf(octree, node);
            return;
        }
    }

    Octree_InsertColor(octree, node->children[index], r, g, b, level + 1);
}

static bool OctreeNode_AllChildrenAreLeaves(OctreeNode *node)
{
    if (node->isLeaf)
        return false;

    bool hasAnyChildren = false;
    for (int i = 0; i < 8; ++i) {
        if (node->children[i]) {
            hasAnyChildren = true;
            if (!node->children[i]->isLeaf) {
                return false;
            }
        }
    }
    return hasAnyChildren;
}

static bool Octree_Reduce(Octree *octree)
{
    OctreeNode *nodeToCollapse = NULL;
    uint32_t minPixelCount = (uint32_t)-1;

    for (int level = OCTREE_MAX_LEVELS - 2; level >= 0; --level) {
        OctreeNode *currentLeaf = octree->head_leaf_list[level + 1];
        while (currentLeaf) {
            OctreeNode *parentNode = currentLeaf->parent;
            if (parentNode && parentNode->level == level && !parentNode->isLeaf) {
                if (OctreeNode_AllChildrenAreLeaves(parentNode)) {
                    uint32_t currentPixelCount = 0;
                    for (int i = 0; i < 8; ++i) {
                        if (parentNode->children[i]) {
                            currentPixelCount += parentNode->children[i]->pixelCount;
                        }
                    }

                    if (currentPixelCount < minPixelCount) {
                        minPixelCount = currentPixelCount;
                        nodeToCollapse = parentNode;
                    }
                }
            }
            currentLeaf = currentLeaf->next_leaf;
        }
    }

    if (!nodeToCollapse) {
        SDL_SetError("Octree_Reduce: No suitable node found to collapse to reduce leafCount.");
        return false;
    }

    nodeToCollapse->rSum = 0;
    nodeToCollapse->gSum = 0;
    nodeToCollapse->bSum = 0;
    nodeToCollapse->pixelCount = 0;

    for (int i = 0; i < 8; ++i) {
        if (nodeToCollapse->children[i]) {
            OctreeNode *child = nodeToCollapse->children[i];
            Octree_RemoveLeaf(octree, child);

            nodeToCollapse->rSum += child->rSum;
            nodeToCollapse->gSum += child->gSum;
            nodeToCollapse->bSum += child->bSum;
            nodeToCollapse->pixelCount += child->pixelCount;

            SDL_free(child);
            nodeToCollapse->children[i] = NULL;
        }
    }

    nodeToCollapse->isLeaf = true;
    Octree_AddLeaf(octree, nodeToCollapse);

    return true;
}

static bool Octree_BuildPalette(Octree *octree, OctreeNode *node, int *paletteIndexCounter)
{
    if (!node)
        return false;

    if (node->isLeaf) {
        if (node->pixelCount > 0) {
            octree->palette[*paletteIndexCounter * 3 + 0] = (uint8_t)(node->rSum / node->pixelCount);
            octree->palette[*paletteIndexCounter * 3 + 1] = (uint8_t)(node->gSum / node->pixelCount);
            octree->palette[*paletteIndexCounter * 3 + 2] = (uint8_t)(node->bSum / node->pixelCount);
        } else {
            octree->palette[*paletteIndexCounter * 3 + 0] = 0;
            octree->palette[*paletteIndexCounter * 3 + 1] = 0;
            octree->palette[*paletteIndexCounter * 3 + 2] = 0;
        }
        node->paletteIndex = *paletteIndexCounter;
        (*paletteIndexCounter)++;
        octree->paletteSize++;
        return true;
    }

    for (int i = 0; i < 8; ++i) {
        if (node->children[i]) {
            if (!Octree_BuildPalette(octree, node->children[i], paletteIndexCounter)) {
                return false;
            }
        }
    }

    return true;
}

static int Octree_GetPaletteIndex(OctreeNode *node, uint8_t r, uint8_t g, uint8_t b, int level)
{
    if (node->isLeaf) {
        return node->paletteIndex;
    }

    int index = 0;
    if (r & (1 << (7 - level)))
        index |= 4;
    if (g & (1 << (7 - level)))
        index |= 2;
    if (b & (1 << (7 - level)))
        index |= 1;

    if (node->children[index]) {
        return Octree_GetPaletteIndex(node->children[index], r, g, b, level + 1);
    } else {
        int bestDist = 196608; // 3 * 256 * 256, larger than any possible distance
        int bestIndex = -1;

        // Find the first valid child to initialize the search
        for (int i = 0; i < 8; ++i) {
            if (node->children[i] && node->children[i]->pixelCount > 0) {
                int dr = r - (uint8_t)(node->children[i]->rSum / node->children[i]->pixelCount);
                int dg = g - (uint8_t)(node->children[i]->gSum / node->children[i]->pixelCount);
                int db = b - (uint8_t)(node->children[i]->bSum / node->children[i]->pixelCount);
                bestDist = dr * dr + dg * dg + db * db;
                bestIndex = node->children[i]->paletteIndex;
                break;
            }
        }

        // Now search the rest of the children for a better match
        for (int i = 0; i < 8; ++i) {
            if (node->children[i] && node->children[i]->pixelCount > 0) {
                int dr = r - (uint8_t)(node->children[i]->rSum / node->children[i]->pixelCount);
                int dg = g - (uint8_t)(node->children[i]->gSum / node->children[i]->pixelCount);
                int db = b - (uint8_t)(node->children[i]->bSum / node->children[i]->pixelCount);
                int dist = dr * dr + dg * dg + db * db;

                if (dist < bestDist) {
                    bestDist = dist;
                    bestIndex = node->children[i]->paletteIndex;
                }
            }
        }

        return (bestIndex >= 0) ? bestIndex : 0;
    }
}

static void Octree_Free(Octree *octree)
{
    if (octree->root) {
        OctreeNode_Free(octree->root);
        octree->root = NULL;
    }
    if (octree->palette) {
        SDL_free(octree->palette);
        octree->palette = NULL;
    }
    octree->paletteSize = 0;
    octree->leafCount = 0;
    for (int i = 0; i < OCTREE_MAX_LEVELS; ++i) {
        octree->head_leaf_list[i] = NULL;
        octree->tail_leaf_list[i] = NULL;
    }
}
#endif /* SAVE_GIF_OCTREE */

static int count_set_bits(uint32_t n)
{
    int count = 0;
    while (n > 0) {
        n &= (n - 1);
        count++;
    }
    return count;
}

static void buildColorMapLUT(uint8_t lut[32][32][32], uint8_t palette[][3], uint16_t numColors, bool hasTransparency)
{
    const int color_count = hasTransparency ? numColors - 1 : numColors;

    for (int r = 0; r < 32; ++r) {
        for (int g = 0; g < 32; ++g) {
            for (int b = 0; b < 32; ++b) {
                // Map the 5-bit color back to 8-bit to find the closest match
                const uint8_t r8 = (r << 3) | (r >> 2);
                const uint8_t g8 = (g << 3) | (g >> 2);
                const uint8_t b8 = (b << 3) | (b >> 2);

                int best_match = 0;
                int min_dist = 195076; // 3 * 255*255 + 1

                for (uint16_t i = 0; i < color_count; ++i) {
                    const int dr = r8 - palette[i][0];
                    const int dg = g8 - palette[i][1];
                    const int db = b8 - palette[i][2];
                    const int dist = dr * dr + dg * dg + db * db;
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_match = i;
                        if (min_dist == 0) {
                            break; // Exact match
                        }
                    }
                }
                lut[r][g][b] = (uint8_t)best_match;
            }
        }
    }
}

static int mapSurfaceToExistingPalette(SDL_Surface *psurf, uint8_t lut[32][32][32], uint8_t *indexedPixels, int transparentIndex)
{
    SDL_Surface *surf = psurf;
    bool surface_converted = false;
    bool surface_locked = false;
    Uint32 colorKey = 0;
    bool hasTransparency = SDL_SurfaceHasColorKey(surf);
    if (hasTransparency) {
        SDL_GetSurfaceColorKey(surf, &colorKey);
    }

    if (surf->format != SDL_PIXELFORMAT_RGBA32 && surf->format != SDL_PIXELFORMAT_ARGB32) {
        surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
        if (!surf) {
            SDL_SetError("Failed to convert surface to RGBA32 for palette mapping.");
            return -1;
        }
        surface_converted = true;
    }

    const SDL_PixelFormatDetails *pixelFormatDetails = SDL_GetPixelFormatDetails(surf->format);
    if (!pixelFormatDetails) {
        SDL_SetError("Failed to get pixel format details for surface.");
        if (surface_converted) {
            SDL_DestroySurface(surf);
        }
        return -1;
    }

    if (SDL_MUSTLOCK(surf)) {
        if (!SDL_LockSurface(surf)) {
            if (surface_converted) {
                SDL_DestroySurface(surf);
            }
            SDL_SetError("Failed to lock surface for palette mapping.");
            return -1;
        }
        surface_locked = true;
    }

    const int r_bpp = count_set_bits(pixelFormatDetails->Rmask);
    const int g_bpp = count_set_bits(pixelFormatDetails->Gmask);
    const int b_bpp = count_set_bits(pixelFormatDetails->Bmask);

    for (int y = 0; y < surf->h; ++y) {
        uint32_t *src_row = (uint32_t *)((uint8_t *)surf->pixels + y * surf->pitch);
        uint8_t *dst_row = indexedPixels + y * surf->w;
        for (int x = 0; x < surf->w; ++x) {
            uint32_t pixel = src_row[x];
            if (hasTransparency && pixel == colorKey) {
                dst_row[x] = (uint8_t)transparentIndex;
                continue;
            }

            uint8_t r = (uint8_t)(((pixel & pixelFormatDetails->Rmask) >> pixelFormatDetails->Rshift) << (8 - r_bpp));
            uint8_t g = (uint8_t)(((pixel & pixelFormatDetails->Gmask) >> pixelFormatDetails->Gshift) << (8 - g_bpp));
            uint8_t b = (uint8_t)(((pixel & pixelFormatDetails->Bmask) >> pixelFormatDetails->Bshift) << (8 - b_bpp));

            // Use the pre-calculated LUT for an O(1) color lookup.
            dst_row[x] = lut[r >> 3][g >> 3][b >> 3];
        }
    }

    if (surface_locked) {
        SDL_UnlockSurface(surf);
    }
    if (surface_converted) {
        SDL_DestroySurface(surf);
    }

    return 0;
}

static int quantizeSurfaceToIndexedPixels(SDL_Surface *psurf, uint8_t palette[][3], uint16_t numPaletteColors, uint8_t *indexedPixels, int transparentIndex)
{
    if (!psurf || !palette || !indexedPixels || numPaletteColors == 0 || (numPaletteColors & (numPaletteColors - 1)) != 0) {
        SDL_SetError("Invalid arguments for quantizeSurfaceToIndexedPixels: numPaletteColors must be a power of 2.");
        return -1;
    }

    bool surface_converted = false;
    bool surface_locked = false;
    bool hasTransparency = false;
    SDL_Surface *surf = psurf;

#if !SAVE_GIF_OCTREE
    // Without octree, we rely on SDL to convert non-indexed surfaces to indexed format.
    // Since SDL does a fast conversion, this may result in lower quality than octree quantization.
    if (surf->format != SDL_PIXELFORMAT_INDEX8) {
        surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_INDEX8);
        if (!surf) {
            SDL_SetError("Failed to convert surface to INDEX8 format.");
            return -1;
        }
        surface_converted = true;
    }

    Uint32 colorKey = 0;
    if (transparentIndex >= 0 && SDL_SurfaceHasColorKey(surf)) {
        SDL_GetSurfaceColorKey(surf, &colorKey);
        hasTransparency = true;
    }

    SDL_Palette *surface_palette = SDL_GetSurfacePalette(surf);
    if (!surface_palette) {
        SDL_SetError("INDEX8 surface has no palette.");
        if (surface_converted) {
            SDL_DestroySurface(surf);
        }
        return -1;
    }

    if (surface_palette->ncolors > (hasTransparency ? numPaletteColors - 1 : numPaletteColors)) {
        SDL_SetError("INDEX8 surface palette has too many colors for target.");
        if (surface_converted) {
            SDL_DestroySurface(surf);
        }
        return -1;
    }

    for (int i = 0; i < surface_palette->ncolors; ++i) {
        palette[i][0] = surface_palette->colors[i].r;
        palette[i][1] = surface_palette->colors[i].g;
        palette[i][2] = surface_palette->colors[i].b;
    }

    for (int i = surface_palette->ncolors; i < numPaletteColors; ++i) {
        palette[i][0] = 0;
        palette[i][1] = 0;
        palette[i][2] = 0;
    }

    if (hasTransparency) {
        palette[transparentIndex][0] = 0;
        palette[transparentIndex][1] = 0;
        palette[transparentIndex][2] = 0;
    }

    if (SDL_MUSTLOCK(surf)) {
        if (!SDL_LockSurface(surf)) {
            SDL_SetError("Failed to lock INDEX8 surface");
            if (surface_converted) {
                SDL_DestroySurface(surf);
            }
            return -1;
        }

        surface_locked = true;
    }

    for (int y = 0; y < surf->h; ++y) {
        uint8_t *src_row = (uint8_t *)surf->pixels + y * surf->pitch;
        uint8_t *dst_row = indexedPixels + y * surf->w;

        if (hasTransparency) {
            for (int x = 0; x < surf->w; ++x) {
                uint8_t index = src_row[x];
                uint32_t pixel = index;

                if (pixel == colorKey) {
                    dst_row[x] = transparentIndex;
                } else {
                    dst_row[x] = index;
                }
            }
        } else {
            SDL_memcpy(dst_row, src_row, surf->w);
        }
    }

    if (surface_converted) {
        SDL_DestroySurface(surf);
    } else {
        if (surface_locked) {
            SDL_UnlockSurface(surf);
        }
    }

    return 0;
#else

    const SDL_PixelFormatDetails *pixelFormatDetails = SDL_GetPixelFormatDetails(surf->format);
    if (!pixelFormatDetails) {
        SDL_SetError("Failed to get pixel format details for original surface.");
        return -1;
    }

    Uint32 colorKey = 0;
    if (transparentIndex >= 0) {
        if (SDL_SurfaceHasColorKey(psurf)) {
            SDL_GetSurfaceColorKey(psurf, &colorKey);
        }
        hasTransparency = true;
    }

    if (surf->format == SDL_PIXELFORMAT_INDEX8) {
        SDL_Palette *surface_palette = SDL_GetSurfacePalette(surf);
        if (!surface_palette) {
            SDL_SetError("INDEX8 surface has no palette.");
            return -1;
        }

        if (surface_palette->ncolors > (hasTransparency ? numPaletteColors - 1 : numPaletteColors)) {
            SDL_SetError("INDEX8 surface palette has too many colors for target.");
            return -1;
        }

        for (int i = 0; i < surface_palette->ncolors; ++i) {
            palette[i][0] = surface_palette->colors[i].r;
            palette[i][1] = surface_palette->colors[i].g;
            palette[i][2] = surface_palette->colors[i].b;
        }

        for (int i = surface_palette->ncolors; i < numPaletteColors; ++i) {
            palette[i][0] = 0;
            palette[i][1] = 0;
            palette[i][2] = 0;
        }

        if (hasTransparency) {
            palette[transparentIndex][0] = 0;
            palette[transparentIndex][1] = 0;
            palette[transparentIndex][2] = 0;
        }

        if (SDL_MUSTLOCK(surf)) {
            if (!SDL_LockSurface(surf)) {
                SDL_SetError("Failed to lock INDEX8 surface");
                return -1;
            }

            surface_locked = true;
        }

        for (int y = 0; y < surf->h; ++y) {
            uint8_t *src_row = (uint8_t *)surf->pixels + y * surf->pitch;
            uint8_t *dst_row = indexedPixels + y * surf->w;

            if (hasTransparency) {
                for (int x = 0; x < surf->w; ++x) {
                    uint8_t index = src_row[x];
                    uint32_t pixel = index;

                    if (pixel == colorKey) {
                        dst_row[x] = transparentIndex;
                    } else {
                        dst_row[x] = index;
                    }
                }
            } else {
                SDL_memcpy(dst_row, src_row, surf->w);
            }
        }

        if (surface_locked) {
            SDL_UnlockSurface(surf);
        }

        return 0;
    } else {
        if (surf->format != SDL_PIXELFORMAT_RGBA32 && surf->format != SDL_PIXELFORMAT_ARGB32) {
            surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
            if (!surf) {
                SDL_SetError("Failed to convert surface to RGBA32 format.");
                return -1;
            }

            surface_converted = true;
            pixelFormatDetails = SDL_GetPixelFormatDetails(surf->format);
            if (!pixelFormatDetails) {
                SDL_SetError("Failed to get pixel format details for surface.");
                SDL_DestroySurface(surf);
                return -1;
            }
        }

        if (SDL_MUSTLOCK(surf)) {
            if (!SDL_LockSurface(surf)) {
                SDL_SetError("Failed to lock RGBA32 surface");
                if (surface_converted) {
                    SDL_DestroySurface(surf);
                }
                return -1;
            }
            surface_locked = true;
        }

        Octree octree;
        if (!Octree_Init(&octree, hasTransparency ? numPaletteColors - 1 : numPaletteColors)) {
            if (surface_locked) {
                SDL_UnlockSurface(surf);
            }
            if (surface_converted) {
                SDL_DestroySurface(surf);
            }
            return -1;
        }
        octree.palette = (uint8_t *)SDL_calloc(numPaletteColors, 3);
        if (!octree.palette) {
            Octree_Free(&octree);
            if (surface_locked) {
                SDL_UnlockSurface(surf);
            }
            if (surface_converted) {
                SDL_DestroySurface(surf);
            }
            SDL_SetError("Failed to allocate palette memory.");
            return -1;
        }
        octree.paletteSize = 0;
        octree.maxColors = hasTransparency ? numPaletteColors - 1 : numPaletteColors;
        octree.leafCount = 0;
        for (int i = 0; i < OCTREE_MAX_LEVELS; ++i) {
            octree.head_leaf_list[i] = NULL;
            octree.tail_leaf_list[i] = NULL;
        }
        OctreeNode *root = octree.root;
        if (!root) {
            Octree_Free(&octree);
            if (surface_locked) {
                SDL_UnlockSurface(surf);
            }
            if (surface_converted) {
                SDL_DestroySurface(surf);
            }
            SDL_SetError("Failed to create Octree root node.");
            return -1;
        }

        root->pixelCount = 0;
        root->rSum = 0;
        root->gSum = 0;
        root->bSum = 0;
        root->paletteIndex = -1;
        root->isLeaf = true;
        root->inLeafList = false;
        root->next_leaf = NULL;
        root->prev_leaf = NULL;

        const int r_bpp = count_set_bits(pixelFormatDetails->Rmask);
        const int g_bpp = count_set_bits(pixelFormatDetails->Gmask);
        const int b_bpp = count_set_bits(pixelFormatDetails->Bmask);

        const Uint32 current_Amask = pixelFormatDetails->Amask;
        const Uint8 current_Ashift = pixelFormatDetails->Ashift;
        const int current_a_bpp = (current_Amask == 0) ? 0 : count_set_bits(current_Amask);

        for (int y = 0; y < surf->h; ++y) {
            uint32_t *src_row = (uint32_t *)((uint8_t *)surf->pixels + y * surf->pitch);
            for (int x = 0; x < surf->w; ++x) {
                uint32_t pixel = src_row[x];

                bool isTransparent = false;
                if (hasTransparency) {
                    if (current_Amask != 0 && current_a_bpp > 0) {
                        uint8_t a = (uint8_t)(((pixel & current_Amask) >> current_Ashift) << (8 - current_a_bpp));
                        if (a < 128) { // Alpha threshold
                            isTransparent = true;
                        }
                    } else if (SDL_SurfaceHasColorKey(psurf) && (pixel == colorKey)) {
                        isTransparent = true;
                    }
                }

                if (isTransparent) {
                    continue;
                } else {
                    uint8_t r = (uint8_t)(((pixel & pixelFormatDetails->Rmask) >> pixelFormatDetails->Rshift) << (8 - r_bpp));
                    uint8_t g = (uint8_t)(((pixel & pixelFormatDetails->Gmask) >> pixelFormatDetails->Gshift) << (8 - g_bpp));
                    uint8_t b = (uint8_t)(((pixel & pixelFormatDetails->Bmask) >> pixelFormatDetails->Bshift) << (8 - b_bpp));
                    Octree_InsertColor(&octree, root, r, g, b, 0);
                }
            }
        }

        while (octree.leafCount > octree.maxColors && octree.leafCount > 1) {
            if (!Octree_Reduce(&octree)) {
                Octree_Free(&octree);
                if (surface_converted) {
                    SDL_DestroySurface(surf);
                } else if (surface_locked) {
                    SDL_UnlockSurface(surf);
                }
                return -1;
            }
        }

        int paletteIndexCounter = 0;
        octree.paletteSize = 0;
        if (!Octree_BuildPalette(&octree, octree.root, &paletteIndexCounter)) {
            SDL_SetError("Failed to build palette from Octree.");
            Octree_Free(&octree);
            if (surface_converted) {
                SDL_DestroySurface(surf);
            } else if (surface_locked) {
                SDL_UnlockSurface(surf);
            }
            return -1;
        }
        if (paletteIndexCounter > (int)octree.maxColors) {
            SDL_SetError("Octree built more colors than expected.");
            Octree_Free(&octree);
            if (surface_converted) {
                SDL_DestroySurface(surf);
            } else if (surface_locked) {
                SDL_UnlockSurface(surf);
            }
            return -1;
        }

        uint32_t destIndex = 0;
        for (uint32_t i = 0; i < octree.paletteSize; ++i) {
            if (hasTransparency && destIndex == (uint32_t)transparentIndex) {
                destIndex++;
            }

            palette[destIndex][0] = octree.palette[i * 3 + 0];
            palette[destIndex][1] = octree.palette[i * 3 + 1];
            palette[destIndex][2] = octree.palette[i * 3 + 2];
            destIndex++;
        }

        for (uint32_t i = destIndex; i < numPaletteColors; ++i) {
            palette[i][0] = 0;
            palette[i][1] = 0;
            palette[i][2] = 0;
        }

        if (hasTransparency) {
            palette[transparentIndex][0] = 0;
            palette[transparentIndex][1] = 0;
            palette[transparentIndex][2] = 0;
        }

        for (int y = 0; y < surf->h; ++y) {
            uint32_t *src_row = (uint32_t *)((uint8_t *)surf->pixels + y * surf->pitch);
            uint8_t *dst_row = indexedPixels + y * surf->w;
            for (int x = 0; x < surf->w; ++x) {
                uint32_t pixel = src_row[x];

                bool isTransparent = false;
                if (hasTransparency) {
                    if (current_Amask != 0 && current_a_bpp > 0) {
                        uint8_t a = (uint8_t)(((pixel & current_Amask) >> current_Ashift) << (8 - current_a_bpp));
                        if (a < 128) { // Alpha threshold
                            isTransparent = true;
                        }
                    } else if (SDL_SurfaceHasColorKey(psurf) && (pixel == colorKey)) {
                        isTransparent = true;
                    }
                }

                if (isTransparent) {
                    dst_row[x] = transparentIndex;
                } else {
                    uint8_t r = (uint8_t)(((pixel & pixelFormatDetails->Rmask) >> pixelFormatDetails->Rshift) << (8 - r_bpp));
                    uint8_t g = (uint8_t)(((pixel & pixelFormatDetails->Gmask) >> pixelFormatDetails->Gshift) << (8 - g_bpp));
                    uint8_t b = (uint8_t)(((pixel & pixelFormatDetails->Bmask) >> pixelFormatDetails->Bshift) << (8 - b_bpp));
                    int index = Octree_GetPaletteIndex(octree.root, r, g, b, 0);

                    if (hasTransparency && index >= transparentIndex) {
                        index++;
                    }

                    dst_row[x] = index;
                }
            }
        }

        // Clean up the octree
        Octree_Free(&octree);
        if (surface_converted) {
            SDL_DestroySurface(surf);
        } else if (surface_locked) {
            SDL_UnlockSurface(surf);
        }
        return 0;
    }
#endif /* !defined(SAVE_GIF_OCTREE) */
}

static int writeGifHeader(SDL_IOStream *io, uint16_t width, uint16_t height,
                          bool hasGlobalColorTable, uint8_t colorResolution, bool sortedColorTable,
                          uint8_t backgroundColorIndex, uint8_t pixelAspectRatio,
                          uint8_t gctSizeFieldValue)
{
    // Write signature and version (6 bytes)
    if (SDL_WriteIO(io, "GIF89a", 6) != 6) {
        SDL_SetError("Failed to write GIF signature and version.");
        return -1;
    }

    // Write width (2 bytes, little-endian)
    if (!writeWord(io, width)) {
        SDL_SetError("Failed to write GIF width.");
        return -1;
    }

    // Write height (2 bytes, little-endian)
    if (!writeWord(io, height)) {
        SDL_SetError("Failed to write GIF height.");
        return -1;
    }

    // Pack Global Color Table Info byte
    uint8_t globalColorTableInfo = 0;
    if (hasGlobalColorTable) {
        globalColorTableInfo |= 0x80;                       // Set GCT Flag (bit 7)
        globalColorTableInfo |= (gctSizeFieldValue & 0x07); // Set GCT Size (bits 0-2)
    }
    globalColorTableInfo |= ((colorResolution - 1) & 0x07) << 4; // Set Color Resolution (bits 4-6)
    if (sortedColorTable) {
        globalColorTableInfo |= 0x08; // Set Sorted Flag (bit 3)
    }

    // Write packed fields byte
    if (!writeByte(io, globalColorTableInfo)) {
        SDL_SetError("Failed to write GIF packed fields byte.");
        return -1;
    }

    // Write background color index
    if (!writeByte(io, backgroundColorIndex)) {
        SDL_SetError("Failed to write GIF background color index.");
        return -1;
    }

    // Write pixel aspect ratio
    if (!writeByte(io, pixelAspectRatio)) {
        SDL_SetError("Failed to write GIF pixel aspect ratio.");
        return -1;
    }

    return 0;
}

static int writeColorTable(SDL_IOStream *io, uint8_t colors[][3], uint16_t numColors)
{
    if (!io || !colors || numColors == 0)
        return -1;

    if (SDL_WriteIO(io, colors, numColors * 3) != numColors * 3u) {
        SDL_SetError("Failed to write color table data");
        return -1;
    }

    return 0;
}

static int writeGraphicsControlExtension(SDL_IOStream *io, uint16_t delayTime, int transparentColorIndex, uint8_t disposalMethod)
{
    if (!io)
        return -1;

    // Extension Introducer
    if (!writeByte(io, 0x21)) {
        return -1;
    }
    // Graphic Control Label
    if (!writeByte(io, 0xF9)) {
        return -1;
    }
    // Block Size
    if (!writeByte(io, 0x04)) {
        return -1;
    }

    uint8_t packedFields = 0;
    // Reserved (3 bits) - 0
    // Disposal Method (3 bits)
    packedFields |= (disposalMethod & 0x07) << 2;
    // User Input Flag (1 bit) - 0 (no user input)
    // Transparent Color Flag (1 bit)
    if (transparentColorIndex != -1) {
        packedFields |= (1 << 0);
    }
    if (!writeByte(io, packedFields)) {
        return -1;
    }

    if (!writeWord(io, delayTime)) {
        return -1;
    }
    if (!writeByte(io, (transparentColorIndex != -1) ? (uint8_t)transparentColorIndex : 0x00)) {
        return -1;
    }
    // Block Terminator
    if (!writeByte(io, 0x00)) {
        return -1;
    }

    return 0;
}

static int writeImageDescriptor(SDL_IOStream *io, uint16_t left, uint16_t top,
                         uint16_t width, uint16_t height,
                         int hasLocalColorTable, int interlace,
                         int sortedColorTable, uint8_t localColorTableSize)
{
    if (!io)
        return -1;

    // Image Separator
    if (!writeByte(io, 0x2C)) {
        return -1;
    }
    if (!writeWord(io, left)) {
        return -1;
    }
    if (!writeWord(io, top)) {
        return -1;
    }
    if (!writeWord(io, width)) {
        return -1;
    }
    if (!writeWord(io, height)) {
        return -1;
    }

    uint8_t packedFields = 0;
    // Local Color Table Flag (1 bit)
    if (hasLocalColorTable) {
        packedFields |= (1 << 7);
    }
    // Interlace Flag (1 bit)
    if (interlace) {
        packedFields |= (1 << 6);
    }
    // Sort Flag (1 bit)
    if (sortedColorTable) {
        packedFields |= (1 << 5);
    }
    // Reserved (2 bits) - 0
    // Size of Local Color Table (3 bits)
    if (hasLocalColorTable) {
        packedFields |= (localColorTableSize & 0x07);
    }
    if (!writeByte(io, packedFields)) {
        return -1;
    }

    return 0;
}

static int writeImageData(SDL_IOStream *io, uint8_t minCodeSize, const uint8_t *compressedData, size_t dataSize)
{
    if (!io || !compressedData)
        return -1; // dataSize can be 0 for empty images

    // LZW Minimum Code Size
    if (!writeByte(io, minCodeSize)) {
        return -1;
    }

    if (dataSize == 0) {
        // For empty images, write a zero-sized data block
        // Empty block
        writeByte(io, 0x00);
    } else {
        // Write data sub-blocks (max 255 bytes per sub-block)
        size_t bytesWritten = 0;
        while (bytesWritten < dataSize) {
            uint8_t subBlockSize = (uint8_t)((dataSize - bytesWritten > 255) ? 255 : (dataSize - bytesWritten));
            // Block Size
            if (!writeByte(io, subBlockSize)) {
                return -1;
            }
            if (!SDL_WriteIO(io, compressedData + bytesWritten, subBlockSize)) {
                return -1;
            }
            bytesWritten += subBlockSize;
        }
    }

    // Block Terminator (end of image data)
    if (!writeByte(io, 0x00)) {
        return -1;
    }

    return 0;
}

static int writeNetscapeLoopExtension(SDL_IOStream *io, uint16_t loopCount)
{
    if (!io)
        return -1;

    // Extension Introducer
    if (!writeByte(io, 0x21)) {
        return -1;
    }
    // Application Extension Label
    if (!writeByte(io, 0xFF)) {
        return -1;
    }
    // Block Size
    if (!writeByte(io, 0x0B)) {
        return -1;
    }

    // Application Identifier and Authentication Code
    if (SDL_WriteIO(io, "NETSCAPE2.0", 11) != 11) {
        return -1;
    }

    // Sub-block Size
    if (!writeByte(io, 0x03)) {
        return -1;
    }
    // Sub-block ID
    if (!writeByte(io, 0x01)) {
        return -1;
    }
    // Loop Count
    if (!writeWord(io, loopCount)) {
        return -1;
    }
    // Block Terminator
    if (!writeByte(io, 0x00)) {
        return -1;
    }

    return 0;
}

static int writeCommentExtension(SDL_IOStream *io, const char *comment)
{
    if (!io || !comment || SDL_strlen(comment) == 0) {
        return 0;
    }

    size_t remaining = SDL_strlen(comment);
    const char *current_pos = comment;

    // Extension Introducer: 0x21
    if (!writeByte(io, 0x21)) {
        return -1;
    }
    // Comment Extension Label: 0xFE
    if (!writeByte(io, 0xFE)) {
        return -1;
    }

    // Write the comment in sub-blocks of up to 255 bytes
    while (remaining > 0) {
        Uint8 sub_block_size = (remaining > 255) ? 255 : (Uint8)remaining;
        if (!writeByte(io, sub_block_size)) {
            return -1;
        }

        if (SDL_WriteIO(io, current_pos, sub_block_size) != sub_block_size) {
            return -1;
        }

        remaining -= sub_block_size;
        current_pos += sub_block_size;
    }

    // Block Terminator: 0x00
    if (!writeByte(io, 0x00)) {
        return -1;
    }

    return 0;
}

static int writeGifTrailer(SDL_IOStream *io)
{
    if (!io)
        return -1;

    // GIF Trailer
    if (!writeByte(io, 0x3B)) {
        return -1;
    }

    return 0;
}

static bool AnimationEncoder_AddFrame(IMG_AnimationEncoder *encoder, SDL_Surface *surface, Uint64 duration)
{
    IMG_AnimationEncoderContext *ctx = encoder->ctx;
    SDL_IOStream *io = encoder->dst;
    uint8_t *indexedPixels = NULL;
    uint8_t *compressedData = NULL;
    uint16_t numColors = ctx->numGlobalColors;
    uint8_t palette_bits_per_pixel = 0;
    uint8_t localColorTable[256][3];
    bool useLocalColorTable = !ctx->firstFrame;

    if (!io) {
        SDL_SetError("SDL_IOStream pointer (stream->dst) is NULL.");
        return false;
    }

    if (numColors > 1) {
        uint16_t temp_colors = numColors;
        while (temp_colors > 1 && palette_bits_per_pixel < 8) {
            temp_colors >>= 1;
            palette_bits_per_pixel++;
        }
    } else {
        palette_bits_per_pixel = 1;
    }

    size_t pixel_buffer_size = (size_t)surface->w * surface->h;
    if (pixel_buffer_size == 0 || pixel_buffer_size / (size_t)surface->w != (size_t)surface->h) {
        SDL_SetError("Surface dimensions too large for GIF encoding");
        return false;
    }
    indexedPixels = (uint8_t *)SDL_malloc(pixel_buffer_size);
    if (!indexedPixels) {
        SDL_SetError("Failed to allocate indexed pixel buffer.");
        goto error;
    }

    if (ctx->firstFrame) {
        ctx->width = (uint16_t)surface->w;
        ctx->height = (uint16_t)surface->h;

        if (quantizeSurfaceToIndexedPixels(surface, ctx->globalColorTable, numColors, indexedPixels, ctx->transparentColorIndex) != 0) {
            goto error;
        }

        if (ctx->use_lut) {
            // Build the fast lookup table for subsequent frames.
            if (!ctx->lut_initialized) {
                buildColorMapLUT(ctx->colorMapLUT, ctx->globalColorTable, numColors, (ctx->transparentColorIndex != -1));
                ctx->lut_initialized = true;
            }
        }

        uint8_t gct_size_field_value = (palette_bits_per_pixel > 0) ? (palette_bits_per_pixel - 1) : 0;
        if (writeGifHeader(io, ctx->width, ctx->height, true, 8, false, 0, 0, gct_size_field_value) != 0) {
            goto error;
        }
        if (writeColorTable(io, ctx->globalColorTable, numColors) != 0) {
            goto error;
        }

        int loopCount = 0;
        const char *description = NULL;
        if (ctx->metadata) {
            loopCount = (int)SDL_max(SDL_GetNumberProperty(ctx->metadata, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, 0), 0);
            description = SDL_GetStringProperty(ctx->metadata, IMG_PROP_METADATA_DESCRIPTION_STRING, NULL);
        }

        if (writeNetscapeLoopExtension(io, loopCount) != 0) {
            goto error;
        }

        if (description) {
            if (writeCommentExtension(io, description) != 0) {
                goto error;
            }
        }

    } else {
        if (surface->w != ctx->width || surface->h != ctx->height) {
            SDL_SetError("Frame dimensions (%dx%d) do not match GIF canvas dimensions (%dx%d).",
                         surface->w, surface->h, ctx->width, ctx->height);
            goto error;
        }

        if (ctx->use_lut) {
            // For subsequent frames, map pixels to the existing global palette using the fast LUT.
            if (mapSurfaceToExistingPalette(surface, ctx->colorMapLUT, indexedPixels, ctx->transparentColorIndex) != 0) {
                goto error;
            }
        } else {
            // For subsequent frames, create a new optimal palette
            if (quantizeSurfaceToIndexedPixels(surface, localColorTable, numColors, indexedPixels, ctx->transparentColorIndex) != 0) {
                goto error;
            }
        }
    }

    uint16_t resolvedDuration = (uint16_t)IMG_GetEncoderDuration(encoder, duration, 100);
    uint8_t disposalMethod = (ctx->transparentColorIndex != -1) ? 2 : 1;
    if (writeGraphicsControlExtension(io, resolvedDuration, ctx->transparentColorIndex, disposalMethod) != 0) {
        goto error;
    }

    if (ctx->use_lut) {
        // Write image descriptor, indicating we are NOT using a local color table.
        if (writeImageDescriptor(io, 0, 0, surface->w, surface->h, false, 0, 0, 0) != 0) {
            goto error;
        }
    } else {
        // Write image descriptor with local color table for non-first frames
        if (writeImageDescriptor(io, 0, 0, surface->w, surface->h,
                                 useLocalColorTable, 0, 0,
                                 useLocalColorTable ? palette_bits_per_pixel - 1 : 0) != 0) {
            goto error;
        }

        // Write local color table for non-first frames
        if (useLocalColorTable) {
            if (writeColorTable(io, localColorTable, numColors) != 0) {
                goto error;
            }
        }
    }

    size_t compressedSize = 0;
    uint8_t lzwMinCodeSize = SDL_max(2, palette_bits_per_pixel);

    if (lzwCompress(indexedPixels, surface->w, surface->h, lzwMinCodeSize,
                    &compressedData, &compressedSize, encoder->quality) != 0) {
        goto error;
    }

    if (writeImageData(io, lzwMinCodeSize, compressedData, compressedSize) != 0) {
        goto error;
    }

    SDL_free(indexedPixels);
    SDL_free(compressedData);

    if (ctx->firstFrame) {
        ctx->firstFrame = false;
    }

    return true;

error:
    SDL_free(indexedPixels);
    SDL_free(compressedData);
    return false;
}

static bool AnimationEncoder_End(IMG_AnimationEncoder *encoder)
{
    IMG_AnimationEncoderContext *ctx = encoder->ctx;
    SDL_IOStream *io = encoder->dst;
    bool success = true;

    if (ctx->metadata) {
        SDL_DestroyProperties(ctx->metadata);
        ctx->metadata = 0;
    }

    if (io) {
        if (writeGifTrailer(io) != 0) {
            SDL_SetError("Failed to write GIF trailer.");
            success = false;
        }
    } else {
        SDL_SetError("SDL_IOStream pointer (stream->dst) is NULL.");
        success = false;
    }

    SDL_free(ctx);
    encoder->ctx = NULL;

    return success;
}

bool IMG_CreateGIFAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    IMG_AnimationEncoderContext *ctx;
    int transparent_index = -1;
    uint16_t num_global_colors = 256;

    transparent_index = (int)SDL_GetNumberProperty(props, "transparent_color_index", -1);
    Sint64 globalcolors = SDL_GetNumberProperty(props, "num_colors", 256);
    if (globalcolors <= 1 || (globalcolors & (globalcolors - 1)) != 0 || globalcolors > 256) {
        return SDL_SetError("GIF stream property 'num_colors' must be a power of 2 (starting from 2, up to 256).");
    }

    num_global_colors = (uint16_t)globalcolors;

    if (transparent_index >= (int)num_global_colors) {
        return SDL_SetError("Transparent color index %d exceeds palette size %d", transparent_index, num_global_colors);
    }

    if (transparent_index < 0) {
        transparent_index = num_global_colors - 1;
    }

    ctx = (IMG_AnimationEncoderContext *)SDL_calloc(1, sizeof(IMG_AnimationEncoderContext));
    if (!ctx) {
        return false;
    }

    if (encoder->quality < 0)
        encoder->quality = 75;
    else if (encoder->quality > 100)
        encoder->quality = 100;

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    if (!ignoreProps) {
        ctx->metadata = SDL_CreateProperties();
        if (!ctx->metadata) {
            SDL_free(ctx);
            return false;
        }
        if (!SDL_CopyProperties(props, ctx->metadata)) {
            SDL_DestroyProperties(ctx->metadata);
            SDL_free(ctx);
            return false;
        }
    }

    ctx->width = 0;
    ctx->height = 0;
    ctx->numGlobalColors = num_global_colors;
    ctx->transparentColorIndex = transparent_index;
    ctx->firstFrame = true;
    ctx->use_lut = SDL_GetBooleanProperty(props, "use_lut", false);

    encoder->ctx = ctx;
    encoder->AddFrame = AnimationEncoder_AddFrame;
    encoder->Close = AnimationEncoder_End;

    return true;
}

bool IMG_SaveGIF_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    IMG_AnimationEncoder *encoder = IMG_CreateAnimationEncoder_IO(dst, closeio, "gif");
    if (!encoder) {
        return false;
    }

    if (!IMG_AddAnimationEncoderFrame(encoder, surface, 0)) {
        IMG_CloseAnimationEncoder(encoder);
        return false;
    }

    if (!IMG_CloseAnimationEncoder(encoder)) {
        return false;
    }

    return true;
}

bool IMG_SaveGIF(SDL_Surface *surface, const char *file)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return IMG_SaveGIF_IO(surface, dst, true);
    } else {
        return false;
    }
}

#else

bool IMG_CreateGIFAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    (void)encoder;
    (void)props;
    return SDL_SetError("SDL_image built without GIF save support");
}

bool IMG_SaveGIF_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    return SDL_SetError("SDL_image built without GIF save support");
}

bool IMG_SaveGIF(SDL_Surface *surface, const char *file)
{
    (void)surface;
    (void)file;
    return SDL_SetError("SDL_image built without GIF save support");
}

#endif /* SAVE_GIF */
