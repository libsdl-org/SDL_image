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

set(SDLIMAGE_AVIF  TRUE)
set(SDLIMAGE_BMP   TRUE)
set(SDLIMAGE_GIF   TRUE)
set(SDLIMAGE_JPG   TRUE)
set(SDLIMAGE_JXL   TRUE)
set(SDLIMAGE_LBM   TRUE)
set(SDLIMAGE_PCX   TRUE)
set(SDLIMAGE_PNG   TRUE)
set(SDLIMAGE_PNM   TRUE)
set(SDLIMAGE_QOI   TRUE)
set(SDLIMAGE_SVG   TRUE)
set(SDLIMAGE_TGA   TRUE)
set(SDLIMAGE_TIF   TRUE)
set(SDLIMAGE_XCF   TRUE)
set(SDLIMAGE_XPM   TRUE)
set(SDLIMAGE_XV    TRUE)
set(SDLIMAGE_WEBP  TRUE)

set(SDLIMAGE_JPG_SAVE TRUE)
set(SDLIMAGE_PNG_SAVE TRUE)

set(SDLIMAGE_VENDORED  FALSE)

set(SDLIMAGE_BACKEND_IMAGEIO   TRUE)
set(SDLIMAGE_BACKEND_STB       FALSE)
set(SDLIMAGE_BACKEND_WIC       FALSE)

# Compute the installation prefix relative to this file.
set(_sdl3_image_framework_path "${CMAKE_CURRENT_LIST_DIR}")                                     # > /SDL3_image.framework/Resources/CMake/
get_filename_component(_sdl3_image_framework_path "${_sdl3_image_framework_path}" REALPATH)     # > /SDL3_image.framework/Versions/Current/Resources/CMake
get_filename_component(_sdl3_image_framework_path "${_sdl3_image_framework_path}" REALPATH)     # > /SDL3_image.framework/Versions/A/Resources/CMake/
get_filename_component(_sdl3_image_framework_path "${_sdl3_image_framework_path}" PATH)         # > /SDL3_image.framework/Versions/A/Resources/
get_filename_component(_sdl3_image_framework_path "${_sdl3_image_framework_path}" PATH)         # > /SDL3_image.framework/Versions/A/
get_filename_component(_sdl3_image_framework_path "${_sdl3_image_framework_path}" PATH)         # > /SDL3_image.framework/Versions/
get_filename_component(_sdl3_image_framework_path "${_sdl3_image_framework_path}" PATH)         # > /SDL3_image.framework/
get_filename_component(_sdl3_image_framework_parent_path "${_sdl3_image_framework_path}" PATH)  # > /

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL3_image-target.cmake files.

if(NOT TARGET SDL3_image::SDL3_image-shared)
    add_library(SDL3_image::SDL3_image-shared SHARED IMPORTED)
    set_target_properties(SDL3_image::SDL3_image-shared
        PROPERTIES
            FRAMEWORK "TRUE"
            IMPORTED_LOCATION "${_sdl3_image_framework_path}/SDL3_image"
            COMPATIBLE_INTERFACE_BOOL "SDL3_SHARED"
            INTERFACE_SDL3_SHARED "ON"
            COMPATIBLE_INTERFACE_STRING "SDL_VERSION"
            INTERFACE_SDL_VERSION "SDL3"
    )
endif()
set(SDL3_image_SDL3_image-shared_FOUND TRUE)

set(SDL3_image_SDL3_image-static FALSE)

unset(_sdl3_image_framework_path)
unset(_sdl3_image_framework_parent_path)

if(SDL3_image_SDL3_image-shared_FOUND)
    set(SDL3_image_SDL3_image_FOUND TRUE)
endif()

function(_sdl_create_target_alias_compat NEW_TARGET TARGET)
    if(CMAKE_VERSION VERSION_LESS "3.18")
        # Aliasing local targets is not supported on CMake < 3.18, so make it global.
        add_library(${NEW_TARGET} INTERFACE IMPORTED)
        set_target_properties(${NEW_TARGET} PROPERTIES INTERFACE_LINK_LIBRARIES "${TARGET}")
    else()
        add_library(${NEW_TARGET} ALIAS ${TARGET})
    endif()
endfunction()

# Make sure SDL3_image::SDL3_image always exists
if(NOT TARGET SDL3_image::SDL3_image)
    if(TARGET SDL3_image::SDL3_image-shared)
        _sdl_create_target_alias_compat(SDL3_image::SDL3_image SDL3_image::SDL3_image-shared)
    endif()
endif()

check_required_components(SDL3_image)
