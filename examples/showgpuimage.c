/*
  showgpuimage:  A test application for the SDL image loading library.
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

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

static SDL_Window *window;
static SDL_GPUDevice *device;
static SDL_GPUTexture *texture;
static int texture_width;
static int texture_height;

static const char *get_file_path(const char *file)
{
    static char path[4096];

    if (*file != '/' && !SDL_GetPathInfo(file, NULL)) {
        SDL_snprintf(path, sizeof(path), "%s%s", SDL_GetBasePath(), file);
        if (SDL_GetPathInfo(path, NULL)) {
            return path;
        }
    }
    return file;
}

static bool load_image(SDL_GPUCommandBuffer *command_buffer, const char *path)
{
    SDL_ReleaseGPUTexture(device, texture);
    texture = NULL;

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) {
        SDL_Log("SDL_BeginGPUCopyPass() failed: %s", SDL_GetError());
        return false;
    }
    texture = IMG_LoadGPUTexture(device, copy_pass, get_file_path(path), &texture_width, &texture_height);
    if (!texture) {
        SDL_Log("IMG_LoadGPUTexture() failed: %s", SDL_GetError());
    }
    SDL_EndGPUCopyPass(copy_pass);

    return texture != NULL;
}

int main(int argc, char *argv[])
{
    int result = 0;
    bool quit = false;
    SDL_GPUCommandBuffer *command_buffer = NULL;
    SDL_Event event = {0};
    SDL_GPUTexture *swapchain_texture = NULL;
    Uint32 swapchain_width = 0;
    Uint32 swapchain_height = 0;
    SDL_GPUBlitInfo blit_info = {0};

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s", SDL_GetError());
        result = 2;
        goto done;
    }

    window = SDL_CreateWindow("", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow() failed: %s", SDL_GetError());
        result = 2;
        goto done;
    }

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    if (!window) {
        SDL_Log("SDL_CreateGPUDevice() failed: %s", SDL_GetError());
        result = 2;
        goto done;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice() failed: %s", SDL_GetError());
        result = 2;
        goto done;
    }

    if (argc > 1) {
        SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
        if (!command_buffer) {
            SDL_Log("SDL_AcquireGPUCommandBuffer() failed: %s", SDL_GetError());
            result = 2;
            goto done;
        }
        if (!load_image(command_buffer, argv[1])) {
            result = 2;
            goto done;
        }
        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    while (!quit) {
        command_buffer = SDL_AcquireGPUCommandBuffer(device);
        if (!command_buffer) {
            SDL_Log("SDL_AcquireGPUCommandBuffer() failed: %s", SDL_GetError());
            continue;
        }

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;
            case SDL_EVENT_DROP_FILE:
                load_image(command_buffer, event.drop.data);
                break;
            }
        }
        if (quit) {
            break;
        }

        if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &swapchain_width, &swapchain_height)) {
            SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture() failed: %s", SDL_GetError());
            SDL_CancelGPUCommandBuffer(command_buffer);
            continue;
        }

        if (!swapchain_texture || !swapchain_width || !swapchain_height) {
            // Not an error. Happens on minimize
            SDL_CancelGPUCommandBuffer(command_buffer);
            continue;
        }

        if (texture) {
            blit_info.source.texture = texture;
            blit_info.source.w = texture_width;
            blit_info.source.h = texture_height;
            blit_info.destination.texture = swapchain_texture;
            blit_info.destination.w = swapchain_width;
            blit_info.destination.h = swapchain_height;
            SDL_BlitGPUTexture(command_buffer, &blit_info);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

done:
    SDL_ReleaseGPUTexture(device, texture);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
