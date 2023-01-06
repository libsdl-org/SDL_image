# SDL2_image CMake configuration file:
# This file is meant to be placed in Resources/CMake of a SDL2_image framework

# INTERFACE_LINK_OPTIONS needs CMake 3.12
cmake_minimum_required(VERSION 3.12)

include(FeatureSummary)
set_package_properties(SDL2_image PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_image/"
    DESCRIPTION "SDL_image is an image file loading library"
)

set(SDL2_image_FOUND TRUE)

set(SDL2IMAGE_AVIF  FALSE)
set(SDL2IMAGE_BMP   TRUE)
set(SDL2IMAGE_GIF   TRUE)
set(SDL2IMAGE_JPG   TRUE)
set(SDL2IMAGE_JXL   FALSE)
set(SDL2IMAGE_LBM   TRUE)
set(SDL2IMAGE_PCX   TRUE)
set(SDL2IMAGE_PNG   TRUE)
set(SDL2IMAGE_PNM   TRUE)
set(SDL2IMAGE_QOI   TRUE)
set(SDL2IMAGE_SVG   TRUE)
set(SDL2IMAGE_TGA   TRUE)
set(SDL2IMAGE_TIF   TRUE)
set(SDL2IMAGE_XCF   TRUE)
set(SDL2IMAGE_XPM   TRUE)
set(SDL2IMAGE_XV    TRUE)
set(SDL2IMAGE_WEBP  FALSE)

set(SDL2IMAGE_JPG_SAVE FALSE)
set(SDL2IMAGE_PNG_SAVE FALSE)

set(SDL2IMAGE_VENDORED  FALSE)

set(SDL2IMAGE_BACKEND_IMAGEIO   FALSE)
set(SDL2IMAGE_BACKEND_STB       TRUE)
set(SDL2IMAGE_BACKEND_WIC       FALSE)

string(REGEX REPLACE "SDL2_image\\.framework.*" "SDL2_image.framework" _sdl2image_framework_path "${CMAKE_CURRENT_LIST_DIR}")
string(REGEX REPLACE "SDL2_image\\.framework.*" "" _sdl2image_framework_parent_path "${CMAKE_CURRENT_LIST_DIR}")

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL2_image-target.cmake files.

if(NOT TARGET SDL2_image::SDL2_image)
    add_library(SDL2_image::SDL2_image INTERFACE IMPORTED)
    set_target_properties(SDL2_image::SDL2_image
        PROPERTIES
            INTERFACE_COMPILE_OPTIONS "SHELL:-F ${_sdl2image_framework_parent_path}"
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl2image_framework_path}/Headers"
            INTERFACE_LINK_OPTIONS "SHELL:-F ${_sdl2image_framework_parent_path};SHELL:-framework SDL2_image"
            COMPATIBLE_INTERFACE_BOOL "SDL2_SHARED"
            INTERFACE_SDL2_SHARED "ON"
    )
endif()

unset(_sdl2image_framework_path)
unset(_sdl2image_framework_parent_path)
