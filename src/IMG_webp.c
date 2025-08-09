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

/* This is a WEBP image file loading framework */

#include <SDL3_image/SDL_image.h>

#include "IMG_anim_encoder.h"
#include "IMG_anim_decoder.h"
#include "IMG_webp.h"

// We will have the saving WEBP feature by default
#if !defined(SAVE_WEBP)
#ifdef LOAD_WEBP
#define SAVE_WEBP 1
#else
#define SAVE_WEBP 0
#endif
#endif

#ifdef LOAD_WEBP

/*=============================================================================
        File: SDL_webp.c
     Purpose: A WEBP loader for the SDL library
    Revision:
  Created by: Michael Bonfils (Murlock) (26 November 2011)
              murlock42@gmail.com

=============================================================================*/

#ifdef macintosh
#define MACOS
#endif
#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>
#include <webp/mux.h>
#include <webp/types.h>

static struct
{
    int loaded;
    void *handle_libwebpdemux;
    void *handle_libwebp;
    void *handle_libwebpmux;

    VP8StatusCode (*WebPGetFeaturesInternal)(const uint8_t *data, size_t data_size, WebPBitstreamFeatures *features, int decoder_abi_version);
    uint8_t *(*WebPDecodeRGBInto)(const uint8_t *data, size_t data_size, uint8_t *output_buffer, size_t output_buffer_size, int output_stride);
    uint8_t *(*WebPDecodeRGBAInto)(const uint8_t *data, size_t data_size, uint8_t *output_buffer, size_t output_buffer_size, int output_stride);
    WebPDemuxer *(*WebPDemuxInternal)(const WebPData *data, int allow_partial, WebPDemuxState *state, int version);
    int (*WebPDemuxGetFrame)(const WebPDemuxer *dmux, int frame_number, WebPIterator *iter);
    int (*WebPDemuxNextFrame)(WebPIterator *iter);
    void (*WebPDemuxReleaseIterator)(WebPIterator *iter);
    uint32_t (*WebPDemuxGetI)(const WebPDemuxer *dmux, WebPFormatFeature feature);
    void (*WebPDemuxDelete)(WebPDemuxer *dmux);

    // Encoding frame functions
    int (*WebPConfigInitInternal)(WebPConfig *, WebPPreset, float, int);
    int (*WebPValidateConfig)(const WebPConfig *);
    int (*WebPPictureInitInternal)(WebPPicture *, int);
    int (*WebPEncode)(const WebPConfig *, WebPPicture *);
    void (*WebPPictureFree)(WebPPicture *);
    int (*WebPPictureImportRGBA)(WebPPicture *, const uint8_t *, int);
    void (*WebPMemoryWriterInit)(WebPMemoryWriter *);
    int (*WebPMemoryWrite)(const uint8_t *, size_t, const WebPPicture *);
    void (*WebPMemoryWriterClear)(WebPMemoryWriter *);

    // Free function required for cleanup after muxing.
    void (*WebPFree)(void *ptr);

    // Muxing functions
    WebPAnimEncoder *(*WebPAnimEncoderNewInternal)(int, int, const WebPAnimEncoderOptions *, int); // Export #33 (0x0021),  (0x), 0x00003900, None
    int (*WebPAnimEncoderOptionsInitInternal)(WebPAnimEncoderOptions *, int);                 // Export #34 (0x0022),  (0x), 0x000038c0, None
    int (*WebPAnimEncoderAdd)(WebPAnimEncoder *, WebPPicture *, int, const WebPConfig *);     // Export #29 (0x001d),  (0x), 0x00003e60, None
    int (*WebPAnimEncoderAssemble)(WebPAnimEncoder *, WebPData *);                            // Export #30 (0x001e),  (0x), 0x00004580, None
    void (*WebPAnimEncoderDelete)(WebPAnimEncoder *);                                         // Export #31 (0x001f),  (0x), 0x00003cb0, None
} lib;

#if defined(LOAD_WEBP_DYNAMIC) && defined(LOAD_WEBPDEMUX_DYNAMIC) && defined(LOAD_WEBPMUX_DYNAMIC)
#define FUNCTION_LOADER_LIBWEBP(FUNC, SIG)                       \
    lib.FUNC = (SIG)SDL_LoadFunction(lib.handle_libwebp, #FUNC); \
    if (lib.FUNC == NULL) {                                      \
        SDL_UnloadObject(lib.handle_libwebp);                    \
        return false;                                            \
    }
#define FUNCTION_LOADER_LIBWEBPDEMUX(FUNC, SIG)                       \
    lib.FUNC = (SIG)SDL_LoadFunction(lib.handle_libwebpdemux, #FUNC); \
    if (lib.FUNC == NULL) {                                           \
        SDL_UnloadObject(lib.handle_libwebpdemux);                    \
        return false;                                                 \
    }
#define FUNCTION_LOADER_LIBWEBPMUX(FUNC, SIG)                       \
    lib.FUNC = (SIG)SDL_LoadFunction(lib.handle_libwebpmux, #FUNC); \
    if (lib.FUNC == NULL) {                                         \
        SDL_UnloadObject(lib.handle_libwebpmux);                    \
        return false;                                               \
    }
#else
#define FUNCTION_LOADER_LIBWEBP(FUNC, SIG)             \
    lib.FUNC = FUNC;                                   \
    if (lib.FUNC == NULL) {                            \
        return SDL_SetError("Missing webp.framework"); \
    }
#define FUNCTION_LOADER_LIBWEBPDEMUX(FUNC, SIG)             \
    lib.FUNC = FUNC;                                        \
    if (lib.FUNC == NULL) {                                 \
        return SDL_SetError("Missing webpdemux.framework"); \
    }
#define FUNCTION_LOADER_LIBWEBPMUX(FUNC, SIG)                        \
    lib.FUNC = FUNC;                                                 \
    if (lib.FUNC == NULL) {                                          \
        return SDL_SetError("Missing webpmux.framework: %s", #FUNC); \
    }
#endif

#ifdef __APPLE__
/* Need to turn off optimizations so weak framework load check works */
__attribute__((optnone))
#endif
static bool IMG_InitWEBP(void)
{
    if (lib.loaded == 0) {
#if defined(LOAD_WEBP_DYNAMIC) && defined(LOAD_WEBPDEMUX_DYNAMIC) && defined(LOAD_WEBPMUX_DYNAMIC)
        lib.handle_libwebp = SDL_LoadObject(LOAD_WEBP_DYNAMIC);
        if (lib.handle_libwebp == NULL) {
            return false;
        }
        lib.handle_libwebpdemux = SDL_LoadObject(LOAD_WEBPDEMUX_DYNAMIC);
        if (lib.handle_libwebpdemux == NULL) {
            return false;
        }
        lib.handle_libwebpmux = SDL_LoadObject(LOAD_WEBPMUX_DYNAMIC);
        if (lib.handle_libwebpmux == NULL) {
            return false;
        }
#endif
        FUNCTION_LOADER_LIBWEBP(WebPGetFeaturesInternal, VP8StatusCode(*)(const uint8_t *data, size_t data_size, WebPBitstreamFeatures *features, int decoder_abi_version))
        FUNCTION_LOADER_LIBWEBP(WebPDecodeRGBInto, uint8_t *(*)(const uint8_t *data, size_t data_size, uint8_t *output_buffer, size_t output_buffer_size, int output_stride))
        FUNCTION_LOADER_LIBWEBP(WebPDecodeRGBAInto, uint8_t *(*)(const uint8_t *data, size_t data_size, uint8_t *output_buffer, size_t output_buffer_size, int output_stride))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxInternal, WebPDemuxer * (*)(const WebPData *, int, WebPDemuxState *, int))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxGetFrame, int (*)(const WebPDemuxer *dmux, int frame_number, WebPIterator *iter))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxNextFrame, int (*)(WebPIterator *iter))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxReleaseIterator, void (*)(WebPIterator *iter))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxGetI, uint32_t (*)(const WebPDemuxer *dmux, WebPFormatFeature feature))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxDelete, void (*)(WebPDemuxer *dmux))

        // Encoding frame functions
        FUNCTION_LOADER_LIBWEBP(WebPConfigInitInternal, int (*)(WebPConfig *, WebPPreset, float, int))
        FUNCTION_LOADER_LIBWEBP(WebPValidateConfig, int (*)(const WebPConfig *))
        FUNCTION_LOADER_LIBWEBP(WebPPictureInitInternal, int (*)(WebPPicture *, int))
        FUNCTION_LOADER_LIBWEBP(WebPEncode, int (*)(const WebPConfig *, WebPPicture *))
        FUNCTION_LOADER_LIBWEBP(WebPPictureFree, void (*)(WebPPicture *))
        FUNCTION_LOADER_LIBWEBP(WebPPictureImportRGBA, int (*)(WebPPicture *, const uint8_t *, int))

        FUNCTION_LOADER_LIBWEBP(WebPMemoryWriterInit, void (*)(WebPMemoryWriter *))
        FUNCTION_LOADER_LIBWEBP(WebPMemoryWrite, int (*)(const uint8_t *, size_t, const WebPPicture *))
        FUNCTION_LOADER_LIBWEBP(WebPMemoryWriterClear, void (*)(WebPMemoryWriter *))

        // Free function required for cleanup after muxing.
        FUNCTION_LOADER_LIBWEBP(WebPFree, void (*)(void *))

        // Muxing functions
        FUNCTION_LOADER_LIBWEBPMUX(WebPAnimEncoderNewInternal, WebPAnimEncoder * (*)(int, int, const WebPAnimEncoderOptions *, int))
        FUNCTION_LOADER_LIBWEBPMUX(WebPAnimEncoderOptionsInitInternal, int (*)(WebPAnimEncoderOptions *, int))
        FUNCTION_LOADER_LIBWEBPMUX(WebPAnimEncoderAdd, int (*)(WebPAnimEncoder *, WebPPicture *, int, const WebPConfig *))
        FUNCTION_LOADER_LIBWEBPMUX(WebPAnimEncoderAssemble, int (*)(WebPAnimEncoder *, WebPData *))
        FUNCTION_LOADER_LIBWEBPMUX(WebPAnimEncoderDelete, void (*)(WebPAnimEncoder *))
    }
    ++lib.loaded;

    return true;
}
#if 0
void IMG_QuitWEBP(void)
{
    if (lib.loaded == 0) {
        return;
    }
    if (lib.loaded == 1) {
#if defined(LOAD_WEBP_DYNAMIC) && defined(LOAD_WEBPDEMUX_DYNAMIC)
        SDL_UnloadObject(lib.handle_libwebp);
        SDL_UnloadObject(lib.handle_libwebpdemux);
#endif
    }
    --lib.loaded;
}
#endif // 0

static bool webp_getinfo(SDL_IOStream *src, size_t *datasize)
{
    Sint64 start, size;
    bool is_WEBP;
    Uint8 magic[20];

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_WEBP = false;
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic)) {
        if (magic[0] == 'R' &&
            magic[1] == 'I' &&
            magic[2] == 'F' &&
            magic[3] == 'F' &&
            magic[8] == 'W' &&
            magic[9] == 'E' &&
            magic[10] == 'B' &&
            magic[11] == 'P' &&
            magic[12] == 'V' &&
            magic[13] == 'P' &&
            magic[14] == '8' &&
            (magic[15] == ' ' || magic[15] == 'X' || magic[15] == 'L')) {
            is_WEBP = true;
            if (datasize) {
                size = SDL_GetIOSize(src);
                if (size > 0) {
                    *datasize = (size_t)(size - start);
                } else {
                    *datasize = 0;
                }
            }
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_WEBP;
}

/* See if an image is contained in a data source */
bool IMG_isWEBP(SDL_IOStream *src)
{
    return webp_getinfo(src, NULL);
}

SDL_Surface *IMG_LoadWEBP_IO(SDL_IOStream *src)
{
    Sint64 start;
    const char *error = NULL;
    SDL_Surface *surface = NULL;
    Uint32 format;
    WebPBitstreamFeatures features;
    size_t raw_data_size;
    uint8_t *raw_data = NULL;
    uint8_t *ret;

    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }

    start = SDL_TellIO(src);

    if (!IMG_InitWEBP()) {
        goto error;
    }

    raw_data_size = -1;
    if (!webp_getinfo(src, &raw_data_size)) {
        error = "Invalid WEBP";
        goto error;
    }

    raw_data = (uint8_t *)SDL_malloc(raw_data_size);
    if (raw_data == NULL) {
        error = "Failed to allocate enough buffer for WEBP";
        goto error;
    }

    if (SDL_ReadIO(src, raw_data, raw_data_size) != raw_data_size) {
        error = "Failed to read WEBP";
        goto error;
    }

#if 0
    {/* extract size of picture, not interesting since we don't know about alpha channel */
      int width = -1, height = -1;
      if (!WebPGetInfo(raw_data, raw_data_size, &width, &height)) {
          printf("WebPGetInfo has failed\n");
          return NULL;
    } }
#endif

    if (lib.WebPGetFeaturesInternal(raw_data, raw_data_size, &features, WEBP_DECODER_ABI_VERSION) != VP8_STATUS_OK) {
        error = "WebPGetFeatures has failed";
        goto error;
    }

    // Special casing for animated WebP images to extract a single frame.
    if (features.has_animation) {
        if (SDL_SeekIO(src, start, SDL_IO_SEEK_SET) < 0) {
            error = "Failed to seek IO to read animated WebP";
            goto error;
        } else {

            IMG_Animation *animation = IMG_DecodeAsAnimation(src, "webp", 1);
            if (animation && animation->count > 0) {
                SDL_Surface *surf = animation->frames[0];
                if (surf) {
                    ++surf->refcount;
                    if (raw_data) {
                        SDL_free(raw_data);
                    }
                    IMG_FreeAnimation(animation);
                    return surf;
                } else {
                    error = "Failed to load first frame of animated WebP";
                    IMG_FreeAnimation(animation);
                    goto error;
                }
            } else {
                IMG_FreeAnimation(animation);
                error = "Received an animated WebP but the animation data was invalid";
                goto error;
            }
        }
    }

    if (features.has_alpha) {
        format = SDL_PIXELFORMAT_RGBA32;
    } else {
        format = SDL_PIXELFORMAT_RGB24;
    }

    surface = SDL_CreateSurface(features.width, features.height, format);
    if (surface == NULL) {
        error = "Failed to allocate SDL_Surface";
        goto error;
    }

    if (features.has_alpha) {
        ret = lib.WebPDecodeRGBAInto(raw_data, raw_data_size, (uint8_t *)surface->pixels, surface->pitch * surface->h, surface->pitch);
    } else {
        ret = lib.WebPDecodeRGBInto(raw_data, raw_data_size, (uint8_t *)surface->pixels, surface->pitch * surface->h, surface->pitch);
    }

    if (!ret) {
        error = "Failed to decode WEBP";
        goto error;
    }

    if (raw_data) {
        SDL_free(raw_data);
    }

    return surface;

error:
    if (raw_data) {
        SDL_free(raw_data);
    }

    if (surface) {
        SDL_DestroySurface(surface);
    }

    if (error) {
        SDL_SetError("%s", error);
    }

    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return NULL;
}

IMG_Animation *IMG_LoadWEBPAnimation_IO(SDL_IOStream *src)
{
    return IMG_DecodeAsAnimation(src, "webp", 0);
}

struct IMG_AnimationDecoderStreamContext
{
    WebPDemuxer *demuxer;
    WebPIterator iter;
    SDL_Surface *canvas;
    WebPMuxAnimDispose dispose_method;
    uint32_t bgcolor;
    uint8_t *raw_data;
    size_t raw_data_size;
    WebPDemuxState demux_state;
    Sint64 last_pts;
};

static bool IMG_AnimationDecoderStreamReset_Internal(IMG_AnimationDecoderStream *stream)
{
    lib.WebPDemuxReleaseIterator(&stream->ctx->iter);
    SDL_zero(stream->ctx->iter);
    stream->ctx->dispose_method = WEBP_MUX_DISPOSE_BACKGROUND;

    return lib.WebPDemuxGetFrame(stream->ctx->demuxer, 1, &stream->ctx->iter);
}

static bool IMG_AnimationDecoderStreamGetFrames_Internal(IMG_AnimationDecoderStream *stream, int framesToLoad, IMG_AnimationDecoderFrames *animationFrames)
{
    int totalFrames = stream->ctx->iter.num_frames;
    int availableFrames = totalFrames - (stream->ctx->iter.frame_num - 1);

    if (framesToLoad < 1)
        framesToLoad = availableFrames;
    else
        framesToLoad = SDL_min(availableFrames, framesToLoad);

    animationFrames->frames = (SDL_Surface **)SDL_calloc(framesToLoad, sizeof(SDL_Surface *));
    animationFrames->delays = (Sint64 *)SDL_calloc(framesToLoad, sizeof(Sint64));
    animationFrames->count = 0;
    animationFrames->w = stream->ctx->canvas->w;
    animationFrames->h = stream->ctx->canvas->h;

    SDL_Surface *canvas = stream->ctx->canvas;
    uint32_t bgcolor = stream->ctx->bgcolor;
    WebPMuxAnimDispose dispose_method = stream->ctx->dispose_method;
    WebPIterator *iter = &stream->ctx->iter;
    int decoded_frames = 0;

    do {
        if (decoded_frames >= framesToLoad) {
            break;
        }

        if (decoded_frames == 0 || dispose_method == WEBP_MUX_DISPOSE_BACKGROUND) {
            SDL_FillSurfaceRect(canvas, NULL, bgcolor);
        }

        SDL_Surface *curr = SDL_CreateSurface(iter->width, iter->height, SDL_PIXELFORMAT_RGBA32);
        if (!curr) {
            break;
        }

        if (!lib.WebPDecodeRGBAInto(iter->fragment.bytes, iter->fragment.size,
                                    (uint8_t *)curr->pixels, curr->pitch * curr->h, curr->pitch)) {
            SDL_DestroySurface(curr);
            break;
        }

        SDL_Rect dst = { iter->x_offset, iter->y_offset, iter->width, iter->height };
        if (iter->blend_method == WEBP_MUX_BLEND) {
            if (!SDL_SetSurfaceBlendMode(curr, SDL_BLENDMODE_BLEND)) {
                SDL_DestroySurface(curr);
                SDL_SetError("Failed to set blend mode for WEBP frame");
                break;
            }
        } else {
            if (!SDL_SetSurfaceBlendMode(curr, SDL_BLENDMODE_NONE)) {
                SDL_DestroySurface(curr);
                SDL_SetError("Failed to set blend mode for WEBP frame");
                break;
            }
        }
        if (!SDL_BlitSurface(curr, NULL, canvas, &dst)) {
            SDL_DestroySurface(curr);
            SDL_SetError("Failed to blit WEBP frame to canvas");
            break;
        }
        SDL_DestroySurface(curr);

        animationFrames->frames[decoded_frames] = SDL_DuplicateSurface(canvas);
        if (!animationFrames->frames[decoded_frames]) {
            break;
        }

        if (stream->ctx->iter.frame_num == 1 || stream->timebase_denominator == 0) {
            animationFrames->delays[decoded_frames] = 0;
            stream->ctx->last_pts = 0;
        } else {
            animationFrames->delays[decoded_frames] = stream->ctx->last_pts += iter->duration * stream->timebase_denominator / (1000 * stream->timebase_numerator);
        }
        decoded_frames++;

        dispose_method = iter->dispose_method;
    } while (lib.WebPDemuxNextFrame(iter));

    stream->ctx->dispose_method = dispose_method;

    animationFrames->count = decoded_frames;

    if (decoded_frames > 0 && decoded_frames < framesToLoad) {
        SDL_Surface **new_frames = (SDL_Surface **)SDL_realloc(animationFrames->frames,
                                                               decoded_frames * sizeof(SDL_Surface *));
        Sint64 *new_delays = (Sint64 *)SDL_realloc(animationFrames->delays,
                                                   decoded_frames * sizeof(Sint64));
        if (new_frames) {
            animationFrames->frames = new_frames;
        }
        if (new_delays) {
            animationFrames->delays = new_delays;
        }
    }

    return decoded_frames > 0;
}

static bool IMG_AnimationDecoderStreamClose_Internal(IMG_AnimationDecoderStream *stream)
{
    if (!stream->ctx) {
        return false;
    }

    if (stream->ctx->canvas) {
        SDL_DestroySurface(stream->ctx->canvas);
    }
    if (stream->ctx->demuxer) {
        lib.WebPDemuxDelete(stream->ctx->demuxer);
    }
    if (stream->ctx->raw_data) {
        SDL_free(stream->ctx->raw_data);
    }
    lib.WebPDemuxReleaseIterator(&stream->ctx->iter);
    SDL_free(stream->ctx);
    stream->ctx = NULL;
    return true;
}

bool IMG_CreateWEBPAnimationDecoderStream(IMG_AnimationDecoderStream *stream, SDL_PropertiesID decoderProps)
{
    (void)decoderProps;

    if (!IMG_InitWEBP()) {
        SDL_SetError("Failed to initialize WEBP library");
        return false;
    }

    if (!webp_getinfo(stream->src, NULL)) {
        SDL_SetError("Not a valid WebP file: %s", SDL_GetError());
        return false;
    }

    Sint64 stream_size = SDL_GetIOSize(stream->src);
    if (stream_size <= 0) {
        SDL_SetError("Stream has no data (size: %lld)", stream_size);
        return false;
    }

    stream->ctx = (IMG_AnimationDecoderStreamContext *)SDL_calloc(1, sizeof(IMG_AnimationDecoderStreamContext));
    if (!stream->ctx) {
        SDL_SetError("Out of memory");
        return false;
    }

    stream->ctx->raw_data_size = (size_t)stream_size;
    stream->ctx->raw_data = (uint8_t *)SDL_malloc(stream->ctx->raw_data_size);
    if (!stream->ctx->raw_data) {
        SDL_free(stream->ctx);
        stream->ctx = NULL;
        SDL_SetError("Out of memory for WebP file buffer");
        return false;
    }
    if (SDL_SeekIO(stream->src, stream->start, SDL_IO_SEEK_SET) < 0 ||
        SDL_ReadIO(stream->src, stream->ctx->raw_data, stream->ctx->raw_data_size) != stream->ctx->raw_data_size) {
        SDL_free(stream->ctx->raw_data);
        SDL_free(stream->ctx);
        stream->ctx = NULL;
        SDL_SetError("Failed to read WebP file into memory");
        return false;
    }

    WebPData wd = { stream->ctx->raw_data, stream->ctx->raw_data_size };
    stream->ctx->demuxer = lib.WebPDemuxInternal(&wd, 0, NULL, WEBP_DEMUX_ABI_VERSION);
    if (!stream->ctx->demuxer) {
        SDL_free(stream->ctx->raw_data);
        SDL_free(stream->ctx);
        stream->ctx = NULL;
        SDL_SetError("WebPDemux failed to initialize demuxer (not a valid WebP file or corrupted data)");
        return false;
    }

    uint32_t width = lib.WebPDemuxGetI(stream->ctx->demuxer, WEBP_FF_CANVAS_WIDTH);
    uint32_t height = lib.WebPDemuxGetI(stream->ctx->demuxer, WEBP_FF_CANVAS_HEIGHT);
    uint32_t flags = lib.WebPDemuxGetI(stream->ctx->demuxer, WEBP_FF_FORMAT_FLAGS);
    bool has_alpha = (flags & 0x10) != 0;

    stream->ctx->canvas = SDL_CreateSurface(width, height, has_alpha ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGBX32);
    if (!stream->ctx->canvas) {
        lib.WebPDemuxDelete(stream->ctx->demuxer);
        SDL_free(stream->ctx->raw_data);
        SDL_free(stream->ctx);
        stream->ctx = NULL;
        return false;
    }

    uint32_t bgcolor = lib.WebPDemuxGetI(stream->ctx->demuxer, WEBP_FF_BACKGROUND_COLOR);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    stream->ctx->bgcolor = SDL_MapSurfaceRGBA(stream->ctx->canvas,
                                              (bgcolor >> 8) & 0xFF,
                                              (bgcolor >> 16) & 0xFF,
                                              (bgcolor >> 24) & 0xFF,
                                              (bgcolor >> 0) & 0xFF);
#else
    stream->ctx->bgcolor = SDL_MapSurfaceRGBA(stream->ctx->canvas,
                                              (bgcolor >> 16) & 0xFF,
                                              (bgcolor >> 8) & 0xFF,
                                              (bgcolor >> 0) & 0xFF,
                                              (bgcolor >> 24) & 0xFF);
#endif

    stream->ctx->dispose_method = WEBP_MUX_DISPOSE_BACKGROUND;

    SDL_zero(stream->ctx->iter);
    lib.WebPDemuxGetFrame(stream->ctx->demuxer, 1, &stream->ctx->iter);

    stream->GetFrames = IMG_AnimationDecoderStreamGetFrames_Internal;
    stream->Reset = IMG_AnimationDecoderStreamReset_Internal;
    stream->Close = IMG_AnimationDecoderStreamClose_Internal;

    return true;
}

#endif // LOAD_WEBP

#if SAVE_WEBP

static const char *GetWebPEncodingErrorStringInternal(WebPEncodingError error_code)
{
    switch (error_code) {
    case VP8_ENC_OK:
        return "OK";
    case VP8_ENC_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY:
        return "Bitstream out of memory";
    case VP8_ENC_ERROR_NULL_PARAMETER:
        return "Null parameter";
    case VP8_ENC_ERROR_INVALID_CONFIGURATION:
        return "Invalid configuration";
    case VP8_ENC_ERROR_BAD_DIMENSION:
        return "Bad dimension";
    case VP8_ENC_ERROR_PARTITION0_OVERFLOW:
        return "Partition 0 overflow";
    case VP8_ENC_ERROR_PARTITION_OVERFLOW:
        return "Partition overflow";
    case VP8_ENC_ERROR_BAD_WRITE:
        return "Bad write";
    case VP8_ENC_ERROR_FILE_TOO_BIG:
        return "File too big";
    case VP8_ENC_ERROR_USER_ABORT:
        return "User abort";
    default:
        return "Unknown WebP encoding error";
    }
}

bool IMG_SaveWEBP_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio, float quality)
{
    WebPConfig config;
    WebPPicture pic;
    WebPMemoryWriter writer;
    SDL_Surface *converted_surface = NULL;
    bool ret = true;
    const char *error = NULL;
    bool pic_initialized = false;
    bool memorywriter_initialized = false;
    bool converted_surface_locked = false;
    Sint64 start = -1;

    if (!surface || !dst) {
        error = "Invalid input surface or destination stream.";
        goto cleanup;
    }

    start = SDL_TellIO(dst);

    if (!IMG_InitWEBP()) {
        error = SDL_GetError();
        goto cleanup;
    }

    if (!lib.WebPConfigInitInternal(&config, WEBP_PRESET_DEFAULT, quality, WEBP_ENCODER_ABI_VERSION)) {
        error = "Failed to initialize WebPConfig.";
        goto cleanup;
    }

    quality = SDL_clamp(quality, 0.0f, 100.0f);

    config.lossless = quality == 100.0f;
    config.quality = quality;

    // TODO: Take a look if the method 4 fits here for us.
    config.method = 4;

    if (!lib.WebPValidateConfig(&config)) {
        error = "Invalid WebP configuration.";
        goto cleanup;
    }

    if (!lib.WebPPictureInitInternal(&pic, WEBP_ENCODER_ABI_VERSION)) {
        error = "Failed to initialize WebPPicture.";
        goto cleanup;
    }
    pic_initialized = true;

    pic.width = surface->w;
    pic.height = surface->h;

    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (!converted_surface) {
            error = SDL_GetError();
            goto cleanup;
        }
    } else {
        converted_surface = surface;
    }

    if (SDL_MUSTLOCK(converted_surface)) {
        if (!SDL_LockSurface(converted_surface)) {
            error = SDL_GetError();
            goto cleanup;
        }
        converted_surface_locked = true;
    }

    if (!lib.WebPPictureImportRGBA(&pic, (const uint8_t *)converted_surface->pixels, converted_surface->pitch)) {
        error = "Failed to import RGBA pixels into WebPPicture.";
        goto cleanup;
    }

    if (converted_surface_locked) {
        SDL_UnlockSurface(converted_surface);
        converted_surface_locked = false;
    }

    lib.WebPMemoryWriterInit(&writer);
    memorywriter_initialized = true;
    pic.writer = (WebPWriterFunction)lib.WebPMemoryWrite;
    pic.custom_ptr = &writer;

    if (!lib.WebPEncode(&config, &pic)) {
        error = GetWebPEncodingErrorStringInternal(pic.error_code);
        goto cleanup;
    }

    if (writer.size > 0) {
        if (SDL_WriteIO(dst, writer.mem, writer.size) != writer.size) {
            error = "Failed to write all WebP data to destination.";
            goto cleanup;
        }
    } else {
        error = "No WebP data generated.";
        goto cleanup;
    }

cleanup:
    if (converted_surface_locked) {
        SDL_UnlockSurface(converted_surface);
    }

    if (converted_surface && converted_surface != surface) {
        SDL_DestroySurface(converted_surface);
    }

    if (pic_initialized) {
        lib.WebPPictureFree(&pic);
    }

    if (memorywriter_initialized) {
        lib.WebPMemoryWriterClear(&writer);
    }

    if (error) {
        if (!closeio && start != -1) {
            SDL_SeekIO(dst, start, SDL_IO_SEEK_SET);
        }
        SDL_SetError("%s", error);
        ret = false;
    }

    if (closeio) {
        SDL_CloseIO(dst);
    }

    return ret;
}

bool IMG_SaveWEBP(SDL_Surface *surface, const char *file, float quality)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (!dst) {
        return false;
    }

   return IMG_SaveWEBP_IO(surface, dst, true, quality);
}

struct IMG_AnimationEncoderStreamContext
{
    WebPAnimEncoder *encoder;
    WebPConfig config;
    int frames;
};

static bool IMG_AddWEBPAnimationFrame(IMG_AnimationEncoderStream *stream, SDL_Surface *surface, Uint64 pts)
{
    IMG_AnimationEncoderStreamContext *ctx = stream->ctx;
    const char *error = NULL;
    WebPPicture pic;
    bool pic_initialized = false;
    SDL_Surface *converted = NULL;

    if (!ctx->encoder) {
        ctx->encoder = lib.WebPAnimEncoderNewInternal(surface->w, surface->h, NULL, WEBP_MUX_ABI_VERSION);
        if (!ctx->encoder) {
            return SDL_SetError("WebPAnimEncoderNew() failed");
        }
    }

    if (!lib.WebPPictureInitInternal(&pic, WEBP_ENCODER_ABI_VERSION)) {
        error = "WebPPictureInit() failed";
        goto done;
    }
    pic.use_argb = ctx->config.lossless;
    pic.width = surface->w;
    pic.height = surface->h;
    pic_initialized = true;

    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (!converted) {
            error = SDL_GetError();
            goto done;
        }
        surface = converted;
    }

    if (!lib.WebPPictureImportRGBA(&pic, (const uint8_t *)surface->pixels, surface->pitch)) {
        error = "WebPPictureImportRGBA() failed";
        goto done;
    }

    int timestamp = GetStreamPresentationTimestampMS(stream, pts);
    if (!lib.WebPAnimEncoderAdd(ctx->encoder, &pic, timestamp, &ctx->config)) {
        error = GetWebPEncodingErrorStringInternal(pic.error_code);
        goto done;
    }
    ++ctx->frames;

done:
    if (pic_initialized) {
        lib.WebPPictureFree(&pic);
    }
    if (converted) {
        SDL_DestroySurface(converted);
    }
    if (error) {
        SDL_SetError("%s", error);
        return false;
    }
    return true;
}

static bool IMG_CloseWEBPAnimationStream(IMG_AnimationEncoderStream *stream)
{
    IMG_AnimationEncoderStreamContext *ctx = stream->ctx;
    const char *error = NULL;
    WebPData data = { NULL, 0 };

    if (!ctx->encoder) {
        error = "No frames added to animation";
        goto done;
    }

    int timestamp = GetStreamPresentationTimestampMS(stream, stream->last_pts);
    if (ctx->frames > 1) {
        timestamp += (timestamp / (ctx->frames - 1));
    }
    if (!lib.WebPAnimEncoderAdd(ctx->encoder, NULL, timestamp, &ctx->config)) {
        error = "WebPAnimEncoderAdd() failed";
        goto done;
    }

    if (!lib.WebPAnimEncoderAssemble(ctx->encoder, &data)) {
        error = "WebPAnimEncoderAssemble() failed";
        goto done;
    }

    if (SDL_WriteIO(stream->dst, data.bytes, data.size) != data.size) {
        error = SDL_GetError();
        goto done;
    }

done:
    if (data.bytes) {
        lib.WebPFree((void *)data.bytes);
    }
    if (ctx->encoder) {
        lib.WebPAnimEncoderDelete(ctx->encoder);
    }
    SDL_free(ctx);

    if (error) {
        SDL_SetError("%s", error);
        return false;
    }
    return true;
}

bool IMG_CreateWEBPAnimationEncoderStream(IMG_AnimationEncoderStream *stream, SDL_PropertiesID props)
{
    (void)props;

    if (!IMG_InitWEBP()) {
        return false;
    }

    IMG_AnimationEncoderStreamContext *ctx = (IMG_AnimationEncoderStreamContext *)SDL_calloc(1, sizeof(*stream->ctx));
    if (!ctx) {
        return false;
    }
    stream->ctx = ctx;
    stream->AddFrame = IMG_AddWEBPAnimationFrame;
    stream->Close = IMG_CloseWEBPAnimationStream;

    float quality;
    if (stream->quality < 0) {
        quality = 75.0f;
    } else {
        quality = (float)SDL_clamp(stream->quality, 0, 100);
    }
    if (!lib.WebPConfigInitInternal(&ctx->config, WEBP_PRESET_DEFAULT, quality, WEBP_ENCODER_ABI_VERSION)) {
        SDL_free(ctx);
        return SDL_SetError("WebPConfigInit() failed");
    }
    ctx->config.lossless = (quality == 100.0f);
    ctx->config.quality = quality;
    ctx->config.method = 4;

    if (!lib.WebPValidateConfig(&ctx->config)) {
        SDL_free(ctx);
        return SDL_SetError("WebPValidateConfig() failed");
    }
    return true;
}

#endif // SAVE_WEBP

#if !LOAD_WEBP

/* See if an image is contained in a data source */
bool IMG_isWEBP(SDL_IOStream *src)
{
    (void)src;
    return false;
}

/* Load a WEBP type image from an SDL datasource */
SDL_Surface *IMG_LoadWEBP_IO(SDL_IOStream *src)
{
    (void)src;
    SDL_SetError("SDL_image was not built with WEBP support");
    return NULL;
}

IMG_Animation *IMG_LoadWEBPAnimation_IO(SDL_IOStream *src)
{
    (void)src;
    SDL_SetError("SDL_image was not built with WEBP support");
    return NULL;
}

bool IMG_CreateWEBPAnimationDecoderStream(IMG_AnimationDecoderStream *stream, SDL_PropertiesID decoderProps)
{
    (void)stream;
    (void)decoderProps;
    return SDL_SetError("SDL_image was not built with WEBP load support");
}

#endif // !LOAD_WEBP

#if !SAVE_WEBP

bool IMG_SaveWEBP_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio, float quality)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    (void)quality;
    return SDL_SetError("SDL_image was not built with WEBP save support");
}

bool IMG_SaveWEBP(SDL_Surface *surface, const char *file, float quality)
{
    (void)surface;
    (void)file;
    (void)quality;
    return SDL_SetError("SDL_image was not built with WEBP save support");
}

bool IMG_CreateWEBPAnimationEncoderStream(IMG_AnimationStream *stream, SDL_PropertiesID props)
{
    (void)stream;
    (void)props;
    return SDL_SetError("SDL_image was not built with WEBP save support");
}

#endif // !SAVE_WEBP
