/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

static SDL_GPUTexture * LoadGPUTexture(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, SDL_Surface *surface, Uint32 *width, Uint32 *height)
{
    if (width) {
        *width = 0;
    }
    if (height) {
        *height = 0;
    }
    if (!surface) {
        return NULL;
    }

    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *old_surface = surface;
        surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(old_surface);
        if (!surface) {
            return NULL;
        }
    }

    SDL_GPUTextureCreateInfo texture_create_info = {0};
    texture_create_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_create_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_create_info.layer_count_or_depth = 1;
    texture_create_info.num_levels = 1;
    texture_create_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
    texture_create_info.width = surface->w;
    texture_create_info.height = surface->h;
    SDL_GPUTexture  *texture = SDL_CreateGPUTexture(device, &texture_create_info);
    if (!texture) {
        SDL_DestroySurface(surface);
        return NULL;
    }

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {0};
    transfer_buffer_create_info.size = surface->w * surface->h * 4;
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
    if (!transfer_buffer) {
        SDL_DestroySurface(surface);
        SDL_ReleaseGPUTexture(device, texture);
        return NULL;
    }

    Uint8 *dst = SDL_MapGPUTransferBuffer(device, transfer_buffer, 0);
    if (!dst) {
        SDL_DestroySurface(surface);
        SDL_ReleaseGPUTexture(device, texture);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return NULL;
    }
    const Uint8 *src = surface->pixels;
    const int row_bytes = surface->w * 4;
    if (row_bytes == surface->pitch) {
        SDL_memcpy(dst, src, row_bytes * surface->h);
    }
    else {
        for (int y = 0; y < surface->h; y++) {
            SDL_memcpy(dst + y * row_bytes, src + y * surface->pitch, row_bytes);
        }
    }
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUTextureTransferInfo texture_transfer_info = {0};
    SDL_GPUTextureRegion texture_region = {0};
    texture_transfer_info.transfer_buffer = transfer_buffer;
    texture_region.texture = texture;
    texture_region.w = surface->w;
    texture_region.h = surface->h;
    texture_region.d = 1;
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, 0);

    if (width) {
        *width = surface->w;
    }
    if (height) {
        *height = surface->h;
    }

    SDL_DestroySurface(surface);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    return texture;
}

SDL_GPUTexture * IMG_LoadGPUTexture(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, const char *file, int *width, int *height)
{
    if (!device) {
        SDL_InvalidParamError("device");
        return NULL;
    }
    if (!copy_pass) {
        SDL_InvalidParamError("copy_pass");
        return NULL;
    }

    return LoadGPUTexture(device, copy_pass, IMG_Load(file), width, height);
}

SDL_GPUTexture * IMG_LoadGPUTexture_IO(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, SDL_IOStream *src, bool closeio, int *width, int *height)
{
    return IMG_LoadGPUTextureTyped_IO(device, copy_pass, src, closeio, NULL, width, height);
}

SDL_GPUTexture * IMG_LoadGPUTextureTyped_IO(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, SDL_IOStream *src, bool closeio, const char *type, int *width, int *height)
{
    if (!device) {
        SDL_InvalidParamError("device");
        return NULL;
    }
    if (!copy_pass) {
        SDL_InvalidParamError("copy_pass");
        return NULL;
    }

    return LoadGPUTexture(device, copy_pass, IMG_LoadTyped_IO(src, closeio, type), width, height);
}
