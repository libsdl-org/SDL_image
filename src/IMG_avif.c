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

/* This is a AVIF image file loading framework */

#include <SDL3_image/SDL_image.h>

#include "IMG_avif.h"
#include "IMG_anim_encoder.h"
#include "IMG_anim_decoder.h"
#include "xmlman.h"

/* We'll have AVIF save support by default */
#if !defined(SAVE_AVIF)
#define SAVE_AVIF 1
#endif

#ifdef LOAD_AVIF

#include <avif/avif.h>


/*
 * - `SDL_PROP_SURFACE_MAXCLL_NUMBER`: MaxCLL (Maximum Content Light Level)
 *   indicates the maximum light level of any single pixel (in cd/m2 or nits)
 *   of the content. MaxCLL is usually measured off the final delivered
 *   content after mastering. If one uses the full light level of the HDR
 *   mastering display and adds a hard clip at its maximum value, MaxCLL would
 *   be equal to the peak luminance of the mastering monitor.
 * - `SDL_PROP_SURFACE_MAXFALL_NUMBER`: MaxFALL (Maximum Frame Average Light
 *   Level) indicates the maximum value of the frame average light level (in
 *   cd/m2 or nits) of the content. MaxFALL is calculated by averaging the
 *   decoded luminance values of all the pixels within a frame. MaxFALL is
 *   usually much lower than MaxCLL.
 */
#define SDL_PROP_SURFACE_MAXCLL_NUMBER                      "SDL.surface.maxCLL"
#define SDL_PROP_SURFACE_MAXFALL_NUMBER                     "SDL.surface.maxFALL"


static struct {
    int loaded;
    void *handle;
    avifDecoder * (*avifDecoderCreate)(void);
    void (*avifDecoderDestroy)(avifDecoder * decoder);
    avifResult (*avifDecoderNextImage)(avifDecoder * decoder);
    avifResult (*avifDecoderParse)(avifDecoder * decoder);
    void (*avifDecoderSetIO)(avifDecoder * decoder, avifIO * io);
    avifResult (*avifEncoderAddImage)(avifEncoder * encoder, const avifImage * image, uint64_t durationInTimescales, avifAddImageFlags addImageFlags);
    avifEncoder * (*avifEncoderCreate)(void);
    void (*avifEncoderDestroy)(avifEncoder * encoder);
    avifResult (*avifEncoderFinish)(avifEncoder * encoder, avifRWData * output);
    avifImage * (*avifImageCreate)(uint32_t width, uint32_t height, uint32_t depth, avifPixelFormat yuvFormat);
    void (*avifImageDestroy)(avifImage * image);
    avifResult (*avifImageRGBToYUV)(avifImage * image, const avifRGBImage * rgb);
    avifResult (*avifImageYUVToRGB)(const avifImage * image, avifRGBImage * rgb);
    avifBool (*avifPeekCompatibleFileType)(const avifROData * input);
    void (*avifRGBImageSetDefaults)(avifRGBImage * rgb, const avifImage * image);
    void (*avifRWDataFree)(avifRWData * raw);
    const char * (*avifResultToString)(avifResult res);

    // XMP metadata support
    avifResult (*avifImageSetMetadataXMP)(avifImage * image, const uint8_t * xmp, size_t xmpSize);
} lib;

#ifdef LOAD_AVIF_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = (SIG) SDL_LoadFunction(lib.handle, #FUNC); \
    if (lib.FUNC == NULL) { SDL_UnloadObject(lib.handle); return false; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = FUNC; \
    if (lib.FUNC == NULL) { return SDL_SetError("Missing avif.framework"); }
#endif

#ifdef __APPLE__
    /* Need to turn off optimizations so weak framework load check works */
    __attribute__ ((optnone))
#endif
static bool IMG_InitAVIF(void)
{
    if ( lib.loaded == 0 ) {
#ifdef LOAD_AVIF_DYNAMIC
        lib.handle = SDL_LoadObject(LOAD_AVIF_DYNAMIC);
        if ( lib.handle == NULL ) {
            return false;
        }
#endif
        FUNCTION_LOADER(avifDecoderCreate, avifDecoder * (*)(void))
        FUNCTION_LOADER(avifDecoderDestroy, void (*)(avifDecoder * decoder))
        FUNCTION_LOADER(avifDecoderNextImage, avifResult (*)(avifDecoder * decoder))
        FUNCTION_LOADER(avifDecoderParse, avifResult (*)(avifDecoder * decoder))
        FUNCTION_LOADER(avifDecoderSetIO, void (*)(avifDecoder * decoder, avifIO * io))
        FUNCTION_LOADER(avifEncoderAddImage, avifResult (*)(avifEncoder * encoder, const avifImage * image, uint64_t durationInTimescales, avifAddImageFlags addImageFlags))
        FUNCTION_LOADER(avifEncoderCreate, avifEncoder * (*)(void))
        FUNCTION_LOADER(avifEncoderDestroy, void (*)(avifEncoder * encoder))
        FUNCTION_LOADER(avifEncoderFinish, avifResult (*)(avifEncoder * encoder, avifRWData * output))
        FUNCTION_LOADER(avifImageCreate, avifImage * (*)(uint32_t width, uint32_t height, uint32_t depth, avifPixelFormat yuvFormat))
        FUNCTION_LOADER(avifImageDestroy, void (*)(avifImage * image))
        FUNCTION_LOADER(avifImageRGBToYUV, avifResult (*)(avifImage * image, const avifRGBImage * rgb))
        FUNCTION_LOADER(avifImageYUVToRGB, avifResult (*)(const avifImage * image, avifRGBImage * rgb))
        FUNCTION_LOADER(avifPeekCompatibleFileType, avifBool (*)(const avifROData * input))
        FUNCTION_LOADER(avifRGBImageSetDefaults, void (*)(avifRGBImage * rgb, const avifImage * image))
        FUNCTION_LOADER(avifRWDataFree, void (*)(avifRWData * raw))
        FUNCTION_LOADER(avifResultToString, const char * (*)(avifResult res))

        // XMP metadata support
        FUNCTION_LOADER(avifImageSetMetadataXMP, avifResult (*)(avifImage * image, const uint8_t * xmp, size_t xmpSize))
    }
    ++lib.loaded;

    return true;
}
#if 0
void IMG_QuitAVIF(void)
{
    if ( lib.loaded == 0 ) {
        return;
    }
    if ( lib.loaded == 1 ) {
#ifdef LOAD_AVIF_DYNAMIC
        SDL_UnloadObject(lib.handle);
#endif
    }
    --lib.loaded;
}
#endif // 0

static bool ReadAVIFHeader(SDL_IOStream *src, Uint8 **header_data, size_t *header_size)
{
    Uint8 magic[16];
    Uint64 size;
    Uint64 read = 0;
    Uint8 *data;

    *header_data = NULL;
    *header_size = 0;

    if (SDL_ReadIO(src, magic, 8) != 8) {
        return false;
    }
    read += 8;

    if (SDL_memcmp(&magic[4], "ftyp", 4) != 0) {
        return false;
    }

    size = (((Uint64)magic[0] << 24) |
            ((Uint64)magic[1] << 16) |
            ((Uint64)magic[2] << 8)  |
            ((Uint64)magic[3] << 0));
    if (size == 1) {
        /* 64-bit header size */
        if (SDL_ReadIO(src, &magic[8], 8) != 8) {
            return false;
        }
        read += 8;

        size = (((Uint64)magic[8] << 56)  |
                ((Uint64)magic[9] << 48)  |
                ((Uint64)magic[10] << 40) |
                ((Uint64)magic[11] << 32) |
                ((Uint64)magic[12] << 24) |
                ((Uint64)magic[13] << 16) |
                ((Uint64)magic[14] << 8)  |
                ((Uint64)magic[15] << 0));
    }

    if (size > SDL_SIZE_MAX) {
        return false;
    }
    if (size <= read) {
        return false;
    }

    /* Read in the header */
    data = (Uint8 *)SDL_malloc((size_t)size);
    if (!data) {
        return false;
    }
    SDL_memcpy(data, magic, (size_t)read);

    if (SDL_ReadIO(src, &data[read], (size_t)(size - read)) != (size_t)(size - read)) {
        SDL_free(data);
        return false;
    }
    *header_data = data;
    *header_size = (size_t)size;
    return true;
}

/* See if an image is contained in a data source */
bool IMG_isAVIF(SDL_IOStream *src)
{
    Sint64 start;
    bool is_AVIF;
    Uint8 *data;
    size_t size;

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_AVIF = false;
    if (ReadAVIFHeader(src, &data, &size)) {
        /* This might be AVIF, do more thorough checks */
        if (IMG_InitAVIF()) {
            avifROData header;

            header.data = data;
            header.size = size;
            is_AVIF = lib.avifPeekCompatibleFileType(&header);
        }
        SDL_free(data);
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_AVIF;
}

/* Context for AFIF I/O operations */
typedef struct
{
    SDL_IOStream *src;
    Uint64 start;
    uint8_t *data;
    Sint64 size;
} avifIOContext;

static avifResult ReadAVIFIO(struct avifIO * io, uint32_t readFlags, uint64_t offset, size_t size, avifROData * out)
{
    avifIOContext *context = (avifIOContext *)io->data;

    (void) readFlags;   /* not used */

    /* The AVIF reader bounces all over, so always seek to the correct offset */
    if (SDL_SeekIO(context->src, context->start + offset, SDL_IO_SEEK_SET) < 0) {
        return AVIF_RESULT_IO_ERROR;
    }

    if (size > (Uint64)context->size) {
        uint8_t *data = (uint8_t *)SDL_realloc(context->data, size);
        if (!data) {
            return AVIF_RESULT_IO_ERROR;
        }
        context->data = data;
        context->size = (Sint64)size;
    }

    out->data = context->data;
    out->size = SDL_ReadIO(context->src, context->data, size);
    if (out->size == 0) {
        if (SDL_GetIOStatus(context->src) == SDL_IO_STATUS_NOT_READY) {
            return AVIF_RESULT_WAITING_ON_IO;
        } else {
            return AVIF_RESULT_IO_ERROR;
        }
    }

    return AVIF_RESULT_OK;
}

static void DestroyAVIFIO(struct avifIO * io)
{
    avifIOContext *context = (avifIOContext *)io->data;

    if (context->data) {
        SDL_free(context->data);
        context->data = NULL;
    }
}

static int ConvertGBR444toXBGR2101010(avifImage *image, SDL_Surface *surface)
{
    const Uint16 *srcR, *srcG, *srcB;
    int srcskipR, srcskipG, srcskipB;
    int sR, sG, sB;
    Uint32 *dst;
    int dstskip;
    int width, height;

    srcR = (Uint16 *)image->yuvPlanes[2];
    srcskipR = (image->yuvRowBytes[2] - image->width * sizeof(Uint16)) / sizeof(Uint16);
    srcG = (Uint16 *)image->yuvPlanes[0];
    srcskipG = (image->yuvRowBytes[0] - image->width * sizeof(Uint16)) / sizeof(Uint16);
    srcB = (Uint16 *)image->yuvPlanes[1];
    srcskipB = (image->yuvRowBytes[1] - image->width * sizeof(Uint16)) / sizeof(Uint16);
    if (srcskipR < 0 || srcskipG < 0 || srcskipB < 0) {
        return -1;
    }

    dst = (Uint32 *)surface->pixels;
    dstskip = (surface->pitch - image->width * sizeof(Uint32)) / sizeof(Uint32);

    height = image->height;
    while (height--) {
        width = image->width;
        while (width--) {
            sR = *srcR++;
            sG = *srcG++;
            sB = *srcB++;
            sR = SDL_min(sR, 1023);
            sG = SDL_min(sG, 1023);
            sB = SDL_min(sB, 1023);
            *dst = (0x03 << 30) | (sB << 20) | (sG << 10) | sR;
            ++dst;
        }
        srcR += srcskipR;
        srcG += srcskipG;
        srcB += srcskipB;
        dst += dstskip;
    }
    return 0;
}

static void ConvertRGB16toXBGR2101010(avifRGBImage *image, SDL_Surface *surface)
{
    const Uint16 *src;
    Uint32 sR, sG, sB;
    Uint32 *dst;
    int dstskip;
    int width, height;

    src = (Uint16 *)image->pixels;
    dst = (Uint32 *)surface->pixels;
    dstskip = (surface->pitch - image->width * sizeof(Uint32)) / sizeof(Uint32);

    height = image->height;
    while (height--) {
        width = image->width;
        while (width--) {
            sR = *src++;
            sR >>= 6;
            sG = *src++;
            sG >>= 6;
            sB = *src++;
            sB >>= 6;
            *dst = (0x03 << 30) | (sB << 20) | (sG << 10) | sR;
            ++dst;
        }
        dst += dstskip;
    }
}

/* Load a AVIF type image from an SDL datasource */
SDL_Surface *IMG_LoadAVIF_IO(SDL_IOStream *src)
{
    Sint64 start;
    avifDecoder *decoder = NULL;
    avifImage *image;
    avifIO io;
    avifIOContext context;
    avifResult result;
    SDL_Surface *surface = NULL;

    if (!src) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }
    start = SDL_TellIO(src);

    if (!IMG_InitAVIF()) {
        return NULL;
    }

    SDL_zero(context);
    SDL_zero(io);

    decoder = lib.avifDecoderCreate();
    if (!decoder) {
        SDL_SetError("Couldn't create AVIF decoder");
        goto done;
    }

    /* Be permissive so we can load as many images as possible */
    decoder->strictFlags = AVIF_STRICT_DISABLED;

    context.src = src;
    context.start = start;
    io.destroy = DestroyAVIFIO;
    io.read = ReadAVIFIO;
    io.data = &context;
    lib.avifDecoderSetIO(decoder, &io);

    result = lib.avifDecoderParse(decoder);
    if (result != AVIF_RESULT_OK) {
        SDL_SetError("Couldn't parse AVIF image: %s", lib.avifResultToString(result));
        goto done;
    }

    result = lib.avifDecoderNextImage(decoder);
    if (result != AVIF_RESULT_OK) {
        SDL_SetError("Couldn't get AVIF image: %s", lib.avifResultToString(result));
        goto done;
    }

    image = decoder->image;
    if (image->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_SMPTE2084) {
        // This is an HDR PQ image

        if (image->matrixCoefficients == AVIF_MATRIX_COEFFICIENTS_IDENTITY &&
            image->yuvFormat == AVIF_PIXEL_FORMAT_YUV444) {
            // This image uses identity GBR channel ordering
            if (image->depth == 10) {
                surface = SDL_CreateSurface(image->width, image->height, SDL_PIXELFORMAT_XBGR2101010);
                if (surface) {
                    if (ConvertGBR444toXBGR2101010(image, surface) < 0) {
                        // Invalid image, let avif take care of it
                        SDL_free(surface);
                        surface = NULL;
                    }
                }
            }
        }

        if (!surface) {
            avifRGBImage rgb;

            // Convert the YUV image to 10-bit RGB
            SDL_zero(rgb);
            rgb.width = image->width;
            rgb.height = image->height;
            rgb.depth = 16;
            rgb.format = AVIF_RGB_FORMAT_RGB;
            rgb.rowBytes = (uint32_t)image->width * 3 * sizeof(Uint16);
            rgb.pixels = (uint8_t *)SDL_malloc(image->height * rgb.rowBytes);
            if (!rgb.pixels) {
                goto done;
            }
            result = lib.avifImageYUVToRGB(image, &rgb);
            if (result != AVIF_RESULT_OK) {
                SDL_SetError("Couldn't convert AVIF image to RGB: %s", lib.avifResultToString(result));
                SDL_free(rgb.pixels);
                goto done;
            }

            surface = SDL_CreateSurface(image->width, image->height, SDL_PIXELFORMAT_XBGR2101010);
            if (surface) {
                ConvertRGB16toXBGR2101010(&rgb, surface);
            }

            SDL_free(rgb.pixels);
        }

        if (surface) {
            // Set HDR properties

            // The older standards use an SDR white point of 100 nits.
            // ITU-R BT.2408-6 recommends using an SDR white point of 203 nits.
            // This is the default Chrome uses, and what a lot of game content
            // assumes, so we'll go with that.
            const float DEFAULT_PQ_SDR_WHITE_POINT = 203.0f;

            // The official definition is 10000, but PQ game content is often mastered for 400 or 1000 nits
            const uint16_t DEFAULT_PQ_MAXCLL = 1000;
            uint16_t maxCLL = DEFAULT_PQ_MAXCLL;

            SDL_PropertiesID props = SDL_GetSurfaceProperties(surface);
            SDL_Colorspace colorspace = SDL_DEFINE_COLORSPACE(SDL_COLOR_TYPE_RGB,
                                                              SDL_COLOR_RANGE_FULL,
                                                              image->colorPrimaries,
                                                              image->transferCharacteristics,
                                                              SDL_MATRIX_COEFFICIENTS_IDENTITY,
                                                              SDL_CHROMA_LOCATION_NONE);
            SDL_SetSurfaceColorspace(surface, colorspace);
            if (image->clli.maxCLL > 0) {
                maxCLL = image->clli.maxCLL;
                SDL_SetNumberProperty(props, SDL_PROP_SURFACE_MAXCLL_NUMBER, image->clli.maxCLL);
            }
            if (image->clli.maxPALL > 0) {
                SDL_SetNumberProperty(props, SDL_PROP_SURFACE_MAXFALL_NUMBER, image->clli.maxPALL);
            }
            SDL_SetFloatProperty(props, SDL_PROP_SURFACE_SDR_WHITE_POINT_FLOAT, DEFAULT_PQ_SDR_WHITE_POINT);
            SDL_SetFloatProperty(props, SDL_PROP_SURFACE_HDR_HEADROOM_FLOAT, (float)maxCLL / DEFAULT_PQ_SDR_WHITE_POINT);
        }
    }

    if (!surface) {
        avifRGBImage rgb;

        surface = SDL_CreateSurface(image->width, image->height, SDL_PIXELFORMAT_ARGB8888);
        if (!surface) {
            goto done;
        }

        /* Convert the YUV image to RGB */
        SDL_zero(rgb);
        rgb.width = surface->w;
        rgb.height = surface->h;
        rgb.depth = 8;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        rgb.format = AVIF_RGB_FORMAT_BGRA;
#else
        rgb.format = AVIF_RGB_FORMAT_ARGB;
#endif
        rgb.pixels = (uint8_t *)surface->pixels;
        rgb.rowBytes = (uint32_t)surface->pitch;
        result = lib.avifImageYUVToRGB(image, &rgb);
        if (result != AVIF_RESULT_OK) {
            SDL_SetError("Couldn't convert AVIF image to RGB: %s", lib.avifResultToString(result));
            SDL_DestroySurface(surface);
            surface = NULL;
            goto done;
        }
    }

done:
    if (decoder) {
        lib.avifDecoderDestroy(decoder);
    }
    if (!surface) {
        SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    }
    return surface;
}

static bool IMG_SaveAVIF_IO_libavif(SDL_Surface *surface, SDL_IOStream *dst, int quality)
{
    avifImage *image = NULL;
    avifRGBImage rgb;
    avifEncoder *encoder = NULL;
    avifRWData avifOutput = AVIF_DATA_EMPTY;
    avifResult rc;
    SDL_Colorspace colorspace;
    Uint16 maxCLL, maxFALL;
    SDL_PropertiesID props;
    bool result = false;

    if (!IMG_InitAVIF()) {
        return false;
    }

    /* Get the colorspace and light level properties, if any */
    colorspace = SDL_GetSurfaceColorspace(surface);
    props = SDL_GetSurfaceProperties(surface);
    maxCLL = (Uint16)SDL_GetNumberProperty(props, SDL_PROP_SURFACE_MAXCLL_NUMBER, 0);
    maxFALL = (Uint16)SDL_GetNumberProperty(props, SDL_PROP_SURFACE_MAXFALL_NUMBER, 0);

    SDL_zero(rgb);
    image = lib.avifImageCreate(surface->w, surface->h, 10, AVIF_PIXEL_FORMAT_YUV444);
    if (!image) {
        SDL_SetError("Couldn't create AVIF YUV image");
        goto done;
    }
    image->yuvRange = AVIF_RANGE_FULL;
    image->colorPrimaries = (avifColorPrimaries)SDL_COLORSPACEPRIMARIES(colorspace);
    image->transferCharacteristics = (avifTransferCharacteristics)SDL_COLORSPACETRANSFER(colorspace);
    image->clli.maxCLL = maxCLL;
    image->clli.maxPALL = maxFALL;

    lib.avifRGBImageSetDefaults(&rgb, image);

    if (SDL_ISPIXELFORMAT_10BIT(surface->format)) {
        const Uint16 expand_alpha[] = {
            0, 0x155, 0x2aa, 0x3ff
        };
        int width, height;
        Uint16 *dst16;
        Uint32 *src;
        int srcskip;

        /* We are not actually using YUV, but storing raw GBR (yes not RGB) data */
        image->yuvFormat = AVIF_PIXEL_FORMAT_YUV444;
        image->depth = 10;
        image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_IDENTITY;

        if (SDL_PIXELORDER(surface->format) == SDL_PACKEDORDER_XRGB ||
            SDL_PIXELORDER(surface->format) == SDL_PACKEDORDER_ARGB) {
            rgb.format = AVIF_RGB_FORMAT_RGBA;
        } else {
            rgb.format = AVIF_RGB_FORMAT_BGRA;
        }
        rgb.ignoreAlpha = SDL_ISPIXELFORMAT_ALPHA(surface->format) ? false : true;
        rgb.depth = 10;
        rgb.rowBytes = (uint32_t)image->width * 4 * sizeof(Uint16);
        rgb.pixels = (uint8_t *)SDL_malloc(image->height * rgb.rowBytes);
        if (!rgb.pixels) {
            goto done;
        }

        src = (Uint32 *)surface->pixels;
        srcskip = surface->pitch - (surface->w * sizeof(Uint32));
        dst16 = (Uint16 *)rgb.pixels;
        height = image->height;
        while (height--) {
            width = image->width;
            while (width--) {
                Uint32 pixelvalue = *src++;

                *dst16++ = (pixelvalue >> 20) & 0x3FF;
                *dst16++ = (pixelvalue >> 10) & 0x3FF;
                *dst16++ = (pixelvalue >> 0) & 0x3FF;
                *dst16++ = expand_alpha[(pixelvalue >> 30) & 0x3];
            }
            src = (Uint32 *)(((Uint8 *)src) + srcskip);
        }

        rc = lib.avifImageRGBToYUV(image, &rgb);
        if (rc != AVIF_RESULT_OK) {
            SDL_SetError("Couldn't convert to YUV: %s", lib.avifResultToString(rc));
            goto done;
        }

    } else {
        SDL_Surface *temp = NULL;

        rgb.depth = 8;
        switch (surface->format) {
        case SDL_PIXELFORMAT_RGBX32:
        case SDL_PIXELFORMAT_RGBA32:
            rgb.format = AVIF_RGB_FORMAT_RGBA;
            temp = surface;
            break;
        case SDL_PIXELFORMAT_XRGB32:
        case SDL_PIXELFORMAT_ARGB32:
            rgb.format = AVIF_RGB_FORMAT_ARGB;
            temp = surface;
            break;
        case SDL_PIXELFORMAT_BGRX32:
        case SDL_PIXELFORMAT_BGRA32:
            rgb.format = AVIF_RGB_FORMAT_BGRA;
            temp = surface;
            break;
        case SDL_PIXELFORMAT_XBGR32:
        case SDL_PIXELFORMAT_ABGR32:
            rgb.format = AVIF_RGB_FORMAT_ABGR;
            temp = surface;
            break;
        default:
            /* Need to convert to a format libavif understands */
            rgb.format = AVIF_RGB_FORMAT_RGBA;
            if (SDL_ISPIXELFORMAT_ALPHA(surface->format)) {
                temp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            } else {
                temp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBX32);
            }
            if (!temp) {
                goto done;
            }
            break;
        }
        rgb.ignoreAlpha = SDL_ISPIXELFORMAT_ALPHA(surface->format) ? false : true;
        rgb.pixels = (uint8_t *)temp->pixels;
        rgb.rowBytes = (uint32_t)temp->pitch;

        /* Convert to YUV */
        rc = lib.avifImageRGBToYUV(image, &rgb);

        /* Do any cleanup */
        if (temp != surface) {
            SDL_free(temp);
        }
        rgb.pixels = NULL;

        /* Check the result of the conversion */
        if (rc != AVIF_RESULT_OK) {
            SDL_SetError("Couldn't convert to YUV: %s", lib.avifResultToString(rc));
            goto done;
        }
    }

    encoder = lib.avifEncoderCreate();
    encoder->quality = quality;
    encoder->qualityAlpha = AVIF_QUALITY_LOSSLESS;
    encoder->speed = AVIF_SPEED_FASTEST;

    rc = lib.avifEncoderAddImage(encoder, image, 1, AVIF_ADD_IMAGE_FLAG_SINGLE);
    if (rc != AVIF_RESULT_OK) {
        SDL_SetError("Failed to add image to avif encoder: %s", lib.avifResultToString(rc));
        goto done;
    }

    rc = lib.avifEncoderFinish(encoder, &avifOutput);
    if (rc != AVIF_RESULT_OK) {
        SDL_SetError("Failed to finish encoder: %s", lib.avifResultToString(rc));
        goto done;
    }

    if (SDL_WriteIO(dst, avifOutput.data, avifOutput.size) == avifOutput.size) {
        result = true;
    }

done:
    if (rgb.pixels) {
        SDL_free(rgb.pixels);
    }
    if (image) {
        lib.avifImageDestroy(image);
    }
    if (encoder) {
        lib.avifEncoderDestroy(encoder);
    }
    lib.avifRWDataFree(&avifOutput);

    return result;
}

#else

/* We don't have any way to save AVIF files */
#undef SAVE_AVIF

/* See if an image is contained in a data source */
bool IMG_isAVIF(SDL_IOStream *src)
{
    (void)src;
    return false;
}

/* Load a AVIF type image from an SDL datasource */
SDL_Surface *IMG_LoadAVIF_IO(SDL_IOStream *src)
{
    (void)src;
    SDL_SetError("SDL_image built without AVIF support");
    return NULL;
}

#endif /* LOAD_AVIF */

#if SAVE_AVIF

bool IMG_SaveAVIF_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio, int quality)
{
    bool result = false;

    if (!surface) {
        SDL_InvalidParamError("surface");
        goto done;
    }
    if (!dst) {
        SDL_InvalidParamError("dst");
        goto done;
    }

    result = IMG_SaveAVIF_IO_libavif(surface, dst, quality);

done:
    if (closeio) {
        result &= SDL_CloseIO(dst);
    }
    return result;
}

bool IMG_SaveAVIF(SDL_Surface *surface, const char *file, int quality)
{
    SDL_IOStream *dst = SDL_IOFromFile(file, "wb");
    if (dst) {
        return IMG_SaveAVIF_IO(surface, dst, true, quality);
    } else {
        return false;
    }
}

#else

bool IMG_SaveAVIF_IO(SDL_Surface *surface, SDL_IOStream *dst, bool closeio, int quality)
{
    (void)surface;
    (void)dst;
    (void)closeio;
    (void)quality;
    return SDL_SetError("SDL_image built without AVIF save support");
}

bool IMG_SaveAVIF(SDL_Surface *surface, const char *file, int quality)
{
    (void)surface;
    (void)file;
    (void)quality;
    return SDL_SetError("SDL_image built without AVIF save support");
}

#endif // SAVE_AVIF

#ifdef LOAD_AVIF

static void SetHDRProperties(SDL_Surface *surface, const avifImage *image)
{
    // Standard HDR constants
    const float DEFAULT_PQ_SDR_WHITE_POINT = 203.0f;
    const uint16_t DEFAULT_PQ_MAXCLL = 1000;
    uint16_t maxCLL = DEFAULT_PQ_MAXCLL;

    SDL_PropertiesID props = SDL_GetSurfaceProperties(surface);

    // Set colorspace first
    SDL_Colorspace colorspace = SDL_DEFINE_COLORSPACE(SDL_COLOR_TYPE_RGB,
                                                    SDL_COLOR_RANGE_FULL,
                                                    image->colorPrimaries,
                                                    image->transferCharacteristics,
                                                    image->matrixCoefficients,
                                                    SDL_CHROMA_LOCATION_NONE);
    SDL_SetSurfaceColorspace(surface, colorspace);

    // Check if this is an HDR image by transfer function
    bool isHDR = (image->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_SMPTE2084 ||
                 image->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_HLG);

    if (isHDR) {
        // Use metadata if available
        if (image->clli.maxCLL > 0) {
            maxCLL = image->clli.maxCLL;
            SDL_SetNumberProperty(props, SDL_PROP_SURFACE_MAXCLL_NUMBER, image->clli.maxCLL);
        } else {
            // Default values based on common HDR mastering practices
            SDL_SetNumberProperty(props, SDL_PROP_SURFACE_MAXCLL_NUMBER, DEFAULT_PQ_MAXCLL);
        }

        if (image->clli.maxPALL > 0) {
            SDL_SetNumberProperty(props, SDL_PROP_SURFACE_MAXFALL_NUMBER, image->clli.maxPALL);
        } else {
            // A reasonable default for MaxFALL
            SDL_SetNumberProperty(props, SDL_PROP_SURFACE_MAXFALL_NUMBER, DEFAULT_PQ_MAXCLL / 4);
        }

        // HDR properties needed for proper tone mapping
        SDL_SetFloatProperty(props, SDL_PROP_SURFACE_SDR_WHITE_POINT_FLOAT, DEFAULT_PQ_SDR_WHITE_POINT);
        SDL_SetFloatProperty(props, SDL_PROP_SURFACE_HDR_HEADROOM_FLOAT, (float)maxCLL / DEFAULT_PQ_SDR_WHITE_POINT);
    }
}

struct IMG_AnimationDecoderContext
{
    avifDecoder *decoder;             /* AVIF decoder instance */
    avifIO io;                        /* IO context for the decoder */
    avifIOContext ioContext;          /* Context data for IO operations */
    Sint64 start_pos;                 /* Starting position in the stream */

    int current_frame;                /* Current frame index */
    int total_frames;                 /* Total number of frames in the animation */

    int width;                        /* Width of the animation */
    int height;                       /* Height of the animation */
};

static bool IMG_AnimationDecoderReset_Internal(IMG_AnimationDecoder *decoder)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;

    // Reset the decoder
    if (ctx->decoder) {
        lib.avifDecoderDestroy(ctx->decoder);
        ctx->decoder = NULL;
    }

    // Reset stream position
    if (SDL_SeekIO(decoder->src, ctx->start_pos, SDL_IO_SEEK_SET) < 0) {
        return SDL_SetError("Failed to seek to beginning of AVIF file");
    }

    // Recreate the decoder
    ctx->decoder = lib.avifDecoderCreate();
    if (!ctx->decoder) {
        return SDL_SetError("Couldn't create AVIF decoder");
    }

    // Be permissive to decode as many frames as possible
    ctx->decoder->strictFlags = AVIF_STRICT_DISABLED;

    // Set up IO
    if (ctx->ioContext.data) {
        SDL_free(ctx->ioContext.data);
    }

    ctx->ioContext.src = decoder->src;
    ctx->ioContext.start = ctx->start_pos;
    ctx->ioContext.data = NULL;
    ctx->ioContext.size = 0;

    ctx->io.destroy = DestroyAVIFIO;
    ctx->io.read = ReadAVIFIO;
    ctx->io.data = &ctx->ioContext;
    lib.avifDecoderSetIO(ctx->decoder, &ctx->io);

    // Reset state
    ctx->current_frame = 0;

    // Need to re-parse the animation if we reset
    avifResult result = lib.avifDecoderParse(ctx->decoder);
    if (result != AVIF_RESULT_OK) {
        return SDL_SetError("Couldn't re-parse AVIF animation after reset: %s", lib.avifResultToString(result));
    }

    return true;
}

static bool IMG_AnimationDecoderGetNextFrame_Internal(IMG_AnimationDecoder *decoder, SDL_Surface **frame, Uint64 *duration)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;
    avifResult result;

    if (ctx->total_frames - ctx->current_frame < 1) {
        decoder->status = IMG_DECODER_STATUS_COMPLETE;
        return false;
    }

    result = lib.avifDecoderNextImage(ctx->decoder);
    if (result != AVIF_RESULT_OK) {
        if (result == AVIF_RESULT_NO_IMAGES_REMAINING) {
            // This shouldn't happen here, but handle it gracefully
            decoder->status = IMG_DECODER_STATUS_COMPLETE;
            return false;
        }

        return SDL_SetError("Couldn't get AVIF frame %d: %s", ctx->current_frame + 1, lib.avifResultToString(result));
    }

    avifImage *image = ctx->decoder->image;
    SDL_Surface *frame_surface = NULL;

    if (image->depth == 16) {
        // Handle 16-bit depth
        avifRGBImage rgb;
        SDL_zero(rgb);

        rgb.width = image->width;
        rgb.height = image->height;
        rgb.depth = 16;
        rgb.format = AVIF_RGB_FORMAT_RGBA;

        frame_surface = SDL_CreateSurface(image->width, image->height, SDL_PIXELFORMAT_RGBA64);
        if (!frame_surface) {
            return SDL_SetError("Couldn't create 16-bit surface for AVIF frame");
        }

        rgb.pixels = (uint8_t *)frame_surface->pixels;
        rgb.rowBytes = (uint32_t)frame_surface->pitch;
        rgb.ignoreAlpha = false;

        result = lib.avifImageYUVToRGB(image, &rgb);
        if (result != AVIF_RESULT_OK) {
            SDL_DestroySurface(frame_surface);
            return SDL_SetError("Couldn't convert 16-bit AVIF image to RGB: %s", lib.avifResultToString(result));
        }

        // Set HDR properties if needed
        SetHDRProperties(frame_surface, image);
    } else if (image->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_SMPTE2084) {
        // Handle HDR PQ image
        if (image->matrixCoefficients == AVIF_MATRIX_COEFFICIENTS_IDENTITY &&
            image->yuvFormat == AVIF_PIXEL_FORMAT_YUV444) {
            if (image->depth == 10) {
                frame_surface = SDL_CreateSurface(image->width, image->height, SDL_PIXELFORMAT_XBGR2101010);
                if (frame_surface) {
                    if (ConvertGBR444toXBGR2101010(image, frame_surface) < 0) {
                        SDL_DestroySurface(frame_surface);
                        frame_surface = NULL;
                    }
                }
            }
        }

        if (!frame_surface) {
            avifRGBImage rgb;
            SDL_zero(rgb);

            rgb.width = image->width;
            rgb.height = image->height;
            rgb.depth = 16;
            rgb.format = AVIF_RGB_FORMAT_RGB;
            rgb.rowBytes = (uint32_t)image->width * 3 * sizeof(Uint16);
            rgb.pixels = (uint8_t *)SDL_malloc(image->height * rgb.rowBytes);
            if (!rgb.pixels) {
                return SDL_SetError("Out of memory for AVIF RGB pixels");
            }

            result = lib.avifImageYUVToRGB(image, &rgb);
            if (result != AVIF_RESULT_OK) {
                SDL_free(rgb.pixels);
                return SDL_SetError("Couldn't convert AVIF image to RGB: %s", lib.avifResultToString(result));
            }

            frame_surface = SDL_CreateSurface(image->width, image->height, SDL_PIXELFORMAT_XBGR2101010);
            if (frame_surface) {
                ConvertRGB16toXBGR2101010(&rgb, frame_surface);
            }
            SDL_free(rgb.pixels);
        }

        if (frame_surface) {
            SetHDRProperties(frame_surface, image);
        }
    } else {
        frame_surface = SDL_CreateSurface(image->width, image->height, SDL_PIXELFORMAT_RGBA32);
        if (!frame_surface) {
            return SDL_SetError("Couldn't create surface for AVIF frame");
        }

        avifRGBImage rgb;
        SDL_zero(rgb);

        rgb.width = frame_surface->w;
        rgb.height = frame_surface->h;
        rgb.depth = 8;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        rgb.format = AVIF_RGB_FORMAT_RGBA;
#else
        rgb.format = AVIF_RGB_FORMAT_ABGR;
#endif

        rgb.ignoreAlpha = false;
        rgb.pixels = (uint8_t *)frame_surface->pixels;
        rgb.rowBytes = (uint32_t)frame_surface->pitch;

        result = lib.avifImageYUVToRGB(image, &rgb);
        if (result != AVIF_RESULT_OK) {
            SDL_DestroySurface(frame_surface);
            return SDL_SetError("Couldn't convert AVIF image to RGB: %s", lib.avifResultToString(result));
        }

        SDL_Colorspace colorspace = SDL_DEFINE_COLORSPACE(SDL_COLOR_TYPE_RGB,
                                                          SDL_COLOR_RANGE_FULL,
                                                          image->colorPrimaries,
                                                          image->transferCharacteristics,
                                                          image->matrixCoefficients,
                                                          SDL_CHROMA_LOCATION_NONE);
        SDL_SetSurfaceColorspace(frame_surface, colorspace);
    }

    *duration = ctx->decoder->imageTiming.durationInTimescales * decoder->timebase_numerator;

    ctx->current_frame++;

    *frame = frame_surface;
    return true;
}

static bool IMG_AnimationDecoderClose_Internal(IMG_AnimationDecoder *decoder)
{
    IMG_AnimationDecoderContext *ctx = decoder->ctx;

    if (ctx->decoder) {
        lib.avifDecoderDestroy(ctx->decoder);
        ctx->decoder = NULL;
    }

    if (ctx->ioContext.data) {
        SDL_free(ctx->ioContext.data);
        ctx->ioContext.data = NULL;
        ctx->ioContext.size = 0;
    }

    SDL_free(ctx);
    decoder->ctx = NULL;
    return true;
}

bool IMG_CreateAVIFAnimationDecoder(IMG_AnimationDecoder *decoder, SDL_PropertiesID props)
{
    if (!IMG_InitAVIF()) {
        return false;
    }

    IMG_AnimationDecoderContext *ctx = (IMG_AnimationDecoderContext *)SDL_calloc(1, sizeof(*ctx));
    if (!ctx) {
        return false;
    }

    ctx->start_pos = SDL_TellIO(decoder->src);
    if (ctx->start_pos < 0) {
        SDL_free(ctx);
        return SDL_SetError("Failed to get current stream position");
    }

    ctx->decoder = lib.avifDecoderCreate();
    if (!ctx->decoder) {
        SDL_free(ctx);
        return SDL_SetError("Couldn't create AVIF decoder");
    }

    ctx->decoder->strictFlags = AVIF_STRICT_DISABLED;
    ctx->decoder->timescale = decoder->timebase_denominator;

    ctx->ioContext.src = decoder->src;
    ctx->ioContext.start = ctx->start_pos;
    ctx->ioContext.data = NULL;
    ctx->ioContext.size = 0;

    ctx->io.destroy = DestroyAVIFIO;
    ctx->io.read = ReadAVIFIO;
    ctx->io.data = &ctx->ioContext;
    lib.avifDecoderSetIO(ctx->decoder, &ctx->io);

    ctx->current_frame = 0;

    int maxLCores = SDL_GetNumLogicalCPUCores();
    int maxThreads = (int)SDL_GetNumberProperty(props, "avif.maxthreads", maxLCores / 2);
    maxThreads = SDL_clamp(maxThreads, 1, maxLCores);
    ctx->decoder->maxThreads = maxThreads;

    bool allowProgressive = SDL_GetBooleanProperty(props, "avif.allowprogressive", true);
    ctx->decoder->allowProgressive = allowProgressive ? AVIF_TRUE : AVIF_FALSE;

    bool allowIncremental = SDL_GetBooleanProperty(props, "avif.allowincremental", false);
    ctx->decoder->allowIncremental = allowIncremental ? AVIF_TRUE : AVIF_FALSE;

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    ctx->decoder->ignoreExif = ignoreProps ? AVIF_TRUE : AVIF_FALSE;
    ctx->decoder->ignoreXMP = ignoreProps ? AVIF_TRUE : AVIF_FALSE;

    decoder->ctx = ctx;
    decoder->Reset = IMG_AnimationDecoderReset_Internal;
    decoder->GetNextFrame = IMG_AnimationDecoderGetNextFrame_Internal;
    decoder->Close = IMG_AnimationDecoderClose_Internal;

    avifResult result = lib.avifDecoderParse(ctx->decoder);
    if (result != AVIF_RESULT_OK) {
        SDL_SetError("Couldn't parse AVIF animation: %s", lib.avifResultToString(result));
        SDL_free(ctx);
        return false;
    }

    if (ctx->decoder->imageCount < 1) {
        SDL_SetError("No frames found in AVIF");
        SDL_free(ctx);
        return false;
    }

    ctx->width = ctx->decoder->image->width;
    ctx->height = ctx->decoder->image->height;
    ctx->total_frames = ctx->decoder->imageCount;

    if (!ignoreProps) {
        // Allow implicit properties to be set which are not globalized but specific to the decoder.
        SDL_SetNumberProperty(decoder->props, "IMG_PROP_METADATA_FRAME_COUNT_NUMBER", ctx->total_frames);

        // Set well-defined properties.
        SDL_SetNumberProperty(decoder->props, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, ctx->decoder->repetitionCount);

        // Get other well-defined properties and set them in our props.
        if (!ctx->decoder->ignoreXMP) {
            uint8_t *data = ctx->decoder->image->xmp.data;
            size_t len = ctx->decoder->image->xmp.size;
            if (data && len > 0) {
                char *desc = __xmlman_GetXMPDescription(data, len);
                char *rights = __xmlman_GetXMPCopyright(data, len);
                char *title = __xmlman_GetXMPTitle(data, len);
                char *creator = __xmlman_GetXMPCreator(data, len);
                char *createDate = __xmlman_GetXMPCreateDate(data, len);
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
                if (createDate) {
                    SDL_SetStringProperty(decoder->props, IMG_PROP_METADATA_CREATION_TIME_STRING, createDate);
                    SDL_free(createDate);
                }
            }
        }
    }

    return true;
}

#else

bool IMG_CreateAVIFAnimationDecoder(IMG_AnimationDecoder* decoder, SDL_PropertiesID props)
{
    (void)decoder;
    (void)props;
    return SDL_SetError("SDL_image built without AVIF animation support");
}

#endif /* LOAD_AVIF */

#if SAVE_AVIF

struct IMG_AnimationEncoderContext
{
    avifEncoder *encoder;
    bool first_frame_added;
    SDL_PropertiesID metadata;
};

static bool AnimationEncoder_AddFrame(IMG_AnimationEncoder *encoder, SDL_Surface *surface, Uint64 duration)
{
    avifImage *image = NULL;
    avifRGBImage rgb;
    avifResult rc;
    SDL_Colorspace colorspace;
    Uint16 maxCLL, maxFALL;
    SDL_PropertiesID props;
    uint64_t durationInTimescales;
    bool lockedSurf = false;
    SDL_Surface *temp = NULL;
    bool temp_is_copy = false;
    bool isLossless = encoder->quality == 100;
    bool is10bit = SDL_ISPIXELFORMAT_10BIT(surface->format);
    bool is16bit = (surface->format == SDL_PIXELFORMAT_RGB48 ||
                   surface->format == SDL_PIXELFORMAT_BGR48 ||
                   surface->format == SDL_PIXELFORMAT_RGBA64 ||
                   surface->format == SDL_PIXELFORMAT_ARGB64 ||
                   surface->format == SDL_PIXELFORMAT_BGRA64 ||
                   surface->format == SDL_PIXELFORMAT_ABGR64);
    bool hasAlpha = SDL_ISPIXELFORMAT_ALPHA(surface->format);

    durationInTimescales = duration * encoder->timebase_numerator;
    colorspace = SDL_GetSurfaceColorspace(surface);
    props = SDL_GetSurfaceProperties(surface);
    maxCLL = (Uint16)SDL_GetNumberProperty(props, SDL_PROP_SURFACE_MAXCLL_NUMBER, 0);
    maxFALL = (Uint16)SDL_GetNumberProperty(props, SDL_PROP_SURFACE_MAXFALL_NUMBER, 0);

    uint32_t depth = is16bit ? 16 : (is10bit ? 10 : 8);
    avifPixelFormat pixelFormat = (isLossless) ? AVIF_PIXEL_FORMAT_NONE : AVIF_PIXEL_FORMAT_YUV444;

    image = lib.avifImageCreate(surface->w, surface->h, depth, pixelFormat);
    if (!image) {
        return SDL_SetError("Couldn't create AVIF image");
    }

    if (!encoder->ctx->first_frame_added && IMG_HasMetadata(encoder->ctx->metadata)) {
        size_t outlen = 0;
        uint8_t *xmp_data = __xmlman_ConstructXMPWithRDFDescription(SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_TITLE_STRING, NULL), SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_AUTHOR_STRING, NULL), SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_DESCRIPTION_STRING, NULL), SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_COPYRIGHT_STRING, NULL), SDL_GetStringProperty(encoder->ctx->metadata, IMG_PROP_METADATA_CREATION_TIME_STRING, NULL), &outlen);
        if (!xmp_data || outlen < 1) {
            lib.avifImageDestroy(image);
            return SDL_SetError("Couldn't create XMP data for AVIF image");
        }
        avifResult ar = lib.avifImageSetMetadataXMP(image, xmp_data, outlen);
        if (ar != AVIF_RESULT_OK) {
            lib.avifImageDestroy(image);
            SDL_free(xmp_data);
            return SDL_SetError("Couldn't set XMP metadata for AVIF image: %s", lib.avifResultToString(ar));
        }
        SDL_free(xmp_data);
    }

    image->yuvRange = AVIF_RANGE_FULL;
    image->colorPrimaries = (avifColorPrimaries)SDL_COLORSPACEPRIMARIES(colorspace);
    image->transferCharacteristics = (avifTransferCharacteristics)SDL_COLORSPACETRANSFER(colorspace);

    if (is10bit || is16bit || isLossless) {
        image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_IDENTITY;
    } else {
        image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_BT709;
    }

    image->clli.maxCLL = maxCLL;
    image->clli.maxPALL = maxFALL;

    SDL_zero(rgb);
    lib.avifRGBImageSetDefaults(&rgb, image);

    if (SDL_MUSTLOCK(surface)) {
        if (!SDL_LockSurface(surface)) {
            lib.avifImageDestroy(image);
            return SDL_SetError("Couldn't lock surface for reading");
        }
        lockedSurf = true;
    }

    if (is16bit) {
        rgb.depth = 16;

        switch (surface->format) {
        case SDL_PIXELFORMAT_RGB48:
            rgb.format = AVIF_RGB_FORMAT_RGB;
            break;
        case SDL_PIXELFORMAT_BGR48:
            rgb.format = AVIF_RGB_FORMAT_BGR;
            break;
        case SDL_PIXELFORMAT_RGBA64:
            rgb.format = AVIF_RGB_FORMAT_RGBA;
            break;
        case SDL_PIXELFORMAT_ARGB64:
            rgb.format = AVIF_RGB_FORMAT_ARGB;
            break;
        case SDL_PIXELFORMAT_BGRA64:
            rgb.format = AVIF_RGB_FORMAT_BGRA;
            break;
        case SDL_PIXELFORMAT_ABGR64:
            rgb.format = AVIF_RGB_FORMAT_ABGR;
            break;
        default:
            if (lockedSurf) {
                SDL_UnlockSurface(surface);
            }
            lib.avifImageDestroy(image);
            return SDL_SetError("Received a pixelformat that isn't 16 bit for 16 bit encoding");
        }

        rgb.ignoreAlpha = !hasAlpha;
        rgb.pixels = (uint8_t *)surface->pixels;
        rgb.rowBytes = (uint32_t)surface->pitch;

        if (!isLossless) {
            rc = lib.avifImageRGBToYUV(image, &rgb);

            if (rc != AVIF_RESULT_OK) {
                if (lockedSurf) {
                    SDL_UnlockSurface(surface);
                }
                lib.avifImageDestroy(image);
                return SDL_SetError("Couldn't convert 16-bit RGB to YUV: %s", lib.avifResultToString(rc));
            }
        }
    } else if (is10bit) {
        const Uint16 expand_alpha[] = {
            0, 0x155, 0x2aa, 0x3ff
        };

        rgb.depth = 10;
        rgb.format = (SDL_PIXELORDER(surface->format) == SDL_PACKEDORDER_XRGB ||
                      SDL_PIXELORDER(surface->format) == SDL_PACKEDORDER_ARGB) ?
                      AVIF_RGB_FORMAT_RGBA : AVIF_RGB_FORMAT_BGRA;
        rgb.ignoreAlpha = !hasAlpha;
        rgb.rowBytes = (uint32_t)image->width * 4 * sizeof(Uint16);
        rgb.pixels = (uint8_t *)SDL_malloc(image->height * rgb.rowBytes);

        if (!rgb.pixels) {
            if (lockedSurf) {
                SDL_UnlockSurface(surface);
            }
            lib.avifImageDestroy(image);
            return SDL_SetError("Out of memory for RGB pixels");
        }

        // Convert 10-bit packed format to planar format for libavif
        int width, height;
        Uint16 *dst16 = (Uint16 *)rgb.pixels;
        Uint32 *src = (Uint32 *)surface->pixels;
        int srcskip = surface->pitch - (surface->w * sizeof(Uint32));

        height = image->height;
        while (height--) {
            width = image->width;
            while (width--) {
                Uint32 pixelvalue = *src++;

                // Extract 10-bit components
                *dst16++ = (pixelvalue >> 20) & 0x3FF;  // R
                *dst16++ = (pixelvalue >> 10) & 0x3FF;  // G
                *dst16++ = (pixelvalue >> 0) & 0x3FF;   // B

                // Handle alpha - use full opacity for non-alpha formats
                if (hasAlpha) {
                    *dst16++ = expand_alpha[(pixelvalue >> 30) & 0x3];  // A
                } else {
                    *dst16++ = 0x3FF;  // Full opacity
                }
            }
            src = (Uint32 *)(((Uint8 *)src) + srcskip);
        }

        if (!isLossless) {
            rc = lib.avifImageRGBToYUV(image, &rgb);

            SDL_free(rgb.pixels);
            rgb.pixels = NULL;

            if (rc != AVIF_RESULT_OK) {
                if (lockedSurf) {
                    SDL_UnlockSurface(surface);
                }
                lib.avifImageDestroy(image);
                return SDL_SetError("Couldn't convert to YUV: %s", lib.avifResultToString(rc));
            }
        }
    } else {
        avifRGBFormat format;
        bool needsConversion = true;

        switch (surface->format) {
        case SDL_PIXELFORMAT_RGBA32:
            format = AVIF_RGB_FORMAT_RGBA;
            needsConversion = false;
            break;
        case SDL_PIXELFORMAT_ARGB32:
            format = AVIF_RGB_FORMAT_ARGB;
            needsConversion = false;
            break;
        case SDL_PIXELFORMAT_BGRA32:
            format = AVIF_RGB_FORMAT_BGRA;
            needsConversion = false;
            break;
        case SDL_PIXELFORMAT_ABGR32:
            format = AVIF_RGB_FORMAT_ABGR;
            needsConversion = false;
            break;
        case SDL_PIXELFORMAT_RGB24:
            format = AVIF_RGB_FORMAT_RGB;
            needsConversion = true;
            break;
        case SDL_PIXELFORMAT_BGR24:
            format = AVIF_RGB_FORMAT_BGR;
            needsConversion = true;
            break;
        case SDL_PIXELFORMAT_RGBX32:
        case SDL_PIXELFORMAT_XRGB32:
        case SDL_PIXELFORMAT_BGRX32:
        case SDL_PIXELFORMAT_XBGR32:
            needsConversion = true;
            format = AVIF_RGB_FORMAT_RGBA;
            break;
        default:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            format = AVIF_RGB_FORMAT_ABGR;
#else
            format = AVIF_RGB_FORMAT_RGBA;
#endif
            needsConversion = true;
            break;
        }

        if (needsConversion) {
            Uint32 target_format = hasAlpha ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGBX32;
            temp = SDL_ConvertSurface(surface, target_format);
            if (!temp) {
                if (lockedSurf) {
                    SDL_UnlockSurface(surface);
                }
                lib.avifImageDestroy(image);
                return SDL_SetError("Couldn't convert surface to compatible format");
            }
            temp_is_copy = true;

            rgb.format = format;
            rgb.pixels = (uint8_t *)temp->pixels;
            rgb.rowBytes = (uint32_t)temp->pitch;
        } else {
            rgb.format = format;
            rgb.pixels = (uint8_t *)surface->pixels;
            rgb.rowBytes = (uint32_t)surface->pitch;
        }

        rgb.depth = 8;
        rgb.ignoreAlpha = !hasAlpha;
        rgb.alphaPremultiplied = AVIF_FALSE;
        rgb.chromaUpsampling = AVIF_CHROMA_UPSAMPLING_AUTOMATIC;

        if (!isLossless) {
            rc = lib.avifImageRGBToYUV(image, &rgb);

            if (rc != AVIF_RESULT_OK) {
                if (temp_is_copy && temp) {
                    SDL_DestroySurface(temp);
                }
                if (lockedSurf) {
                    SDL_UnlockSurface(surface);
                }
                lib.avifImageDestroy(image);
                return SDL_SetError("Couldn't convert to YUV: %s", lib.avifResultToString(rc));
            }
        }
    }

    avifAddImageFlags addImageFlags = AVIF_ADD_IMAGE_FLAG_NONE;
    if (isLossless || !encoder->ctx->first_frame_added) {
        addImageFlags = AVIF_ADD_IMAGE_FLAG_FORCE_KEYFRAME;
    }

    if (isLossless) {
        if (is10bit && rgb.pixels) {
            // For 10-bit lossless that required conversion
            rc = lib.avifEncoderAddImage(encoder->ctx->encoder, image, durationInTimescales, addImageFlags);
            SDL_free(rgb.pixels);
            rgb.pixels = NULL;
        } else {
            // For direct encoding (8-bit, 16-bit, or pre-converted 10-bit)
            rc = lib.avifEncoderAddImage(encoder->ctx->encoder, image, durationInTimescales, addImageFlags);
        }
    } else {
        // For lossy encoding (YUV conversion already done)
        rc = lib.avifEncoderAddImage(encoder->ctx->encoder, image, durationInTimescales, addImageFlags);
    }

    if (temp_is_copy && temp) {
        SDL_DestroySurface(temp);
    }

    if (lockedSurf) {
        SDL_UnlockSurface(surface);
    }

    lib.avifImageDestroy(image);

    if (rc != AVIF_RESULT_OK) {
        return SDL_SetError("Failed to add image to avif encoder: %s", lib.avifResultToString(rc));
    }

    if (!encoder->ctx->first_frame_added) {
        encoder->ctx->first_frame_added = true;
    }

    return true;
}

static bool AnimationEncoder_End(struct IMG_AnimationEncoder* encoder)
{
    avifRWData avifOutput = AVIF_DATA_EMPTY;
    avifResult rc;
    bool result = false;

    if (!encoder->ctx || !encoder->ctx->encoder) {
        return SDL_SetError("Invalid context or encoder");
    }

    rc = lib.avifEncoderFinish(encoder->ctx->encoder, &avifOutput);
    if (rc != AVIF_RESULT_OK) {
        SDL_SetError("Failed to finish encoder: %s", lib.avifResultToString(rc));
        goto done;
    }

    if (SDL_WriteIO(encoder->dst, avifOutput.data, avifOutput.size) == avifOutput.size) {
        result = true;
    } else {
        SDL_SetError("Failed to write AVIF data to IOStream");
    }

done:
    if (encoder->ctx->metadata) {
        SDL_DestroyProperties(encoder->ctx->metadata);
        encoder->ctx->metadata = 0;
    }
    if (encoder->ctx->encoder) {
        lib.avifEncoderDestroy(encoder->ctx->encoder);
        encoder->ctx->encoder = NULL;
    }
    if (encoder->ctx) {
        SDL_free(encoder->ctx);
        encoder->ctx = NULL;
    }
    lib.avifRWDataFree(&avifOutput);

    return result;
}

bool IMG_CreateAVIFAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    if (!IMG_InitAVIF()) {
        return false;
    }

    IMG_AnimationEncoderContext *ctx = (IMG_AnimationEncoderContext *)SDL_calloc(1, sizeof(*ctx));
    if (!ctx) {
        return SDL_SetError("Out of memory for AVIF context");
    }

    ctx->encoder = lib.avifEncoderCreate();
    if (!ctx->encoder) {
        SDL_free(ctx);
        return SDL_SetError("Couldn't create AVIF encoder");
    }

    if (encoder->quality < 0) {
        encoder->quality = 75;
    } else if (encoder->quality > 100) {
        encoder->quality = 100;
    }

    int availableLCores = SDL_GetNumLogicalCPUCores();
    int threads = (int)SDL_GetNumberProperty(props, "maxthreads", availableLCores / 2);
    threads = SDL_clamp(threads, 1, availableLCores);

    int keyFrameInterval = (int)SDL_GetNumberProperty(props, "keyframeinterval", 0);

    ctx->encoder->maxThreads = threads;
    ctx->encoder->quality = encoder->quality;

    ctx->encoder->qualityAlpha = AVIF_QUALITY_DEFAULT;
    ctx->encoder->speed = AVIF_SPEED_FASTEST;

    if (ctx->encoder->quality >= 100) {
        ctx->encoder->keyframeInterval = SDL_min(1, keyFrameInterval);
        ctx->encoder->minQuantizer = 0;
        ctx->encoder->maxQuantizer = 0;

        ctx->encoder->minQuantizerAlpha = 0;
        ctx->encoder->maxQuantizerAlpha = 0;
        ctx->encoder->autoTiling = AVIF_FALSE;
    } else {
        ctx->encoder->keyframeInterval = keyFrameInterval;
        ctx->encoder->minQuantizer = 8;
        ctx->encoder->maxQuantizer = 63;

        ctx->encoder->minQuantizerAlpha = 0;
        ctx->encoder->maxQuantizerAlpha = 63;
        ctx->encoder->autoTiling = AVIF_TRUE;
    }

    ctx->encoder->tileRowsLog2 = 0;
    ctx->encoder->tileColsLog2 = 0;

    ctx->encoder->timescale = encoder->timebase_denominator;

    bool ignoreProps = SDL_GetBooleanProperty(props, IMG_PROP_METADATA_IGNORE_PROPS_BOOLEAN, false);
    if (!ignoreProps) {
        ctx->encoder->repetitionCount = (int)SDL_GetNumberProperty(props, IMG_PROP_METADATA_LOOP_COUNT_NUMBER, -1);
        if (ctx->encoder->repetitionCount < 1 && ctx->encoder->repetitionCount != -1) {
            ctx->encoder->repetitionCount = -1;
        }

        ctx->metadata = SDL_CreateProperties();
        if (!ctx->metadata) {
            lib.avifEncoderDestroy(ctx->encoder);
            SDL_free(ctx);
            return SDL_SetError("Couldn't create properties ID for AVIF encoder metadata");
        }

        if (!SDL_CopyProperties(props, ctx->metadata)) {
            lib.avifEncoderDestroy(ctx->encoder);
            SDL_DestroyProperties(ctx->metadata);
            SDL_free(ctx);
            return SDL_SetError("Couldn't copy properties to AVIF encoder metadata");
        }
    }

    encoder->ctx = ctx;
    encoder->AddFrame = AnimationEncoder_AddFrame;
    encoder->Close = AnimationEncoder_End;

    return true;
}

#else

bool IMG_CreateAVIFAnimationEncoder(IMG_AnimationEncoder *encoder, SDL_PropertiesID props)
{
    (void)encoder;
    (void)props;
    return SDL_SetError("SDL_image built without AVIF animation save support");
}

#endif /* SAVE_AVIF */
