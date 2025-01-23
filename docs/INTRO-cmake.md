
# Introduction to SDL_image with CMake

The easiest way to use SDL_image is to include it along with SDL as subprojects in your project.

We'll start by creating a simple project to build and run [hello.c](hello.c)

Create the file CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.16)
project(hello)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# This assumes the SDL_image source is available in vendored/SDL_image
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

# Create your game executable target as usual
add_executable(hello WIN32 hello.c)

# Link to the actual SDL3 library.
target_link_libraries(hello PRIVATE SDL3_image::SDL3_image SDL3::SDL3)
```

Build:
```sh
cmake -S . -B build
cmake --build build
```

Run:
- On Windows the executable is in the build Debug directory:
```sh
cd build/Debug
./hello
``` 
- On other platforms the executable is in the build directory:
```sh
cd build
./hello
```

Support for AVIF, JPEG-XL, TIFF, and WebP are not included by default because of the size of the decode libraries, but you can get them by running the external/download.sh script and then enabling the appropriate SDLIMAGE_* options defined in CMakeLists.txt. SDLIMAGE_VENDORED allows switching between system and vendored libraries.

A more complete example is available at:

https://github.com/Ravbug/sdl3-sample

