Building SDL2_image
-------------------

Step 0 - get emscripten

Step 1 - get sdl2-emscripten
* clone https://github.com/gsathya/SDL-emscripten.git
* follow the build instructions in SDL-emscripten/README-emscripten.txt

Step 2 - get sdl_image
 * emconfigure ./configure  --disable-sdltest --with-sdl-prefix=/path/to/sdl --prefix=/path/to/install
 * emmake make
 * make install
 
