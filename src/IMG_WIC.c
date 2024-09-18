/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

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

#if defined(SDL_IMAGE_USE_WIC_BACKEND)

#include <SDL3_image/SDL_image.h>
#include "IMG.h"
#define COBJMACROS
#include <initguid.h>
#include <wincodec.h>

static IWICImagingFactory* wicFactory = NULL;

static int WIC_Init(void)
{
    if (wicFactory == NULL) {
        HRESULT hr = CoCreateInstance(
            &CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory,
            (void**)&wicFactory
        );
        if (FAILED(hr)) {
            return -1;
        }
    }

    return 0;
}

static void WIC_Quit(void)
{
    if (wicFactory) {
        IWICImagingFactory_Release(wicFactory);
    }
}

int IMG_InitPNG(void)
{
    return WIC_Init();
}

void IMG_QuitPNG(void)
{
    WIC_Quit();
}

int IMG_InitJPG(void)
{
    return WIC_Init();
}

void IMG_QuitJPG(void)
{
    WIC_Quit();
}

int IMG_InitTIF(void)
{
    return WIC_Init();
}

void IMG_QuitTIF(void)
{
    WIC_Quit();
}

bool IMG_isPNG(SDL_IOStream *src)
{
    Sint64 start;
    bool is_PNG;
    Uint8 magic[4];

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_PNG = false;
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic) ) {
        if ( magic[0] == 0x89 &&
             magic[1] == 'P' &&
             magic[2] == 'N' &&
             magic[3] == 'G' ) {
            is_PNG = true;
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_PNG;
}

bool IMG_isJPG(SDL_IOStream *src)
{
    Sint64 start;
    bool is_JPG;
    bool in_scan;
    Uint8 magic[4];

    /* This detection code is by Steaphan Greene <stea@cs.binghamton.edu> */
    /* Blame me, not Sam, if this doesn't work right. */
    /* And don't forget to report the problem to the the sdl list too! */

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_JPG = false;
    in_scan = false;
    if (SDL_ReadIO(src, magic, 2) == 2) {
        if ( (magic[0] == 0xFF) && (magic[1] == 0xD8) ) {
            is_JPG = true;
            while (is_JPG) {
                if (SDL_ReadIO(src, magic, 2) != 2) {
                    is_JPG = false;
                } else if ( (magic[0] != 0xFF) && !in_scan ) {
                    is_JPG = false;
                } else if ( (magic[0] != 0xFF) || (magic[1] == 0xFF) ) {
                    /* Extra padding in JPEG (legal) */
                    /* or this is data and we are scanning */
                    SDL_SeekIO(src, -1, SDL_IO_SEEK_CUR);
                } else if (magic[1] == 0xD9) {
                    /* Got to end of good JPEG */
                    break;
                } else if ( in_scan && (magic[1] == 0x00) ) {
                    /* This is an encoded 0xFF within the data */
                } else if ( (magic[1] >= 0xD0) && (magic[1] < 0xD9) ) {
                    /* These have nothing else */
                } else if (SDL_ReadIO(src, magic+2, 2) != 2) {
                    is_JPG = false;
                } else {
                    /* Yes, it's big-endian */
                    Sint64 innerStart;
                    Uint32 size;
                    Sint64 end;
                    innerStart = SDL_TellIO(src);
                    size = (magic[2] << 8) + magic[3];
                    end = SDL_SeekIO(src, size-2, SDL_IO_SEEK_CUR);
                    if ( end != innerStart + size - 2 ) {
                        is_JPG = false;
                    }
                    if ( magic[1] == 0xDA ) {
                        /* Now comes the actual JPEG meat */
#ifdef  FAST_IS_JPEG
                        /* Ok, I'm convinced.  It is a JPEG. */
                        break;
#else
                        /* I'm not convinced.  Prove it! */
                        in_scan = true;
#endif
                    }
                }
            }
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_JPG;
}

bool IMG_isTIF(SDL_IOStream * src)
{
    Sint64 start;
    bool is_TIF;
    Uint8 magic[4];

    if (!src) {
        return false;
    }

    start = SDL_TellIO(src);
    is_TIF = false;
    if (SDL_ReadIO(src, magic, sizeof(magic)) == sizeof(magic) ) {
        if ( (magic[0] == 'I' &&
                      magic[1] == 'I' &&
              magic[2] == 0x2a &&
                      magic[3] == 0x00) ||
             (magic[0] == 'M' &&
                      magic[1] == 'M' &&
              magic[2] == 0x00 &&
                      magic[3] == 0x2a) ) {
            is_TIF = true;
        }
    }
    SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
    return is_TIF;
}

static SDL_Surface* WIC_LoadImage(SDL_IOStream *src)
{
    SDL_Surface* surface = NULL;

    IWICStream* stream = NULL;
    IWICBitmapDecoder* bitmapDecoder = NULL;
    IWICBitmapFrameDecode* bitmapFrame = NULL;
    IWICFormatConverter* formatConverter = NULL;
    UINT width, height;

    if (wicFactory == NULL && (WIC_Init() < 0)) {
        SDL_SetError("WIC failed to initialize!");
        return NULL;
    }

    size_t fileSize;
    Uint8 *memoryBuffer = (Uint8 *)SDL_LoadFile_IO(src, &fileSize, false);
    if (!memoryBuffer) {
        return NULL;  
    }

#define DONE_IF_FAILED(X) if (FAILED((X))) { goto done; }
    DONE_IF_FAILED(IWICImagingFactory_CreateStream(wicFactory, &stream));
    DONE_IF_FAILED(IWICStream_InitializeFromMemory(stream, memoryBuffer, fileSize));
    DONE_IF_FAILED(IWICImagingFactory_CreateDecoderFromStream(
        wicFactory,
        (IStream*)stream,
        NULL,
        WICDecodeMetadataCacheOnDemand,
        &bitmapDecoder
    ));
    DONE_IF_FAILED(IWICBitmapDecoder_GetFrame(bitmapDecoder, 0, &bitmapFrame));
    DONE_IF_FAILED(IWICImagingFactory_CreateFormatConverter(wicFactory, &formatConverter));
    DONE_IF_FAILED(IWICFormatConverter_Initialize(
        formatConverter,
        (IWICBitmapSource*)bitmapFrame,
        &GUID_WICPixelFormat32bppPRGBA,
        WICBitmapDitherTypeNone,
        NULL,
        0.0,
        WICBitmapPaletteTypeCustom
    ));
    DONE_IF_FAILED(IWICBitmapFrameDecode_GetSize(bitmapFrame, &width, &height));
#undef DONE_IF_FAILED

    surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ABGR8888);
    IWICFormatConverter_CopyPixels(
        formatConverter,
        NULL,
        width * 4,
        width * height * 4,
        (BYTE*)surface->pixels
    );

done:
    if (formatConverter) {
        IWICFormatConverter_Release(formatConverter);
    }
    if (bitmapFrame) {
        IWICBitmapFrameDecode_Release(bitmapFrame);
    }
    if (bitmapDecoder) {
        IWICBitmapDecoder_Release(bitmapDecoder);
    }
    if (stream) {
        IWICStream_Release(stream);
    }

 SDL_free(memoryBuffer);

 return surface;
}

SDL_Surface *IMG_LoadPNG_IO(SDL_IOStream *src)
{
    return WIC_LoadImage(src);
}

SDL_Surface *IMG_LoadJPG_IO(SDL_IOStream *src)
{
    return WIC_LoadImage(src);
}

SDL_Surface *IMG_LoadTIF_IO(SDL_IOStream *src)
{
    return WIC_LoadImage(src);
}

#endif /* SDL_IMAGE_USE_WIC_BACKEND */

