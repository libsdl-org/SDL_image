# SDL3_image CMake configuration file:
# This file is meant to be placed in a cmake subfolder of SDL3_image-devel-3.x.y-VC

include(FeatureSummary)
set_package_properties(SDL3_image PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_image/"
    DESCRIPTION "SDL_image is an image file loading library"
)

cmake_minimum_required(VERSION 3.0)

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
set(SDL3IMAGE_TIF   FALSE)
set(SDL3IMAGE_XCF   FALSE)
set(SDL3IMAGE_XPM   TRUE)
set(SDL3IMAGE_XV    TRUE)
set(SDL3IMAGE_WEBP  FALSE)

set(SDL3IMAGE_JPG_SAVE FALSE)
set(SDL3IMAGE_PNG_SAVE FALSE)

set(SDL3IMAGE_VENDORED  FALSE)

set(SDL3IMAGE_BACKEND_IMAGEIO   FALSE)
set(SDL3IMAGE_BACKEND_STB       TRUE)
set(SDL3IMAGE_BACKEND_WIC       FALSE)

if(CMAKE_SIZEOF_VOID_P STREQUAL "4")
    set(_sdl_arch_subdir "x86")
elseif(CMAKE_SIZEOF_VOID_P STREQUAL "8")
    set(_sdl_arch_subdir "x64")
else()
    unset(_sdl_arch_subdir)
    set(SDL3_image_FOUND FALSE)
    return()
endif()

set(_sdl3image_incdir       "${CMAKE_CURRENT_LIST_DIR}/../include")
set(_sdl3image_library      "${CMAKE_CURRENT_LIST_DIR}/../lib/${_sdl_arch_subdir}/SDL3_image.lib")
set(_sdl3image_dll          "${CMAKE_CURRENT_LIST_DIR}/../lib/${_sdl_arch_subdir}/SDL3_image.dll")

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL3_image-target.cmake files.

if(NOT TARGET SDL3_image::SDL3_image)
    add_library(SDL3_image::SDL3_image SHARED IMPORTED)
    set_target_properties(SDL3_image::SDL3_image
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl3image_incdir}"
            IMPORTED_IMPLIB "${_sdl3image_library}"
            IMPORTED_LOCATION "${_sdl3image_dll}"
            COMPATIBLE_INTERFACE_BOOL "SDL3_SHARED"
            INTERFACE_SDL3_SHARED "ON"
    )
endif()

unset(_sdl_arch_subdir)
unset(_sdl3image_incdir)
unset(_sdl3image_library)
unset(_sdl3image_dll)
