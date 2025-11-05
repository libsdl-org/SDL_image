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

#include <SDL3_image/SDL_image.h>

#include "IMG_ani.h"
#include "IMG_anim_encoder.h"
#include "IMG_anim_decoder.h"

// We will have the saving ANI feature by default
#if !defined(SAVE_ANI)
#define SAVE_ANI 1
#endif


#ifndef LOAD_ANI
#undef SAVE_ANI
#define SAVE_ANI 0
#endif

#ifdef LOAD_ANI

#define RIFF_FOURCC(c0, c1, c2, c3)                 \
    ((Uint32)(Uint8)(c0) | ((Uint32)(Uint8)(c1) << 8) | \
     ((Uint32)(Uint8)(c2) << 16) | ((Uint32)(Uint8)(c3) << 24))

#define ANI_FLAG_ICON       0x1
#define ANI_FLAG_SEQUENCE   0x2

typedef struct
{
    Uint32 riffID;
    Uint32 cbSize;
    Uint32 chunkID;
} RIFFHEADER;

typedef struct {
    Uint32 cbSizeof; // sizeof(ANIHEADER) = 36 bytes.
    Uint32 frames;   // Number of frames in the frame list.
    Uint32 steps;    // Number of steps in the animation loop.
    Uint32 width;    // Width
    Uint32 height;   // Height
    Uint32 bpp;      // bpp
    Uint32 planes;   // Not used
    Uint32 jifRate;  // Default display rate, in jiffies (1/60s)
    Uint32 fl;       // AF_ICON should be set. AF_SEQUENCE is optional
} ANIHEADER;


bool IMG_isANI(SDL_IOStream* src)
{
    Sint64 start;
    bool is_ANI;
    RIFFHEADER header;

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_ANI = false;
    if (SDL_ReadU32LE(src, &header.riffID) &&
        SDL_ReadU32LE(src, &header.cbSize) &&
        SDL_ReadU32LE(src, &header.chunkID) &&
        header.riffID == RIFF_FOURCC('R', 'I', 'F', 'F') &&
        header.chunkID == RIFF_FOURCC('A', 'C', 'O', 'N')) {
        is_ANI = true;
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_ANI;
}

struct IMG_AnimationDecoderContext
{
    Uint32 frame_index;
    Uint32 frame_count;
    Sint64 *frame_offsets;
    Uint32 *frame_durations;
    Uint32 *frame_sequence;
};

typedef struct
{
    IMG_AnimationDecoderContext *ctx;
    SDL_IOStream *src;
    bool has_anih;
    ANIHEADER anih;
    char *author;
    char *title;
} IMG_AnimationParseContext;

static bool IMG_AnimationDecoderReset_Internal(IMG_AnimationDecoder* decoder)
{
    IMG_AnimationDecoderContext* ctx = decoder->ctx;

    ctx->frame_index = 0;

    return true;
}

static bool IMG_AnimationDecoderGetNextFrame_Internal(IMG_AnimationDecoder *decoder, SDL_Surface **frame, Uint64 *duration)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;

    if (ctx->frame_index == ctx->frame_count) {
        decoder->status = IMG_DECODER_STATUS_COMPLETE;
        return false;
    }

    *duration = IMG_GetDecoderDuration(decoder, ctx->frame_durations[ctx->frame_index], 60);

    Sint64 offset = ctx->frame_offsets[ctx->frame_sequence[ctx->frame_index]];
    if (SDL_SeekIO(decoder->src, offset, SDL_IO_SEEK_SET) < 0) {
        return SDL_SetError("Failed to seek to frame offset");
    }
    if (IMG_isCUR(decoder->src)) {
        *frame = IMG_LoadCUR_IO(decoder->src);
    } else if (IMG_isICO(decoder->src)) {
        *frame = IMG_LoadICO_IO(decoder->src);
    } else {
        SDL_SetError("Unrecognized frame type");
        *frame = NULL;
    }
    ++ctx->frame_index;

    return (*frame != NULL);
}

static bool IMG_AnimationDecoderClose_Internal(IMG_AnimationDecoder *decoder)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;

    SDL_free(ctx->frame_offsets);
    SDL_free(ctx->frame_durations);
    SDL_free(ctx->frame_sequence);
    SDL_free(ctx);
    decoder->ctx = NULL;
    return true;
}

static bool ParseANIHeader(IMG_AnimationParseContext *parse, Uint32 size)
{
    SDL_IOStream *src = parse->src;
    IMG_AnimationDecoderContext *ctx = parse->ctx;
    ANIHEADER *anih = &parse->anih;

    if (size != sizeof(*anih)) {
        return SDL_SetError("Invalid ANI header");
    }

    if (parse->has_anih) {
        // Ignore duplicate 'anih' chunk
        return true;
    }

    if (!SDL_ReadU32LE(src, &anih->cbSizeof) ||
        !SDL_ReadU32LE(src, &anih->frames) ||
        !SDL_ReadU32LE(src, &anih->steps) ||
        !SDL_ReadU32LE(src, &anih->width) ||
        !SDL_ReadU32LE(src, &anih->height) ||
        !SDL_ReadU32LE(src, &anih->bpp) ||
        !SDL_ReadU32LE(src, &anih->planes) ||
        !SDL_ReadU32LE(src, &anih->jifRate) ||
        !SDL_ReadU32LE(src, &anih->fl)) {
        return SDL_SetError("Couldn't read ANI header");
    }
    parse->has_anih = true;

    if (anih->cbSizeof != sizeof(*anih) ||
        anih->frames == 0 ||
        anih->steps == 0) {
        return SDL_SetError("Invalid ANI header");
    }

    // We could support raw frames if we get an example of this
    if (!(anih->fl & ANI_FLAG_ICON)) {
        return SDL_SetError("Raw ANI frames are unsupported");
    }

    ctx->frame_count = anih->steps;
    ctx->frame_offsets = (Sint64 *)SDL_calloc(anih->frames, sizeof(*ctx->frame_offsets));
    ctx->frame_durations = (Uint32 *)SDL_calloc(ctx->frame_count, sizeof(*ctx->frame_durations));
    ctx->frame_sequence = (Uint32 *)SDL_calloc(ctx->frame_count, sizeof(*ctx->frame_durations));
    if (!ctx->frame_offsets || !ctx->frame_durations || !ctx->frame_sequence) {
        return false;
    }

    for (Uint32 i = 0; i < ctx->frame_count; ++i) {
        ctx->frame_sequence[i] = i;
        ctx->frame_durations[i] = anih->jifRate;
    }
    return true;
}

static bool ParseInfoList(IMG_AnimationParseContext *parse, Uint32 list_size)
{
    SDL_IOStream *src = parse->src;

    Sint64 offset = 0;
    while (offset <= (list_size - 8)) {
        Uint32 chunk;
        Uint32 size;
        if (!SDL_ReadU32LE(src, &chunk) ||
            !SDL_ReadU32LE(src, &size)) {
            return false;
        }
        offset += 8;

        if ((offset + size) > list_size) {
            return SDL_SetError("Corrupt ANI data while reading info list");
        }

        if (chunk == RIFF_FOURCC('I', 'N', 'A', 'M') && size > 0) {
            parse->title = (char *)SDL_malloc(size);
            if (!parse->title) {
                return false;
            }
            if (!SDL_ReadIO(src, parse->title, size)) {
                return false;
            }
            parse->title[size - 1] = '\0';

        } else if (chunk == RIFF_FOURCC('I', 'A', 'R', 'T') && size > 0) {
            parse->author = (char *)SDL_malloc(size);
            if (!parse->author) {
                return false;
            }
            if (!SDL_ReadIO(src, parse->author, size)) {
                return false;
            }
            parse->author[size - 1] = '\0';
        } else {
            if (SDL_SeekIO(src, size, SDL_IO_SEEK_CUR) < 0) {
                return false;
            }
        }
        offset += size;
    }
    return true;
}

static bool ParseFrameList(IMG_AnimationParseContext *parse, Uint32 list_size)
{
    SDL_IOStream *src = parse->src;
    IMG_AnimationDecoderContext *ctx = parse->ctx;
    ANIHEADER *anih = &parse->anih;

    if (!parse->has_anih) {
        return SDL_SetError("Missing ANI header");
    }

    Uint32 frame_count = 0;
    Sint64 offset = 0;
    while (offset <= (list_size - 8) && frame_count < anih->frames) {
        Uint32 chunk;
        Uint32 size;
        if (!SDL_ReadU32LE(src, &chunk) ||
            !SDL_ReadU32LE(src, &size)) {
            return false;
        }
        offset += 8;

        if ((offset + size) > list_size) {
            return SDL_SetError("Corrupt ANI data while reading frame list");
        }

        if (chunk == RIFF_FOURCC('i', 'c', 'o', 'n')) {
            Sint64 frame_offset = SDL_TellIO(src);
            if (frame_offset < 0) {
                return false;
            }
            ctx->frame_offsets[frame_count++] = frame_offset;
        }
        if (SDL_SeekIO(src, size, SDL_IO_SEEK_CUR) < 0) {
            return false;
        }
        offset += size;
    }
    if (frame_count < anih->frames) {
        return SDL_SetError("Missing frames, read %d of %d", frame_count, anih->frames);
    }
    return true;
}

static bool ParseList(IMG_AnimationParseContext *parse, Uint32 size)
{
    SDL_IOStream *src = parse->src;
    Uint32 type;

    if (!SDL_ReadU32LE(src, &type)) {
        return false;
    }
    size -= 4;

    if (type == RIFF_FOURCC('I', 'N', 'F', 'O')) {
        return ParseInfoList(parse, size);
    } else if (type == RIFF_FOURCC('f', 'r', 'a', 'm')) {
        return ParseFrameList(parse, size);
    } else {
        // Unknown list chunk, ignore it
        return true;
    }
}

static bool ParseSequenceChunk(IMG_AnimationParseContext *parse, Uint32 size)
{
    SDL_IOStream *src = parse->src;
    IMG_AnimationDecoderContext *ctx = parse->ctx;
    ANIHEADER *anih = &parse->anih;

    if (!parse->has_anih) {
        return SDL_SetError("Missing ANI header");
    }

    if (!(anih->fl & ANI_FLAG_SEQUENCE)) {
        // The header says we don't use sequence data, ignore it
        return true;
    }

    if (size != ctx->frame_count * sizeof(Uint32)) {
        return SDL_SetError("Invalid sequence chunk");
    }
    for (Uint32 i = 0; i < ctx->frame_count; ++i) {
        if (!SDL_ReadU32LE(src, &ctx->frame_sequence[i])) {
            return false;
        }
        if (ctx->frame_sequence[i] >= anih->frames) {
            return SDL_SetError("Invalid sequence chunk");
        }
    }
    return true;
}

static bool ParseRateChunk(IMG_AnimationParseContext *parse, Uint32 size)
{
    SDL_IOStream *src = parse->src;
    IMG_AnimationDecoderContext *ctx = parse->ctx;

    if (!parse->has_anih) {
        return SDL_SetError("Missing ANI header");
    }

    if (size != ctx->frame_count * sizeof(Uint32)) {
        return SDL_SetError("Invalid rate chunk");
    }
    for (Uint32 i = 0; i < ctx->frame_count; ++i) {
        if (!SDL_ReadU32LE(src, &ctx->frame_durations[i])) {
            return false;
        }
    }
    return true;
}

bool IMG_CreateANIAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    IMG_AnimationParseContext parse;
    bool result = false;

    IMG_AnimationDecoderContext *ctx = (IMG_AnimationDecoderContext *)SDL_calloc(1, sizeof(IMG_AnimationDecoderContext));
    if (!ctx) {
        return false;
    }
    decoder->ctx = ctx;

    SDL_zero(parse);
    parse.src = decoder->src;
    parse.ctx = ctx;

    RIFFHEADER header;
    if (!SDL_ReadU32LE(decoder->src, &header.riffID) ||
        !SDL_ReadU32LE(decoder->src, &header.cbSize) ||
        !SDL_ReadU32LE(decoder->src, &header.chunkID)) {
        SDL_SetError("Couldn't read RIFF header");
        goto done;
    }

    if (header.riffID != RIFF_FOURCC('R', 'I', 'F', 'F') ||
        header.chunkID != RIFF_FOURCC('A', 'C', 'O', 'N')) {
        SDL_SetError("Invalid RIFF header");
        goto done;
    }
    Sint64 offset = 4;
    Sint64 end = header.cbSize;

    while (offset < end) {
        Uint32 chunk;
        Uint32 size;
        if (!SDL_ReadU32LE(decoder->src, &chunk) ||
            !SDL_ReadU32LE(decoder->src, &size)) {
            goto done;
        }
        offset += 8;

        if ((offset + size) > end) {
            SDL_SetError("Truncated ANI data");
            goto done;
        }

        if (chunk == RIFF_FOURCC('a', 'n', 'i', 'h')) {
            if (!ParseANIHeader(&parse, size)) {
                goto done;
            }
        } else if (chunk == RIFF_FOURCC('L', 'I', 'S', 'T')) {
            if (!ParseList(&parse, size)) {
                goto done;
            }
        } else if (chunk == RIFF_FOURCC('s', 'e', 'q', ' ')) {
            if (!ParseSequenceChunk(&parse, size)) {
                goto done;
            }
        } else if (chunk == RIFF_FOURCC('r', 'a', 't', 'e')) {
            if (!ParseRateChunk(&parse, size)) {
                goto done;
            }
        } else {
            if (SDL_SeekIO(decoder->src, size, SDL_IO_SEEK_CUR) < 0) {
                goto done;
            }
        }
        offset += size;
    }

    decoder->GetNextFrame = IMG_AnimationDecoderGetNextFrame_Internal;
    decoder->Reset = IMG_AnimationDecoderReset_Internal;
    decoder->Close = IMG_AnimationDecoderClose_Internal;

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    if (!ignoreProps) {
        // Allow implicit properties to be set which are not globalized but specific to the decoder.
        SDL_SetNumberProperty(decoder->props, "IMG_PROP_METADATA_FRAME_COUNT_NUMBER", ctx->frame_count);

        if (parse.title && *parse.title) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_TITLE_STRING, parse.title);
        }
        if (parse.author && *parse.author) {
            SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_AUTHOR_STRING, parse.author);
        }
    }

    result = true;

done:
    SDL_free(parse.author);
    SDL_free(parse.title);
    if (!result) {
        IMG_AnimationDecoderClose_Internal(decoder);
    }
    return result;
}

#else

bool IMG_isANI(SDL_IOStream *src)
{
    (void)src;
    return false;
}

bool IMG_CreateANIAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    (void)decoder;
    (void)props;
    return SDL_SetError("SDL_image built without ANI support");
}

#endif // LOAD_ANI

#if SAVE_ANI

struct IMG_AnimationEncoderContext
{
    char *author;
    char *title;
    int num_frames;
    int max_frames;
    SDL_Surface **frames;
    Uint64 *durations;
};


static bool AnimationEncoder_AddFrame(IMG_AnimationEncoder *encoder, SDL_Surface *surface, Uint64 duration)
{
    IMG_AnimationEncoderContext *ctx = encoder->ctx;

    if (ctx->num_frames == ctx->max_frames) {
        int max_frames = ctx->max_frames + 8;

        SDL_Surface **frames = (SDL_Surface **)SDL_realloc(ctx->frames, max_frames * sizeof(*frames));
        if (!frames) {
            return false;
        }

        Uint64 *durations = (Uint64 *)SDL_realloc(ctx->durations, max_frames * sizeof(*durations));
        if (!durations) {
            SDL_free(frames);
            return false;
        }

        ctx->frames = frames;
        ctx->durations = durations;
        ctx->max_frames = max_frames;
    }

    ctx->frames[ctx->num_frames] = surface;
    ++surface->refcount;
    ctx->durations[ctx->num_frames] = duration;
    ++ctx->num_frames;

    return true;
}

static bool SaveChunkSize(SDL_IOStream *dst, Sint64 offset)
{
    Sint64 here = SDL_TellIO(dst);
    if (here < 0) {
        return false;
    }
    if (SDL_SeekIO(dst, offset, SDL_IO_SEEK_SET) < 0) {
        return false;
    }

    Uint32 size = (Uint32)(here - (offset + sizeof(Uint32)));
    if (!SDL_WriteU32LE(dst, size)) {
        return false;
    }
    return SDL_SeekIO(dst, here, SDL_IO_SEEK_SET);
}

static bool WriteIconFrame(SDL_Surface *surface, SDL_IOStream *dst)
{
    bool result = true;

    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('i', 'c', 'o', 'n'));
    Sint64 icon_size_offset = SDL_TellIO(dst);
    result &= SDL_WriteU32LE(dst, 0);
    // Technically this could be ICO format, but it's generally animated cursors
    result &= IMG_SaveCUR_IO(surface, dst, false);
    result &= SaveChunkSize(dst, icon_size_offset);

    return result;
}

static bool WriteAnimInfo(IMG_AnimationEncoderContext *ctx, SDL_IOStream *dst)
{
    bool result = true;

    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('L', 'I', 'S', 'T'));
    Sint64 list_size_offset = SDL_TellIO(dst);
    result &= SDL_WriteU32LE(dst, 0);
    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('I', 'N', 'F', 'O'));

    if (ctx->title) {
        Uint32 size = (Uint32)(SDL_strlen(ctx->title) + 1);
        result &= SDL_WriteU32LE(dst, RIFF_FOURCC('I', 'N', 'A', 'M'));
        result &= SDL_WriteU32LE(dst, size);
        result &= (SDL_WriteIO(dst, ctx->title, size) == size);
    }

    if (ctx->author) {
        Uint32 size = (Uint32)(SDL_strlen(ctx->author) + 1);
        result &= SDL_WriteU32LE(dst, RIFF_FOURCC('I', 'A', 'R', 'T'));
        result &= SDL_WriteU32LE(dst, size);
        result &= (SDL_WriteIO(dst, ctx->author, size) == size);
    }

    result &= SaveChunkSize(dst, list_size_offset);

    return result;
}

static bool WriteAnimation(IMG_AnimationEncoder *encoder)
{
    IMG_AnimationEncoderContext *ctx = encoder->ctx;
    SDL_IOStream *dst = encoder->dst;
    bool result = true;

    // RIFF header
    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('R', 'I', 'F', 'F'));
    Sint64 riff_size_offset = SDL_TellIO(dst);
    result &= SDL_WriteU32LE(dst, 0);
    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('A', 'C', 'O', 'N'));

    // anih header chunk
    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('a', 'n', 'i', 'h'));
    result &= SDL_WriteU32LE(dst, sizeof(ANIHEADER));

    ANIHEADER anih;
    SDL_zero(anih);
    anih.cbSizeof = sizeof(anih);
    anih.frames = ctx->num_frames;
    anih.steps = ctx->num_frames;
    anih.jifRate = 1;
    anih.fl = ANI_FLAG_ICON;
    result &= (SDL_WriteIO(dst, &anih, sizeof(anih)) == sizeof(anih));

    // Info list
    if (ctx->author || ctx->title) {
        WriteAnimInfo(ctx, dst);
    }

    // Rate chunk
    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('r', 'a', 't', 'e'));
    result &= SDL_WriteU32LE(dst, sizeof(Uint32) * ctx->num_frames);
    for (int i = 0; i < ctx->num_frames; ++i) {
        Uint32 duration = (Uint32)IMG_GetEncoderDuration(encoder, ctx->durations[i], 60);
        result &= SDL_WriteU32LE(dst, duration);
    }

    // Frame list
    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('L', 'I', 'S', 'T'));
    Sint64 frame_list_size_offset = SDL_TellIO(dst);
    result &= SDL_WriteU32LE(dst, 0);
    result &= SDL_WriteU32LE(dst, RIFF_FOURCC('f', 'r', 'a', 'm'));

    for (int i = 0; i < ctx->num_frames; ++i) {
        result &= WriteIconFrame(ctx->frames[i], dst);
    }
    result &= SaveChunkSize(dst, frame_list_size_offset);

    // All done!
    result &= SaveChunkSize(dst, riff_size_offset);

    return result;
}

static bool AnimationEncoder_End(IMG_AnimationEncoder *encoder)
{
    IMG_AnimationEncoderContext *ctx = encoder->ctx;
    bool result = true;

    if (ctx->num_frames > 0) {
        result = WriteAnimation(encoder);

        for (int i = 0; i < ctx->num_frames; ++i) {
            SDL_DestroySurface(ctx->frames[i]);
        }
        SDL_free(ctx->frames);
        SDL_free(ctx->durations);
    }

    SDL_free(ctx->author);
    SDL_free(ctx->title);
    SDL_free(ctx);
    encoder->ctx = NULL;

    return result;
}

bool IMG_CreateANIAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    IMG_AnimationEncoderContext *ctx;

    ctx = (IMG_AnimationEncoderContext *)SDL_calloc(1, sizeof(IMG_AnimationEncoderContext));
    if (!ctx) {
        return false;
    }

    const char *author = SDL_GetStringProperty(props, IMG_PROP_METADATA_AUTHOR_STRING, NULL);
    if (author && *author) {
        ctx->author = SDL_strdup(author);
    }

    const char *title = SDL_GetStringProperty(props, IMG_PROP_METADATA_TITLE_STRING, NULL);
    if (title && *title) {
        ctx->title = SDL_strdup(title);
    }

    encoder->ctx = ctx;
    encoder->AddFrame = AnimationEncoder_AddFrame;
    encoder->Close = AnimationEncoder_End;

    return true;
}

#else

bool IMG_CreateANIAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    (void)encoder;
    (void)props;
    return SDL_SetError("SDL_image built without ANI save support");
}

#endif // SAVE_ANI
