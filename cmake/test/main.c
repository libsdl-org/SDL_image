#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#define TEST_INIT_FLAG(FLAG) do {                   \
        if ((IMG_Init(FLAG) & FLAG) == FLAG) {      \
            SDL_Log("IMG_Init("#FLAG") succeeded"); \
        } else {                                    \
            SDL_Log("IMG_Init("#FLAG") failed");    \
        }                                           \
    } while (0);

#define FOREACH_INIT_FLAGS(X) \
    X(IMG_INIT_JPG)           \
    X(IMG_INIT_PNG)           \
    X(IMG_INIT_TIF)           \
    X(IMG_INIT_WEBP)          \
    X(IMG_INIT_JXL)           \
    X(IMG_INIT_AVIF)          \

int main(int argc, char *argv[])
{
    if (SDL_Init(0) < 0) {
        SDL_Log("SDL_Init(0) failed: %s\n", SDL_GetError());
        return 1;
    }

    FOREACH_INIT_FLAGS(TEST_INIT_FLAG)

    IMG_Quit();
    SDL_Quit();
    return 0;
}
