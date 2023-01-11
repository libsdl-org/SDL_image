#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_image.h>

int main(int argc, char *argv[])
{
    if (SDL_Init(0) < 0) {
        SDL_Log("Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    if (IMG_Init(0) == 0) {
        SDL_Log("No image formats supported\n");
    }
    IMG_Quit();
    SDL_Quit();
    return 0;
}
