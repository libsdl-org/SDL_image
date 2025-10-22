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

#include "IMG_webp.h"
#include "IMG_anim_encoder.h"
#include "IMG_anim_decoder.h"
#include "xmlman.h"

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

    // Used for extracting EXIF & XMP chunks.
    int (*WebPDemuxGetChunk)(const WebPDemuxer* dmux, const char fourcc[4], int chunk_number, WebPChunkIterator* iter);
    void (*WebPDemuxReleaseChunkIterator)(WebPChunkIterator* iter);

    // Used for setting EXIF & XMP chunks and for loop count.
    WebPMux *(*WebPMuxCreateInternal)(const WebPData*, int, int); // Export 38 (0x0026),  (0x), 0x00006e90, None
    void (*WebPMuxDelete)(WebPMux* mux);
    WebPMuxError (*WebPMuxSetChunk)(WebPMux *mux, const char fourcc[4], const WebPData *chunk_data, int copy_data);
    WebPMuxError (*WebPMuxGetAnimationParams)(const WebPMux *mux, WebPMuxAnimParams *params);
    WebPMuxError (*WebPMuxSetAnimationParams)(WebPMux* mux, const WebPMuxAnimParams* params);

    WebPMuxError (*WebPMuxAssemble)(WebPMux* mux, WebPData* assembled_data);
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

        // Used for extracting EXIF & XMP chunks.
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxGetChunk, int (*)(const WebPDemuxer *dmux, const char fourcc[4], int chunk_number, WebPChunkIterator *iter))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxReleaseChunkIterator, void (*)(WebPChunkIterator* iter))

        // Used for setting EXIF & XMP chunks and for loop count.
        FUNCTION_LOADER_LIBWEBPMUX(WebPMuxCreateInternal, WebPMux * (*)(const WebPData*, int, int))
        FUNCTION_LOADER_LIBWEBPMUX(WebPMuxDelete, void (*)(WebPMux* mux))
        FUNCTION_LOADER_LIBWEBPMUX(WebPMuxSetChunk, WebPMuxError (*)(WebPMux *mux, const char fourcc[4], const WebPData *chunk_data, int copy_data))
        FUNCTION_LOADER_LIBWEBPMUX(WebPMuxGetAnimationParams, WebPMuxError (*)(const WebPMux* mux, WebPMuxAnimParams* params))
        FUNCTION_LOADER_LIBWEBPMUX(WebPMuxSetAnimationParams, WebPMuxError (*)(WebPMux* mux, const WebPMuxAnimParams* params))
        FUNCTION_LOADER_LIBWEBPMUX(WebPMuxAssemble, WebPMuxError (*)(WebPMux* mux, WebPData* assembled_data))
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

struct IMG_AnimationDecoderContext
{
    WebPDemuxer *demuxer;
    WebPIterator iter;
    SDL_Surface *canvas;
    WebPMuxAnimDispose dispose_method;
    uint32_t bgcolor;
    uint8_t *raw_data;
    size_t raw_data_size;
    WebPDemuxState demux_state;
};

static bool IMG_AnimationDecoderReset_Internal(IMG_AnimationDecoder *decoder)
{
    lib.WebPDemuxReleaseIterator(&decoder->ctx->iter);
    SDL_zero(decoder->ctx->iter);
    decoder->ctx->dispose_method = WEBP_MUX_DISPOSE_BACKGROUND;

    return true;
}

static bool IMG_AnimationDecoderGetNextFrame_Internal(IMG_AnimationDecoder *decoder, SDL_Surface **frame, Uint64 *duration)
{
    // Get the next frame from the demuxer.
    if (decoder->ctx->iter.frame_num < 1) {
        if (!lib.WebPDemuxGetFrame(decoder->ctx->demuxer, 1, &decoder->ctx->iter)) {
            return SDL_SetError("Failed to get first frame from WEBP demuxer");
        }
    } else {
        if (!lib.WebPDemuxNextFrame(&decoder->ctx->iter)) {
            decoder->status = IMG_DECODER_STATUS_COMPLETE;
            return false;
        }
    }

    int totalFrames = decoder->ctx->iter.num_frames;
    int availableFrames = totalFrames - (decoder->ctx->iter.frame_num - 1);

    if (availableFrames < 1) {
        decoder->status = IMG_DECODER_STATUS_COMPLETE;
        return false;
    }

    SDL_Surface *canvas = decoder->ctx->canvas;
    uint32_t bgcolor = decoder->ctx->bgcolor;
    WebPMuxAnimDispose dispose_method = decoder->ctx->dispose_method;
    WebPIterator *iter = &decoder->ctx->iter;
    SDL_Surface *retval = NULL;

    if (totalFrames == availableFrames || dispose_method == WEBP_MUX_DISPOSE_BACKGROUND) {
        SDL_FillSurfaceRect(canvas, NULL, iter->has_alpha ? 0 : bgcolor);
    }

    SDL_Surface *curr = SDL_CreateSurface(iter->width, iter->height, SDL_PIXELFORMAT_RGBA32);
    if (!curr) {
        return SDL_SetError("Failed to create surface for the frame");
    }

    if (!lib.WebPDecodeRGBAInto(iter->fragment.bytes, iter->fragment.size,
                                (uint8_t *)curr->pixels, curr->pitch * curr->h, curr->pitch)) {
        SDL_DestroySurface(curr);
        return SDL_SetError("Failed to decode frame");
    }

    SDL_Rect dst = { iter->x_offset, iter->y_offset, iter->width, iter->height };
    if (iter->blend_method == WEBP_MUX_BLEND) {
        if (!SDL_SetSurfaceBlendMode(curr, SDL_BLENDMODE_BLEND)) {
            SDL_DestroySurface(curr);
            return SDL_SetError("Failed to set blend mode for WEBP frame");
        }
    } else {
        if (!SDL_SetSurfaceBlendMode(curr, SDL_BLENDMODE_NONE)) {
            SDL_DestroySurface(curr);
            return SDL_SetError("Failed to set blend mode for WEBP frame");
        }
    }
    if (!SDL_BlitSurface(curr, NULL, canvas, &dst)) {
        SDL_DestroySurface(curr);
        return SDL_SetError("Failed to blit WEBP frame to canvas");
    }
    SDL_DestroySurface(curr);

    retval = SDL_DuplicateSurface(canvas);
    if (!retval) {
        return SDL_SetError("Failed to duplicate the surface for the next frame");
    }

    *duration = IMG_GetDecoderDuration(decoder, iter->duration, 1000);

    dispose_method = iter->dispose_method;
    decoder->ctx->dispose_method = dispose_method;

    *frame = retval;
    return true;
}

static bool IMG_AnimationDecoderClose_Internal(IMG_AnimationDecoder *decoder)
{
    if (!decoder->ctx) {
        return false;
    }

    if (decoder->ctx->canvas) {
        SDL_DestroySurface(decoder->ctx->canvas);
    }
    if (decoder->ctx->demuxer) {
        lib.WebPDemuxDelete(decoder->ctx->demuxer);
    }
    if (decoder->ctx->raw_data) {
        SDL_free(decoder->ctx->raw_data);
    }
    lib.WebPDemuxReleaseIterator(&decoder->ctx->iter);
    SDL_free(decoder->ctx);
    decoder->ctx = NULL;
    return true;
}

bool IMG_CreateWEBPAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    if (!IMG_InitWEBP()) {
        SDL_SetError("Failed to initialize WEBP library");
        return false;
    }

    if (!webp_getinfo(decoder->src, NULL)) {
        SDL_SetError("Not a valid WebP file: %s", SDL_GetError());
        return false;
    }

    Sint64 stream_size = SDL_GetIOSize(decoder->src);
    if (stream_size <= 0) {
        SDL_SetError("Stream has no data (size: %" SDL_PRIs64 ")", stream_size);
        return false;
    }

    decoder->ctx = (IMG_AnimationDecoderContext *)SDL_calloc(1, sizeof(IMG_AnimationDecoderContext));
    if (!decoder->ctx) {
        SDL_SetError("Out of memory");
        return false;
    }

    decoder->ctx->raw_data_size = (size_t)stream_size;
    decoder->ctx->raw_data = (uint8_t *)SDL_malloc(decoder->ctx->raw_data_size);
    if (!decoder->ctx->raw_data) {
        SDL_free(decoder->ctx);
        decoder->ctx = NULL;
        SDL_SetError("Out of memory for WebP file buffer");
        return false;
    }
    if (SDL_SeekIO(decoder->src, decoder->start, SDL_IO_SEEK_SET) < 0 ||
        SDL_ReadIO(decoder->src, decoder->ctx->raw_data, decoder->ctx->raw_data_size) != decoder->ctx->raw_data_size) {
        SDL_free(decoder->ctx->raw_data);
        SDL_free(decoder->ctx);
        decoder->ctx = NULL;
        SDL_SetError("Failed to read WebP file into memory");
        return false;
    }

    WebPData wd = { decoder->ctx->raw_data, decoder->ctx->raw_data_size };
    decoder->ctx->demuxer = lib.WebPDemuxInternal(&wd, 0, NULL, WEBP_DEMUX_ABI_VERSION);
    if (!decoder->ctx->demuxer) {
        SDL_free(decoder->ctx->raw_data);
        SDL_free(decoder->ctx);
        decoder->ctx = NULL;
        SDL_SetError("WebPDemux failed to initialize demuxer (not a valid WebP file or corrupted data)");
        return false;
    }

    uint32_t width = lib.WebPDemuxGetI(decoder->ctx->demuxer, WEBP_FF_CANVAS_WIDTH);
    uint32_t height = lib.WebPDemuxGetI(decoder->ctx->demuxer, WEBP_FF_CANVAS_HEIGHT);
    uint32_t flags = lib.WebPDemuxGetI(decoder->ctx->demuxer, WEBP_FF_FORMAT_FLAGS);

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    if (!ignoreProps) {
        // Allow implicit properties to be set which are not globalized but specific to the decoder.
        SDL_SetNumberProperty(decoder->props, "IMG_PROP_METADATA_FRAME_COUNT_NUMBER", lib.WebPDemuxGetI(decoder->ctx->demuxer, WEBP_FF_FRAME_COUNT));

        // Set well-defined properties.
        SDL_SetNumberProperty(decoder->props, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, lib.WebPDemuxGetI(decoder->ctx->demuxer, WEBP_FF_LOOP_COUNT));

        // Get other well-defined properties and set them in our props.
        WebPChunkIterator xmp_iter;
        if (lib.WebPDemuxGetChunk(decoder->ctx->demuxer, "XMP ", 1, &xmp_iter)) {
            if (xmp_iter.chunk.bytes && xmp_iter.chunk.size > 0) {
                char *desc = __xmlman_GetXMPDescription(xmp_iter.chunk.bytes, xmp_iter.chunk.size);
                char *rights = __xmlman_GetXMPCopyright(xmp_iter.chunk.bytes, xmp_iter.chunk.size);
                char *title = __xmlman_GetXMPTitle(xmp_iter.chunk.bytes, xmp_iter.chunk.size);
                char *creator = __xmlman_GetXMPCreator(xmp_iter.chunk.bytes, xmp_iter.chunk.size);
                char *createdate = __xmlman_GetXMPCreateDate(xmp_iter.chunk.bytes, xmp_iter.chunk.size);
                if (desc) {
                    SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_DESCRIPTION_STRING, desc);
                    SDL_free(desc);
                }
                if (rights) {
                    SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_COPYRIGHT_STRING, rights);
                    SDL_free(rights);
                }
                if (title) {
                    SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_TITLE_STRING, title);
                    SDL_free(title);
                }
                if (creator) {
                    SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_AUTHOR_STRING, creator);
                    SDL_free(creator);
                }
                if (createdate) {
                    SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_CREATION_TIME_STRING, createdate);
                    SDL_free(createdate);
                }
            }
            lib.WebPDemuxReleaseChunkIterator(&xmp_iter);
        }
    }

    bool has_alpha = (flags & 0x10) != 0;

    decoder->ctx->canvas = SDL_CreateSurface(width, height, has_alpha ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGBX32);
    if (!decoder->ctx->canvas) {
        lib.WebPDemuxDelete(decoder->ctx->demuxer);
        SDL_free(decoder->ctx->raw_data);
        SDL_free(decoder->ctx);
        decoder->ctx = NULL;
        return false;
    }

    uint32_t bgcolor = lib.WebPDemuxGetI(decoder->ctx->demuxer, WEBP_FF_BACKGROUND_COLOR);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    decoder->ctx->bgcolor = SDL_MapSurfaceRGBA(decoder->ctx->canvas,
                                              (bgcolor >> 8) & 0xFF,
                                              (bgcolor >> 16) & 0xFF,
                                              (bgcolor >> 24) & 0xFF,
                                              (bgcolor >> 0) & 0xFF);
#else
    decoder->ctx->bgcolor = SDL_MapSurfaceRGBA(decoder->ctx->canvas,
                                              (bgcolor >> 16) & 0xFF,
                                              (bgcolor >> 8) & 0xFF,
                                              (bgcolor >> 0) & 0xFF,
                                              (bgcolor >> 24) & 0xFF);
#endif

    decoder->ctx->dispose_method = WEBP_MUX_DISPOSE_BACKGROUND;

    SDL_zero(decoder->ctx->iter);

    decoder->GetNextFrame = IMG_AnimationDecoderGetNextFrame_Internal;
    decoder->Reset = IMG_AnimationDecoderReset_Internal;
    decoder->Close = IMG_AnimationDecoderClose_Internal;

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
    bool result = false;
    bool pic_initialized = false;
    bool memorywriter_initialized = false;
    bool converted_surface_locked = false;
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

    if (!IMG_InitWEBP()) {
        goto done;
    }

    if (!lib.WebPConfigInitInternal(&config, WEBP_PRESET_DEFAULT, quality, WEBP_ENCODER_ABI_VERSION)) {
        SDL_SetError("Failed to initialize WebPConfig");
        goto done;
    }

    quality = SDL_clamp(quality, 0.0f, 100.0f);

    config.lossless = quality == 100.0f;
    config.quality = quality;

    // TODO: Take a look if the method 4 fits here for us.
    config.method = 4;

    if (!lib.WebPValidateConfig(&config)) {
        SDL_SetError("Invalid WebP configuration");
        goto done;
    }

    if (!lib.WebPPictureInitInternal(&pic, WEBP_ENCODER_ABI_VERSION)) {
        SDL_SetError("Failed to initialize WebPPicture");
        goto done;
    }
    pic_initialized = true;

    pic.width = surface->w;
    pic.height = surface->h;

    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (!converted_surface) {
            goto done;
        }
    } else {
        converted_surface = surface;
    }

    if (SDL_MUSTLOCK(converted_surface)) {
        if (!SDL_LockSurface(converted_surface)) {
            goto done;
        }
        converted_surface_locked = true;
    }

    if (!lib.WebPPictureImportRGBA(&pic, (const uint8_t *)converted_surface->pixels, converted_surface->pitch)) {
        SDL_SetError("Failed to import RGBA pixels into WebPPicture");
        goto done;
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
        SDL_SetError("Failed to encode WebP: %s", GetWebPEncodingErrorStringInternal(pic.error_code));
        goto done;
    }

    if (writer.size > 0) {
        if (SDL_WriteIO(dst, writer.mem, writer.size) != writer.size) {
            goto done;
        }
    } else {
        SDL_SetError("No WebP data generated.");
        goto done;
    }

    result = true;

done:
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

    if (!result && !closeio && start != -1) {
        SDL_SeekIO(dst, start, SDL_IO_SEEK_SET);
    }

    if (closeio) {
        result &= SDL_CloseIO(dst);
    }

    return result;
}

bool IMG_SaveWEBP(SDL_Surface *surface, const char *file, float quality)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return IMG_SaveWEBP_IO(surface, dst, true, quality);
    } else {
        return false;
    }
}

struct IMG_AnimationEncoderContext
{
    WebPAnimEncoder *encoder;
    WebPConfig config;
    int timestamp;
    SDL_PropertiesID metadata;
};

static bool IMG_AddWEBPAnimationFrame(IMG_AnimationEncoder *encoder, SDL_Surface *surface, Uint64 duration)
{
    IMG_AnimationEncoderContext *ctx = encoder->ctx;
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

    if (!lib.WebPAnimEncoderAdd(ctx->encoder, &pic, ctx->timestamp, &ctx->config)) {
        error = GetWebPEncodingErrorStringInternal(pic.error_code);
        goto done;
    }
    ctx->timestamp += (int)IMG_GetEncoderDuration(encoder, duration, 1000);

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

static bool IMG_CloseWEBPAnimation(IMG_AnimationEncoder *encoder)
{
    IMG_AnimationEncoderContext *ctx = encoder->ctx;
    const char *error = NULL;
    WebPData data = { NULL, 0 };
    WebPMux *mux = NULL;

    if (!ctx->encoder) {
        error = "No frames added to animation";
        goto done;
    }

    if (!lib.WebPAnimEncoderAdd(ctx->encoder, NULL, ctx->timestamp, &ctx->config)) {
        error = "WebPAnimEncoderAdd() failed";
        goto done;
    }

    if (!IMG_HasMetadata(ctx->metadata)) {
        if (!lib.WebPAnimEncoderAssemble(ctx->encoder, &data)) {
            error = "WebPAnimEncoderAssemble() failed";
            goto done;
        }
    } else {
        WebPMuxError muxErr;
        WebPData pdata = { NULL, 0 };
        if (!lib.WebPAnimEncoderAssemble(ctx->encoder, &pdata)) {
            error = "WebPAnimEncoderAssemble() failed";
            goto done;
        }

        if (!pdata.bytes || pdata.size < 1) {
            error = "WebPAnimEncoderAssemble() returned invalid data";
            goto done;
        }

        mux = lib.WebPMuxCreateInternal(&pdata, 1, WEBP_MUX_ABI_VERSION);
        if (!mux) {
            error = "WebPMuxCreateInternal() failed. This usually happens if you tried to encode a single frame only.";
            goto done;
        }

        WebPMuxAnimParams params;
        muxErr = lib.WebPMuxGetAnimationParams(mux, &params);
        if (muxErr != WEBP_MUX_OK) {
            error = "WebPMuxGetAnimationParams() failed";
            goto done;
        }
        params.loop_count = (int)SDL_max(SDL_GetNumberProperty(ctx->metadata, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, 0), 0);
        muxErr = lib.WebPMuxSetAnimationParams(mux, &params);
        if (muxErr != WEBP_MUX_OK) {
            error = "WebPMuxSetAnimationParams() failed";
            goto done;
        }

        size_t siz = 0;
        uint8_t *d = __xmlman_ConstructXMPWithRDFDescription(SDL_GetStringProperty(ctx->metadata, IMG_PROP_METADATA_TITLE_STRING, NULL), SDL_GetStringProperty(ctx->metadata, IMG_PROP_METADATA_AUTHOR_STRING, NULL), SDL_GetStringProperty(ctx->metadata, IMG_PROP_METADATA_DESCRIPTION_STRING, NULL), SDL_GetStringProperty(ctx->metadata, IMG_PROP_METADATA_COPYRIGHT_STRING, NULL), SDL_GetStringProperty(ctx->metadata, IMG_PROP_METADATA_CREATION_TIME_STRING, NULL), &siz);
        if (siz < 1 || !d) {
            error = "Failed to construct XMP data";
            goto done;
        }
        WebPData xmp_data = { d, siz };
        muxErr = lib.WebPMuxSetChunk(mux, "XMP ", &xmp_data, 1);
        if (muxErr != WEBP_MUX_OK) {
            SDL_free(d);
            error = "WebPMuxSetChunk() failed for XMP data";
            goto done;
        }

        SDL_free(d);

        muxErr = lib.WebPMuxAssemble(mux, &data);
        if (muxErr != WEBP_MUX_OK) {
            error = "WebPMuxAssemble() failed";
            goto done;
        }
    }

    if (SDL_WriteIO(encoder->dst, data.bytes, data.size) != data.size) {
        error = SDL_GetError();
        goto done;
    }

done:
    if (ctx->metadata) {
        SDL_DestroyProperties(ctx->metadata);
        ctx->metadata = 0;
    }

    if (data.bytes) {
        lib.WebPFree((void *)data.bytes);
    }
    if (mux) {
        lib.WebPMuxDelete(mux);
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

bool IMG_CreateWEBPAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    if (!IMG_InitWEBP()) {
        return false;
    }

    IMG_AnimationEncoderContext *ctx = (IMG_AnimationEncoderContext *)SDL_calloc(1, sizeof(*encoder->ctx));
    if (!ctx) {
        return false;
    }
    encoder->ctx = ctx;
    encoder->AddFrame = IMG_AddWEBPAnimationFrame;
    encoder->Close = IMG_CloseWEBPAnimation;

    float quality;
    if (encoder->quality < 0) {
        quality = 75.0f;
    } else {
        quality = (float)SDL_clamp(encoder->quality, 0, 100);
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

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    if (!ignoreProps) {
        ctx->metadata = SDL_CreateProperties();
        if (!ctx->metadata) {
            SDL_free(ctx);
            return SDL_SetError("Failed to create properties for WebP metadata");
        }

        if (!SDL_CopyProperties(props, ctx->metadata)) {
            SDL_DestroyProperties(ctx->metadata);
            ctx->metadata = 0;
            SDL_free(ctx);
            return SDL_SetError("Failed to copy properties for WebP metadata");
        }
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
    SDL_SetError("SDL_image built without WEBP support");
    return NULL;
}

bool IMG_CreateWEBPAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    (void)decoder;
    (void)props;
    return SDL_SetError("SDL_image built without WEBP support");
}

#endif // !LOAD_WEBP

#if !SAVE_WEBP

bool IMG_SaveWEBP_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio, float quality)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    (void)quality;
    return SDL_SetError("SDL_image built without WEBP save support");
}

bool IMG_SaveWEBP(SDL_Surface *surface, const char *file, float quality)
{
    (void)surface;
    (void)file;
    (void)quality;
    return SDL_SetError("SDL_image built without WEBP save support");
}

bool IMG_CreateWEBPAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    (void)encoder;
    (void)props;
    return SDL_SetError("SDL_image built without WEBP save support");
}

#endif // !SAVE_WEBP
