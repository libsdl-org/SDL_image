/*
  showclipboard:  A test application for the SDL image loading library.
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

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>


static SDL_Texture *load_clipboard(SDL_Window *window, SDL_Renderer *renderer)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_GetClipboardImage();
    if (surface) {
        char *text = SDL_GetClipboardText();
        if (text && *text) {
            SDL_SetWindowTitle(window, text);
        } else {
            SDL_SetWindowTitle(window, "Copy an image and click here");
        }
        SDL_free(text);

        texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_SetWindowTitle(window, SDL_GetClipboardText());
        SDL_SetWindowSize(window, surface->w, surface->h);
        SDL_SetRenderLogicalPresentation(renderer, surface->w, surface->h, SDL_LOGICAL_PRESENTATION_LETTERBOX);

        SDL_DestroySurface(surface);
    }
    return texture;
}

/* Draw a Gimpish background pattern to show transparency in the image */
static void draw_background(SDL_Renderer *renderer)
{
    const SDL_Color col[2] = {
        { 0x66, 0x66, 0x66, 0xff },
        { 0x99, 0x99, 0x99, 0xff }
    };
    const int dx = 8, dy = 8;
    SDL_FRect rect;
    int i, x, y, w, h;

    SDL_GetCurrentRenderOutputSize(renderer, &w, &h);

    rect.w = (float)dx;
    rect.h = (float)dy;
    for (y = 0; y < h; y += dy) {
        for (x = 0; x < w; x += dx) {
            /* use an 8x8 checkerboard pattern */
            i = (((x ^ y) >> 3) & 1);
            SDL_SetRenderDrawColor(renderer, col[i].r, col[i].g, col[i].b, col[i].a);

            rect.x = (float)x;
            rect.y = (float)y;
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    Uint32 flags = 0;
    int i;
    bool done = false;
    SDL_Event event;
    int result = 0;

    (void)argc;

    /* Check command line usage */
    for ( i=1; argv[i]; ++i ) {
        if (SDL_strcmp(argv[i], "-fullscreen") == 0) {
            SDL_HideCursor();
            flags |= SDL_WINDOW_FULLSCREEN;
        } else {
            SDL_Log("Usage: %s [-fullscreen]\n", argv[0]);
            result = 1;
            goto done;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
        result = 2;
        goto done;
    }

    if (!SDL_CreateWindowAndRenderer("", 640, 480, flags, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer() failed: %s\n", SDL_GetError());
        result = 2;
        goto done;
    }

    texture = load_clipboard(window, renderer);

    while ( !done ) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_CLIPBOARD_UPDATE:
                texture = load_clipboard(window, renderer);
                break;
            case SDL_EVENT_KEY_UP:
                switch (event.key.key) {
                case SDLK_ESCAPE:
                case SDLK_Q:
                    done = 1;
                    break;
                }
                break;
            case SDL_EVENT_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }

        /* Draw a background pattern in case the image has transparency */
        draw_background(renderer);

        /* Display the image */
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, NULL);
        }
        SDL_RenderPresent(renderer);

        SDL_Delay(100);
    }

    if (texture) {
        SDL_DestroyTexture(texture);
    }

    /* We're done! */
done:
    SDL_Quit();
    return result;
}
