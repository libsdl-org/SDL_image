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

#ifdef LOAD_WEBP

/*=============================================================================
        File: SDL_webp.c
     Purpose: A WEBP loader for the SDL library
    Revision:
  Created by: Michael Bonfils (Murlock) (26 November 2011)
              murlock42@gmail.com

=============================================================================*/

#include <SDL3/SDL_endian.h>

#ifdef macintosh
#define MACOS
#endif
#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>

static struct {
    int loaded;
    void *handle_libwebpdemux;
    void *handle_libwebp;
    VP8StatusCode (*WebPGetFeaturesInternal) (const uint8_t *data, size_t data_size, WebPBitstreamFeatures* features, int decoder_abi_version);
    uint8_t* (*WebPDecodeRGBInto) (const uint8_t* data, size_t data_size, uint8_t* output_buffer, size_t output_buffer_size, int output_stride);
    uint8_t* (*WebPDecodeRGBAInto) (const uint8_t* data, size_t data_size, uint8_t* output_buffer, size_t output_buffer_size, int output_stride);
    WebPDemuxer* (*WebPDemuxInternal)(const WebPData* data, int allow_partial, WebPDemuxState* state, int version);
    int (*WebPDemuxGetFrame)(const WebPDemuxer *dmux, int frame_number, WebPIterator *iter);
    int (*WebPDemuxNextFrame)(WebPIterator *iter);
    void (*WebPDemuxReleaseIterator)(WebPIterator *iter);
    uint32_t (*WebPDemuxGetI)(const WebPDemuxer* dmux, WebPFormatFeature feature);
    void (*WebPDemuxDelete)(WebPDemuxer* dmux);

    // Encoding functions
    int (*WebPConfigInitInternal)(WebPConfig *, WebPPreset, float, int);
    int (*WebPValidateConfig)(const WebPConfig *);
    int (*WebPPictureInitInternal)(WebPPicture *, int);
    int (*WebPEncode)(const WebPConfig *, WebPPicture *);
    void (*WebPPictureFree)(WebPPicture *);
    int (*WebPPictureImportRGBA)(WebPPicture *, const uint8_t *, int);
    void (*WebPMemoryWriterInit)(WebPMemoryWriter *);
    int (*WebPMemoryWrite)(const uint8_t *, size_t, const WebPPicture *);
    void (*WebPMemoryWriterClear)(WebPMemoryWriter *);
} lib;

#if defined(LOAD_WEBP_DYNAMIC) && defined(LOAD_WEBPDEMUX_DYNAMIC)
#define FUNCTION_LOADER_LIBWEBP(FUNC, SIG) \
    lib.FUNC = (SIG) SDL_LoadFunction(lib.handle_libwebp, #FUNC); \
    if (lib.FUNC == NULL) { SDL_UnloadObject(lib.handle_libwebp); return false; }
#define FUNCTION_LOADER_LIBWEBPDEMUX(FUNC, SIG) \
    lib.FUNC = (SIG) SDL_LoadFunction(lib.handle_libwebpdemux, #FUNC); \
    if (lib.FUNC == NULL) { SDL_UnloadObject(lib.handle_libwebpdemux); return false; }
#else
#define FUNCTION_LOADER_LIBWEBP(FUNC, SIG) \
    lib.FUNC = FUNC; \
    if (lib.FUNC == NULL) { return SDL_SetError("Missing webp.framework"); }
#define FUNCTION_LOADER_LIBWEBPDEMUX(FUNC, SIG) \
    lib.FUNC = FUNC; \
    if (lib.FUNC == NULL) { return SDL_SetError("Missing webpdemux.framework"); }
#endif

#ifdef __APPLE__
    /* Need to turn off optimizations so weak framework load check works */
    __attribute__ ((optnone))
#endif
static bool IMG_InitWEBP(void)
{
    if (lib.loaded == 0) {
#if defined(LOAD_WEBP_DYNAMIC) && defined(LOAD_WEBPDEMUX_DYNAMIC)
        lib.handle_libwebpdemux = SDL_LoadObject(LOAD_WEBPDEMUX_DYNAMIC);
        if (lib.handle_libwebpdemux == NULL) {
            return false;
        }
        lib.handle_libwebp = SDL_LoadObject(LOAD_WEBP_DYNAMIC);
        if (lib.handle_libwebp == NULL) {
            return false;
        }
#endif
        FUNCTION_LOADER_LIBWEBP(WebPGetFeaturesInternal, VP8StatusCode (*) (const uint8_t *data, size_t data_size, WebPBitstreamFeatures* features, int decoder_abi_version))
        FUNCTION_LOADER_LIBWEBP(WebPDecodeRGBInto, uint8_t * (*) (const uint8_t* data, size_t data_size, uint8_t* output_buffer, size_t output_buffer_size, int output_stride))
        FUNCTION_LOADER_LIBWEBP(WebPDecodeRGBAInto, uint8_t * (*) (const uint8_t* data, size_t data_size, uint8_t* output_buffer, size_t output_buffer_size, int output_stride))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxInternal, WebPDemuxer* (*)(const WebPData*, int, WebPDemuxState*, int))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxGetFrame, int (*)(const WebPDemuxer *dmux, int frame_number, WebPIterator *iter))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxNextFrame, int (*)(WebPIterator *iter))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxReleaseIterator, void (*)(WebPIterator *iter))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxGetI, uint32_t (*)(const WebPDemuxer* dmux, WebPFormatFeature feature))
        FUNCTION_LOADER_LIBWEBPDEMUX(WebPDemuxDelete, void (*)(WebPDemuxer* dmux))

        // Encoding functions
        FUNCTION_LOADER_LIBWEBP(WebPConfigInitInternal, int (*)(WebPConfig *, WebPPreset, float, int))
        FUNCTION_LOADER_LIBWEBP(WebPValidateConfig, int (*)(const WebPConfig *))
        FUNCTION_LOADER_LIBWEBP(WebPPictureInitInternal, int (*)(WebPPicture *, int))
        FUNCTION_LOADER_LIBWEBP(WebPEncode, int (*)(const WebPConfig *, WebPPicture *))
        FUNCTION_LOADER_LIBWEBP(WebPPictureFree, void (*)(WebPPicture *))
        FUNCTION_LOADER_LIBWEBP(WebPPictureImportRGBA, int (*)(WebPPicture *, const uint8_t *, int))

        FUNCTION_LOADER_LIBWEBP(WebPMemoryWriterInit, void (*)(WebPMemoryWriter *))
        FUNCTION_LOADER_LIBWEBP(WebPMemoryWrite, int (*)(const uint8_t*, size_t, const WebPPicture*)) 
        FUNCTION_LOADER_LIBWEBP(WebPMemoryWriterClear, void (*)(WebPMemoryWriter *))
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
        if (magic[ 0] == 'R' &&
            magic[ 1] == 'I' &&
            magic[ 2] == 'F' &&
            magic[ 3] == 'F' &&
            magic[ 8] == 'W' &&
            magic[ 9] == 'E' &&
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

static IMG_Animation *IMG_LoadWEBPAnimation_IO_Internal(SDL_IOStream *src, int maxFrames);

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
            IMG_Animation *animation = IMG_LoadWEBPAnimation_IO_Internal(src, 1);
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

static IMG_Animation *IMG_LoadWEBPAnimation_IO_Internal(SDL_IOStream *src, int maxFrames)
{
    Sint64 start;
    const char *error = NULL;
    WebPBitstreamFeatures features;
    struct WebPDemuxer *demuxer = NULL;
    WebPIterator iter;
    IMG_Animation *anim = NULL;
    size_t raw_data_size;
    uint8_t *raw_data = NULL;
    WebPData wd;
    uint32_t bgcolor;
    SDL_Surface *canvas = NULL;
    WebPMuxAnimDispose dispose_method = WEBP_MUX_DISPOSE_BACKGROUND;

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
        error = "Invalid WEBP Animation";
        goto error;
    }

    raw_data = (uint8_t *)SDL_malloc(raw_data_size);
    if (raw_data == NULL) {
        goto error;
    }

    if (SDL_ReadIO(src, raw_data, raw_data_size) != raw_data_size) {
        goto error;
    }

    if (lib.WebPGetFeaturesInternal(raw_data, raw_data_size, &features, WEBP_DECODER_ABI_VERSION) != VP8_STATUS_OK) {
        error = "WebPGetFeatures() failed";
        goto error;
    }

    wd.size = raw_data_size;
    wd.bytes = raw_data;
    demuxer = lib.WebPDemuxInternal(&wd, 0, NULL, WEBP_DEMUX_ABI_VERSION);
    if (!demuxer) {
        error = "WebPDemux() failed";
        goto error;
    }

    anim = (IMG_Animation *)SDL_calloc(1, sizeof(*anim));
    if (!anim) {
        goto error;
    }
    anim->w = features.width;
    anim->h = features.height;
    uint32_t fc = lib.WebPDemuxGetI(demuxer, WEBP_FF_FRAME_COUNT);
    anim->count = maxFrames > 0 ? SDL_min((unsigned)maxFrames, fc) : fc;
    anim->frames = (SDL_Surface **)SDL_calloc(anim->count, sizeof(*anim->frames));
    anim->delays = (int *)SDL_calloc(anim->count, sizeof(*anim->delays));
    if (!anim->frames || !anim->delays) {
        goto error;
    }

    canvas = SDL_CreateSurface(anim->w, anim->h, features.has_alpha ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGBX32);
    if (!canvas) {
        goto error;
    }

    /* Background color is BGRA byte order according to the spec */
    bgcolor = lib.WebPDemuxGetI(demuxer, WEBP_FF_BACKGROUND_COLOR);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    bgcolor = SDL_MapSurfaceRGBA(canvas,
                                 (bgcolor >> 8) & 0xFF,
                                 (bgcolor >> 16) & 0xFF,
                                 (bgcolor >> 24) & 0xFF,
                                 (bgcolor >> 0) & 0xFF);
#else
    bgcolor = SDL_MapSurfaceRGBA(canvas,
                                 (bgcolor >> 16) & 0xFF,
                                 (bgcolor >> 8) & 0xFF,
                                 (bgcolor >> 0) & 0xFF,
                                 (bgcolor >> 24) & 0xFF);
#endif

    SDL_zero(iter);

    if (lib.WebPDemuxGetFrame(demuxer, 1, &iter)) {
        do {
            int frame_idx = (iter.frame_num - 1);
            if (frame_idx < 0 || frame_idx >= anim->count) {
                continue;
            }

            if (dispose_method == WEBP_MUX_DISPOSE_BACKGROUND) {
                SDL_FillSurfaceRect(canvas, NULL, bgcolor);
            }

            SDL_Surface *curr = SDL_CreateSurface(iter.width, iter.height, SDL_PIXELFORMAT_RGBA32);
            if (!curr) {
                goto error;
            }

            if (!lib.WebPDecodeRGBAInto(iter.fragment.bytes,
                                        iter.fragment.size,
                                        (uint8_t *)curr->pixels,
                                        curr->pitch * curr->h,
                                        curr->pitch)) {
                error = "WebPDecodeRGBAInto() failed";
                SDL_DestroySurface(curr);
                goto error;
            }

            SDL_Rect dst = { iter.x_offset, iter.y_offset, iter.width, iter.height };
            if (iter.blend_method == WEBP_MUX_BLEND) {
                SDL_SetSurfaceBlendMode(curr, SDL_BLENDMODE_BLEND);
            } else {
                SDL_SetSurfaceBlendMode(curr, SDL_BLENDMODE_NONE);
            }
            SDL_BlitSurface(curr, NULL, canvas, &dst);
            SDL_DestroySurface(curr);

            anim->frames[frame_idx] = SDL_DuplicateSurface(canvas);
            anim->delays[frame_idx] = iter.duration;
            dispose_method = iter.dispose_method;

        } while (lib.WebPDemuxNextFrame(&iter));

        lib.WebPDemuxReleaseIterator(&iter);
    }

    SDL_DestroySurface(canvas);

    lib.WebPDemuxDelete(demuxer);

    SDL_free(raw_data);

    return anim;

error:
    if (canvas) {
        SDL_DestroySurface(canvas);
    }
    if (anim) {
        IMG_FreeAnimation(anim);
    }
    if (demuxer) {
        lib.WebPDemuxDelete(demuxer);
    }
    if (raw_data) {
        SDL_free(raw_data);
    }

    if (error) {
        SDL_SetError("%s", error);
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return NULL;
}

IMG_Animation *IMG_LoadWEBPAnimation_IO(SDL_IOStream *src)
{
    return IMG_LoadWEBPAnimation_IO_Internal(src, 0);
}


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

#else

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
    return NULL;
}

IMG_Animation *IMG_LoadWEBPAnimation_IO(SDL_IOStream *src)
{
    (void)src;
    return NULL;
}

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

#endif /* LOAD_WEBP */
