#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

int main(int argc, char *argv[])
{
    if (!SDL_Init(0)) {
        SDL_Log("SDL_Init(0) failed: %s\n", SDL_GetError());
        return 1;
    }

    IMG_Version();

    SDL_Quit();
    return 0;
}
