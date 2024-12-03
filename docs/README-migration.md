
# Migrating to SDL_image 3.0

This guide provides useful information for migrating applications from SDL_image 2.0 to SDL_image 3.0.

IMG_Init() and IMG_Quit() are no longer necessary. If an image format requires dynamically loading a support library, that will be done automatically.
