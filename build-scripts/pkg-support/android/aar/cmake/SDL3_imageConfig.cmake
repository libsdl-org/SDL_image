# SDL CMake configuration file:
# This file is meant to be placed in lib/cmake/SDL3_image subfolder of a reconstructed Android SDL3_image SDK

cmake_minimum_required(VERSION 3.0...3.28)

include(FeatureSummary)
set_package_properties(SDL3_image PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_image/"
    DESCRIPTION "SDL_image is an image file loading library"
)

# Copied from `configure_package_config_file`
macro(set_and_check _var _file)
    set(${_var} "${_file}")
    if(NOT EXISTS "${_file}")
        message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
    endif()
endmacro()

set(SDLIMAGE_AVIF  FALSE)
set(SDLIMAGE_BMP   TRUE)
set(SDLIMAGE_GIF   TRUE)
set(SDLIMAGE_JPG   TRUE)
set(SDLIMAGE_JXL   FALSE)
set(SDLIMAGE_LBM   TRUE)
set(SDLIMAGE_PCX   TRUE)
set(SDLIMAGE_PNG   TRUE)
set(SDLIMAGE_PNM   TRUE)
set(SDLIMAGE_QOI   TRUE)
set(SDLIMAGE_SVG   TRUE)
set(SDLIMAGE_TGA   TRUE)
set(SDLIMAGE_TIF   FALSE)
set(SDLIMAGE_XCF   FALSE)
set(SDLIMAGE_XPM   TRUE)
set(SDLIMAGE_XV    TRUE)
set(SDLIMAGE_WEBP  FALSE)

set(SDLIMAGE_JPG_SAVE FALSE)
set(SDLIMAGE_PNG_SAVE FALSE)

set(SDLIMAGE_VENDORED  FALSE)

set(SDLIMAGE_BACKEND_IMAGEIO   FALSE)
set(SDLIMAGE_BACKEND_STB       TRUE)
set(SDLIMAGE_BACKEND_WIC       FALSE)

# Copied from `configure_package_config_file`
macro(check_required_components _NAME)
    foreach(comp ${${_NAME}_FIND_COMPONENTS})
        if(NOT ${_NAME}_${comp}_FOUND)
            if(${_NAME}_FIND_REQUIRED_${comp})
                set(${_NAME}_FOUND FALSE)
            endif()
        endif()
    endforeach()
endmacro()

set(SDL3_image_FOUND TRUE)

if(SDL_CPU_X86)
    set(_sdl_arch_subdir "x86")
elseif(SDL_CPU_X64)
    set(_sdl_arch_subdir "x86_64")
elseif(SDL_CPU_ARM32)
    set(_sdl_arch_subdir "armeabi-v7a")
elseif(SDL_CPU_ARM64)
    set(_sdl_arch_subdir "arm64-v8a")
else()
    set(SDL3_image_FOUND FALSE)
    return()
endif()

get_filename_component(_sdl3image_prefix "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(_sdl3image_prefix "${_sdl3image_prefix}/.." ABSOLUTE)
get_filename_component(_sdl3image_prefix "${_sdl3image_prefix}/.." ABSOLUTE)
set_and_check(_sdl3image_prefix          "${_sdl3image_prefix}")
set_and_check(_sdl3image_include_dirs    "${_sdl3image_prefix}/include")

set_and_check(_sdl3image_lib             "${_sdl3image_prefix}/lib/${_sdl_arch_subdir}/libSDL3_image.so")

unset(_sdl_arch_subdir)
unset(_sdl3image_prefix)

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL3_image-target.cmake files.

if(EXISTS "${_sdl3image_lib}")
    if(NOT TARGET SDL3_image::SDL3_image-shared)
        add_library(SDL3_image::SDL3_image-shared SHARED IMPORTED)
        set_target_properties(SDL3_image::SDL3_image-shared
            PROPERTIES
                IMPORTED_LOCATION "${_sdl3image_lib}"
                INTERFACE_INCLUDE_DIRECTORIES "${_sdl3image_include_dirs}"
                COMPATIBLE_INTERFACE_BOOL "SDL3_SHARED"
                INTERFACE_SDL3_SHARED "ON"
                COMPATIBLE_INTERFACE_STRING "SDL_VERSION"
                INTERFACE_SDL_VERSION "SDL3"
    )
    endif()
    set(SDL3_image_SDL3_image-shared_FOUND TRUE)
else()
    set(SDL3_image_SDL3_image-shared_FOUND FALSE)
endif()
unset(_sdl3image_lib)
unset(_sdl3image_include_dirs)

set(SDL3_image_SDL3_image-static_FOUND FALSE)

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
