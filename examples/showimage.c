/*
  showimage:  A test application for the SDL image loading library.
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

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>


/* Draw a Gimpish background pattern to show transparency in the image */
static void draw_background(SDL_Renderer *renderer, int w, int h)
{
    SDL_Color col[2] = {
        { 0x66, 0x66, 0x66, 0xff },
        { 0x99, 0x99, 0x99, 0xff },
    };
    int i, x, y;
    SDL_FRect rect;
    const int dx = 8, dy = 8;

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
    Uint32 flags;
    float w, h;
    int i;
    int done = 0;
    int quit = 0;
    SDL_Event event;
    const char *tonemap = NULL;
    const char *saveFile = NULL;
    int result = 0;

    (void)argc;

    /* Check command line usage */
    if ( ! argv[1] ) {
        SDL_Log("Usage: %s [-fullscreen] [-tonemap X] [-save file.png] <image_file> ...\n", argv[0]);
        result = 1;
        goto done;
    }

    flags = SDL_WINDOW_HIDDEN;
    for ( i=1; argv[i]; ++i ) {
        if ( SDL_strcmp(argv[i], "-fullscreen") == 0 ) {
            SDL_HideCursor();
            flags |= SDL_WINDOW_FULLSCREEN;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
        result = 2;
        goto done;
    }

    window = SDL_CreateWindow("", 0, 0, flags);
    if (!window) {
        SDL_Log("SDL_CreateWindow() failed: %s\n", SDL_GetError());
        result = 2;
        goto done;
    }

    if (SDL_GetBooleanProperty(SDL_GetDisplayProperties(SDL_GetPrimaryDisplay()), SDL_PROP_DISPLAY_HDR_ENABLED_BOOLEAN, SDL_FALSE)) {
        SDL_PropertiesID props = SDL_CreateProperties();

        SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window);
        SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_OUTPUT_COLORSPACE_NUMBER, SDL_COLORSPACE_SRGB_LINEAR);
        renderer = SDL_CreateRendererWithProperties(props);
        SDL_DestroyProperties(props);
    }
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, NULL);
    }
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer() failed: %s\n", SDL_GetError());
        result = 2;
        goto done;
    }

    for (i=1; argv[i]; ++i) {
        if (SDL_strcmp(argv[i], "-fullscreen") == 0) {
            continue;
        }

        if (SDL_strcmp(argv[i], "-quit") == 0) {
            quit = 1;
            continue;
        }

        if (SDL_strcmp(argv[i], "-tonemap") == 0 && argv[i+1]) {
            ++i;
            tonemap = argv[i];
            continue;
        }

        if (SDL_strcmp(argv[i], "-save") == 0 && argv[i+1]) {
            ++i;
            saveFile = argv[i];
            continue;
        }

        /* Open the image file */
        if (tonemap) {
            SDL_Surface *surface, *temp;

            surface = IMG_Load(argv[i]);
            if (!surface) {
                SDL_Log("Couldn't load %s: %s\n", argv[i], SDL_GetError());
                continue;
            }

            /* Use the tonemap operator to convert to SDR output */
            SDL_SetStringProperty(SDL_GetSurfaceProperties(surface), SDL_PROP_SURFACE_TONEMAP_OPERATOR_STRING, tonemap);
            temp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surface);
            if (!temp) {
                SDL_Log("Couldn't convert surface: %s\n", SDL_GetError());
                continue;
            }

            texture = SDL_CreateTextureFromSurface(renderer, temp);
            SDL_DestroySurface(temp);
            if (!texture) {
                SDL_Log("Couldn't create texture: %s\n", SDL_GetError());
                continue;
            }
        } else {
            texture = IMG_LoadTexture(renderer, argv[i]);
            if (!texture) {
                SDL_Log("Couldn't load %s: %s\n", argv[i], SDL_GetError());
                continue;
            }
        }
        SDL_GetTextureSize(texture, &w, &h);

        /* Save the image file, if desired */
        if (saveFile) {
            SDL_Surface *surface = IMG_Load(argv[i]);
            if (surface) {
                const char *ext = SDL_strrchr(saveFile, '.');
                SDL_bool saved = SDL_FALSE;
                if (ext && SDL_strcasecmp(ext, ".avif") == 0) {
                    saved = IMG_SaveAVIF(surface, saveFile, 90);
                } else if (ext && SDL_strcasecmp(ext, ".bmp") == 0) {
                    saved = SDL_SaveBMP(surface, saveFile);
                } else if (ext && SDL_strcasecmp(ext, ".jpg") == 0) {
                    saved = IMG_SaveJPG(surface, saveFile, 90);
                } else if (ext && SDL_strcasecmp(ext, ".png") == 0) {
                    saved = IMG_SavePNG(surface, saveFile);
                } else {
                    SDL_SetError("Unknown save file type");
                }
                if (!saved) {
                    SDL_Log("Couldn't save %s: %s\n", saveFile, SDL_GetError());
                    result = 3;
                }
            } else {
                SDL_Log("Couldn't load %s: %s\n", argv[i], SDL_GetError());
                result = 3;
            }
        }

        /* Show the window */
        SDL_SetWindowTitle(window, argv[i]);
        SDL_SetWindowSize(window, (int)w, (int)h);
        SDL_ShowWindow(window);

        done = quit;
        while ( !done ) {
            while ( SDL_PollEvent(&event) ) {
                switch (event.type) {
                    case SDL_EVENT_KEY_UP:
                        switch (event.key.key) {
                        case SDLK_LEFT:
                            if ( i > 1 ) {
                                i -= 2;
                                done = 1;
                            }
                            break;
                        case SDLK_RIGHT:
                            if ( argv[i+1] ) {
                                done = 1;
                            }
                            break;
                        case SDLK_ESCAPE:
                        case SDLK_Q:
                            argv[i+1] = NULL;
                            SDL_FALLTHROUGH;
                        case SDLK_SPACE:
                        case SDLK_TAB:
                            done = 1;
                            break;
                        default:
                            break;
                        }
                        break;
                    case SDL_EVENT_MOUSE_BUTTON_DOWN:
                        done = 1;
                        break;
                    case SDL_EVENT_QUIT:
                        argv[i+1] = NULL;
                        done = 1;
                        break;
                    default:
                        break;
                }
            }
            /* Draw a background pattern in case the image has transparency */
            draw_background(renderer, (int)w, (int)h);

            /* Display the image */
            SDL_RenderTexture(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            SDL_Delay(100);
        }
        SDL_DestroyTexture(texture);
    }

    /* We're done! */
done:
    SDL_Quit();
    return result;
}
