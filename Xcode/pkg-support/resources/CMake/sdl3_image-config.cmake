# SDL3_image CMake configuration file:
# This file is meant to be placed in Resources/CMake of a SDL3_image framework

# INTERFACE_LINK_OPTIONS needs CMake 3.12
cmake_minimum_required(VERSION 3.12)

include(FeatureSummary)
set_package_properties(SDL3_image PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_image/"
    DESCRIPTION "SDL_image is an image file loading library"
)

set(SDL3_image_FOUND TRUE)

set(SDL3IMAGE_AVIF  FALSE)
set(SDL3IMAGE_BMP   TRUE)
set(SDL3IMAGE_GIF   TRUE)
set(SDL3IMAGE_JPG   TRUE)
set(SDL3IMAGE_JXL   FALSE)
set(SDL3IMAGE_LBM   TRUE)
set(SDL3IMAGE_PCX   TRUE)
set(SDL3IMAGE_PNG   TRUE)
set(SDL3IMAGE_PNM   TRUE)
set(SDL3IMAGE_QOI   TRUE)
set(SDL3IMAGE_SVG   TRUE)
set(SDL3IMAGE_TGA   TRUE)
set(SDL3IMAGE_TIF   TRUE)
set(SDL3IMAGE_XCF   TRUE)
set(SDL3IMAGE_XPM   TRUE)
set(SDL3IMAGE_XV    TRUE)
set(SDL3IMAGE_WEBP  FALSE)

set(SDL3IMAGE_JPG_SAVE FALSE)
set(SDL3IMAGE_PNG_SAVE FALSE)

set(SDL3IMAGE_VENDORED  FALSE)

set(SDL3IMAGE_BACKEND_IMAGEIO   FALSE)
set(SDL3IMAGE_BACKEND_STB       TRUE)
set(SDL3IMAGE_BACKEND_WIC       FALSE)

string(REGEX REPLACE "SDL3_image\\.framework.*" "SDL3_image.framework" _sdl3image_framework_path "${CMAKE_CURRENT_LIST_DIR}")
string(REGEX REPLACE "SDL3_image\\.framework.*" "" _sdl3image_framework_parent_path "${CMAKE_CURRENT_LIST_DIR}")

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL3_image-target.cmake files.

if(NOT TARGET SDL3_image::SDL3_image)
    add_library(SDL3_image::SDL3_image INTERFACE IMPORTED)
    set_target_properties(SDL3_image::SDL3_image
        PROPERTIES
            INTERFACE_COMPILE_OPTIONS "SHELL:-F ${_sdl3image_framework_parent_path}"
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl3image_framework_path}/Headers"
            INTERFACE_LINK_OPTIONS "SHELL:-F ${_sdl3image_framework_parent_path};SHELL:-framework SDL3_image"
            COMPATIBLE_INTERFACE_BOOL "SDL3_SHARED"
            INTERFACE_SDL3_SHARED "ON"
    )
endif()

unset(_sdl3image_framework_path)
unset(_sdl3image_framework_parent_path)
