/*
  showimage:  A test application for the SDL image loading library.
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

#ifndef SDL_PROP_SURFACE_FLIP_NUMBER
#define SDL_PROP_SURFACE_FLIP_NUMBER    "SDL.surface.flip"
#endif

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

static void set_cursor(const char *cursor_file)
{
    IMG_Animation *anim = IMG_LoadAnimation(get_file_path(cursor_file));
    if (anim) {
        SDL_Cursor *cursor = IMG_CreateAnimatedCursor(anim, 0, 0);
        if (cursor) {
            SDL_SetCursor(cursor);
        } else {
            SDL_Log("Couldn't create cursor with %s: %s", cursor_file, SDL_GetError());
        }
        IMG_FreeAnimation(anim);
    }
}

static SDL_Texture *load_image(SDL_Renderer *renderer, const char *file, const char *tonemap, SDL_FlipMode *flip, float *rotation)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_Load(get_file_path(file));
    if (!surface) {
        SDL_Log("Couldn't load %s: %s\n", file, SDL_GetError());
        return NULL;
    }

    if (tonemap) {
        SDL_Surface *temp;

        /* Use the tonemap operator to convert to SDR output */
        SDL_SetStringProperty(SDL_GetSurfaceProperties(surface), SDL_PROP_SURFACE_TONEMAP_OPERATOR_STRING, tonemap);
        temp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!temp) {
            SDL_Log("Couldn't convert surface: %s\n", SDL_GetError());
            return NULL;
        }
        surface = temp;
    }

    *flip = (SDL_FlipMode)SDL_GetNumberProperty(SDL_GetSurfaceProperties(surface), SDL_PROP_SURFACE_FLIP_NUMBER, SDL_FLIP_NONE);
    *rotation = SDL_GetFloatProperty(SDL_GetSurfaceProperties(surface), SDL_PROP_SURFACE_ROTATION_FLOAT, 0.0f);

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    SDL_FlipMode flip = SDL_FLIP_NONE;
    float rotation = 0.0f;
    Uint32 flags;
    int i;
    int done = 0;
    int quit = 0;
    SDL_Event event;
    const char *tonemap = NULL;
    const char *saveFile = NULL;
    int attempted = 0;
    int result = 0;

    (void)argc;

#if 0 /* We now allow drag and drop onto the window */
    /* Check command line usage */
    if ( !argv[1] ) {
        SDL_Log("Usage: %s [-fullscreen] [-tonemap X] [-save file.png] <image_file> ...\n", argv[0]);
        result = 1;
        goto done;
    }
#endif

    flags = SDL_WINDOW_HIDDEN;
    for (i = 1; argv[i]; ++i) {
        if (SDL_strcmp(argv[i], "-fullscreen") == 0) {
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

    if (SDL_GetBooleanProperty(SDL_GetDisplayProperties(SDL_GetPrimaryDisplay()), SDL_PROP_DISPLAY_HDR_ENABLED_BOOLEAN, false)) {
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

        if (SDL_strcmp(argv[i], "-cursor") == 0 && argv[i + 1]) {
            ++i;
            set_cursor(argv[i]);
            continue;
        }

        /* Open the image file */
        ++attempted;
        texture = load_image(renderer, argv[i], tonemap, &flip, &rotation);
        if (!texture) {
            continue;
        }

        /* Save the image file, if desired */
        if (saveFile) {
            SDL_Surface *surface = IMG_Load(get_file_path(argv[i]));
            if (surface) {
                if (!IMG_Save(surface, saveFile)) {
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
        if (rotation == 90.0f || rotation == 270.0f) {
            SDL_SetWindowSize(window, texture->h, texture->w);
        } else {
            SDL_SetWindowSize(window, texture->w, texture->h);
        }
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
            draw_background(renderer);

            /* Display the image */
            SDL_FRect dst;
            if (rotation == 90.0f || rotation == 270.0f) {
                // Use a pre-rotated destination rectangle
                dst.x = (texture->h - texture->w) / 2.0f;
                dst.y = (texture->w - texture->h) / 2.0f;
                dst.w = (float)texture->w;
                dst.h = (float)texture->h;
            } else {
                dst.x = 0.0f;
                dst.y = 0.0f;
                dst.w = (float)texture->w;
                dst.h = (float)texture->h;
            }
            SDL_RenderTextureRotated(renderer, texture, NULL, &dst, rotation, NULL, flip);
            SDL_RenderPresent(renderer);

            SDL_Delay(100);
        }
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    if (attempted == 0 && !quit) {
        /* Show the window if needed */
        SDL_SetWindowTitle(window, "showimage");
        SDL_SetWindowSize(window, 640, 480);
        SDL_ShowWindow(window);

        while ( !done ) {
            while ( SDL_PollEvent(&event) ) {
                switch (event.type) {
                    case SDL_EVENT_DROP_FILE:
                        {
                            const char *file = event.drop.data;

                            SDL_DestroyTexture(texture);

                            SDL_Log("Loading %s\n", file);
                            texture = load_image(renderer, file, tonemap, &flip, &rotation);
                            if (!texture) {
                                break;
                            }
                            SDL_SetWindowTitle(window, file);
                            if (rotation == 90.0f || rotation == 270.0f) {
                                SDL_SetWindowSize(window, texture->h, texture->w);
                            } else {
                                SDL_SetWindowSize(window, texture->w, texture->h);
                            }
                        }
                        break;
                    case SDL_EVENT_KEY_UP:
                        switch (event.key.key) {
                        case SDLK_ESCAPE:
                        case SDLK_Q:
                            done = 1;
                            break;
                        }
                        break;
                    case SDL_EVENT_MOUSE_BUTTON_DOWN:
                        done = 1;
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
            SDL_FRect dst;
            if (rotation == 90.0f || rotation == 270.0f) {
                // Use a pre-rotated destination rectangle
                dst.x = (texture->h - texture->w) / 2.0f;
                dst.y = (texture->w - texture->h) / 2.0f;
                dst.w = (float)texture->w;
                dst.h = (float)texture->h;
            } else {
                dst.x = 0.0f;
                dst.y = 0.0f;
                dst.w = (float)texture->w;
                dst.h = (float)texture->h;
            }
            SDL_RenderTextureRotated(renderer, texture, NULL, &dst, rotation, NULL, flip);
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
