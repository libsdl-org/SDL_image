cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# See docs/release_checklist.md
set(MAJOR_VERSION 2)
set(MINOR_VERSION 6)
set(MICRO_VERSION 3)
set(SDL_REQUIRED_VERSION 2.0.9)

# For historical reasons this is 3.0.0 rather than the expected 1.0.0
set(DYLIB_COMPATIBILITY_VERSION "3.0.0")

include(PrivateSdlFunctions)
sdl_calculate_derived_version_variables()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the SDL_image source code and call cmake from there")
endif()

project(SDL2_image
    LANGUAGES C
    VERSION "${FULL_VERSION}"
)

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# Set defaults preventing destination file conflicts
set(SDL2IMAGE_DEBUG_POSTFIX "d"
    CACHE STRING "Name suffix for debug builds")
mark_as_advanced(SDL2IMAGE_DEBUG_POSTFIX)

# Assume MSVC projects don't have a package manager and need vendored dependencies (by default).
# Most other platforms have some kind of package manager.
# FIXME: consider a package manager such as conan/vcpkg instead of vendoring
if(MSVC)
    set(vendored_default ON)
else()
    set(vendored_default OFF)
endif()

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ON)
option(BUILD_SHARED_LIBS "Build the library as a shared library" ON)

option(SDL2IMAGE_INSTALL "Enable SDL2_image install target" ON)
option(SDL2IMAGE_DEPS_SHARED "Load dependencies dynamically" ON)
option(SDL2IMAGE_VENDORED "Use vendored third-party libraries" ${vendored_default})

option(SDL2IMAGE_SAMPLES "Build the SDL2_image sample program(s)" ON)
cmake_dependent_option(SDL2IMAGE_SAMPLES_INSTALL "Install the SDL2_image sample program(s)" OFF "SDL2IMAGE_SAMPLES;SDL2IMAGE_INSTALL" OFF)

option(SDL2IMAGE_TESTS "Build unit tests?" OFF)
cmake_dependent_option(SDL2IMAGE_TESTS_INSTALL "Install unit tests?" OFF "SDL2IMAGE_TESTS;SDL2IMAGE_INSTALL" OFF)

option(SDL2IMAGE_BACKEND_STB "Use stb_image for loading JPEG and PNG files" ON)
cmake_dependent_option(SDL2IMAGE_BACKEND_WIC "Add WIC backend (Windows Imaging Component)" OFF WIN32 OFF)
cmake_dependent_option(SDL2IMAGE_BACKEND_IMAGEIO "Use native Mac OS X frameworks for loading images" ON APPLE OFF)

option(SDL2IMAGE_AVIF "Support loading AVIF images" OFF)
option(SDL2IMAGE_BMP "Support loading BMP images" ON)
option(SDL2IMAGE_GIF "Support loading GIF images" ON)
option(SDL2IMAGE_JPG "Support loading JPEG images" ON)
option(SDL2IMAGE_JXL "Support loading JXL images" OFF)
option(SDL2IMAGE_LBM "Support loading LBM images" ON)
option(SDL2IMAGE_PCX "Support loading PCX images" ON)
option(SDL2IMAGE_PNG "Support loading PNG images" ON)
option(SDL2IMAGE_PNM "Support loading PNM images" ON)
option(SDL2IMAGE_QOI "Support loading QOI images" ON)
option(SDL2IMAGE_SVG "Support loading SVG images" ON)
option(SDL2IMAGE_TGA "Support loading TGA images" ON)
option(SDL2IMAGE_TIF "Support loading TIFF images" OFF)
option(SDL2IMAGE_WEBP "Support loading WEBP images" OFF)
option(SDL2IMAGE_XCF "Support loading XCF images" ON)
option(SDL2IMAGE_XPM "Support loading XPM images" ON)
option(SDL2IMAGE_XV "Support loading XV images" ON)

cmake_dependent_option(SDL2IMAGE_JPG_SAVE "Add JPEG save support" ON SDL2IMAGE_JPG OFF)
cmake_dependent_option(SDL2IMAGE_PNG_SAVE "Add PNG save support" ON SDL2IMAGE_PNG OFF)

set(LIBAVIF_MINIMUM_VERSION "0.9.1")
if(SDL2IMAGE_VENDORED AND SDL2IMAGE_AVIF)
    set(SDL2IMAGE_AVIF_VENDORED ON)
else()
    set(SDL2IMAGE_AVIF_VENDORED OFF)
endif()
cmake_dependent_option(SDL2IMAGE_AVIF_SHARED "Dynamically load AVIF support (requires shared libavif)"
    ${SDL2IMAGE_DEPS_SHARED} SDL2IMAGE_AVIF OFF)

if(SDL2IMAGE_VENDORED AND SDL2IMAGE_JPG AND NOT (SDL2IMAGE_BACKEND_WIC OR SDL2IMAGE_BACKEND_STB OR SDL2IMAGE_BACKEND_IMAGEIO))
    set(SDL2IMAGE_JPG_VENDORED ON)
else()
    set(SDL2IMAGE_JPG_VENDORED OFF)
endif()
cmake_dependent_option(SDL2IMAGE_JPG_SHARED "Dynamically load JPG support (requires shared libjpeg)"
    ${SDL2IMAGE_DEPS_SHARED} "SDL2IMAGE_JPG;NOT SDL2IMAGE_BACKEND_WIC;NOT SDL2IMAGE_BACKEND_STB;NOT SDL2IMAGE_BACKEND_IMAGEIO" OFF)

if(SDL2IMAGE_VENDORED AND SDL2IMAGE_JXL)
    set(SDL2IMAGE_JXL_VENDORED ON)
else()
    set(SDL2IMAGE_JXL_VENDORED OFF)
endif()
cmake_dependent_option(SDL2IMAGE_JXL_SHARED "Dynamically load JXL support (requires shared libjxl)"
    ${SDL2IMAGE_DEPS_SHARED} SDL2IMAGE_JXL OFF)

if(SDL2IMAGE_VENDORED AND SDL2IMAGE_PNG AND NOT (SDL2IMAGE_BACKEND_WIC OR SDL2IMAGE_BACKEND_STB OR SDL2IMAGE_BACKEND_IMAGEIO))
    set(SDL2IMAGE_PNG_VENDORED ON)
else()
    set(SDL2IMAGE_PNG_VENDORED OFF)
endif()
cmake_dependent_option(SDL2IMAGE_PNG_SHARED "Dynamically load PNG support (requires shared libpng)"
    ${SDL2IMAGE_DEPS_SHARED} "SDL2IMAGE_PNG;NOT SDL2IMAGE_BACKEND_WIC;NOT SDL2IMAGE_BACKEND_STB;NOT SDL2IMAGE_BACKEND_IMAGEIO" OFF)

if(SDL2IMAGE_VENDORED AND SDL2IMAGE_TIF)
    set(SDL2IMAGE_TIF_VENDORED ON)
else()
    set(SDL2IMAGE_TIF_VENDORED OFF)
endif()
cmake_dependent_option(SDL2IMAGE_TIF_SHARED "Dynamically load TIFF support (requires shared libtiff)"
    ${SDL2IMAGE_DEPS_SHARED} SDL2IMAGE_TIF OFF)

if(SDL2IMAGE_VENDORED AND SDL2IMAGE_WEBP)
    set(SDL2IMAGE_WEBP_VENDORED ON)
else()
    set(SDL2IMAGE_WEBP_VENDORED OFF)
endif()
cmake_dependent_option(SDL2IMAGE_WEBP_SHARED "Dynamically load WEBP support (requires shared libwebp)"
    ${SDL2IMAGE_DEPS_SHARED} SDL2IMAGE_WEBP OFF)

if(SDL2IMAGE_PNG_VENDORED)
    set(SDL2IMAGE_ZLIB ON)
else()
    set(SDL2IMAGE_ZLIB OFF)
endif()

if(SDL2IMAGE_VENDORED AND SDL2IMAGE_PNG_VENDORED)
    set(SDL2IMAGE_ZLIB_VENDORED ON)
else()
    set(SDL2IMAGE_ZLIB_VENDORED OFF)
endif()
if(SDL2IMAGE_PNG_SHARED)
    set(SDL2IMAGE_ZLIB_SHARED ON)
else()
    set(SDL2IMAGE_ZLIB_SHARED OFF)
endif()

# Save BUILD_SHARED_LIBS variable
set(SDL2IMAGE_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

if(SDL2IMAGE_BUILD_SHARED_LIBS)
    set(sdl2_image_export_name SDL2_image)
    set(sdl2_image_install_name_infix shared)
    set(sdl2_target_name SDL2::SDL2)
else()
    set(sdl2_image_export_name SDL2_image-static)
    set(sdl2_image_install_name_infix static)
    set(sdl2_target_name SDL2::SDL2-static)
endif()

sdl_find_sdl2(${sdl2_target_name} ${SDL_REQUIRED_VERSION})

# Set PROJECT_VERSION of subprojects to "" if it's project call does not set VERSION
cmake_policy(SET CMP0048 NEW)

# Allow cmake_dependent_option to use "Full Condition Syntax"
if(POLICY CMP0127)
    cmake_policy(SET CMP0127 NEW)
endif()

# OpenGL is required by dependencies of (dependencies of) some vendored libraries
if(NOT DEFINED OpenGL_GL_PREFERENCE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

add_library(SDL2_image
    IMG.c
    IMG_WIC.c
    IMG_avif.c
    IMG_bmp.c
    IMG_gif.c
    IMG_jpg.c
    IMG_jxl.c
    IMG_lbm.c
    IMG_pcx.c
    IMG_png.c
    IMG_pnm.c
    IMG_qoi.c
    IMG_stb.c
    IMG_svg.c
    IMG_tga.c
    IMG_tif.c
    IMG_webp.c
    IMG_xcf.c
    IMG_xpm.c
    IMG_xv.c
    IMG_xxx.c
)
add_library(SDL2_image::${sdl2_image_export_name} ALIAS SDL2_image)
target_include_directories(SDL2_image PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>"
)
target_compile_definitions(SDL2_image PRIVATE
    BUILD_SDL
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
)
target_link_libraries(SDL2_image PRIVATE ${sdl2_target_name})
if(WIN32 AND SDL2IMAGE_BUILD_SHARED_LIBS)
    target_sources(SDL2_image PRIVATE
        version.rc
    )
endif()
set_target_properties(SDL2_image PROPERTIES
    DEFINE_SYMBOL DLL_EXPORT
    PUBLIC_HEADER SDL_image.h
    EXPORT_NAME ${sdl2_image_export_name}
    C_VISIBILITY_PRESET "hidden"
)
if(NOT ANDROID)
    set_target_properties(SDL2_image PROPERTIES
        DEBUG_POSTFIX "${SDL2IMAGE_DEBUG_POSTFIX}"
    )
    if(APPLE)
        # the SOVERSION property corresponds to the compatibility version and VERSION corresponds to the current version
        # https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html#mach-o-versions
        set_target_properties(SDL2_image PROPERTIES
            SOVERSION "${DYLIB_COMPATIBILITY_VERSION}"
            VERSION "${DYLIB_CURRENT_VERSION}"
        )
    else()
        set_target_properties(SDL2_image PROPERTIES
            SOVERSION "${LT_MAJOR}"
            VERSION "${LT_VERSION}"
        )
    endif()
endif()
if(SDL2IMAGE_BUILD_SHARED_LIBS AND (APPLE OR (UNIX AND NOT ANDROID)))
    add_custom_command(TARGET SDL2_image POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E create_symlink "$<TARGET_SONAME_FILE_NAME:SDL2_image>" "libSDL2_image$<$<CONFIG:Debug>:${SDL2IMAGE_DEBUG_POSTFIX}>$<TARGET_FILE_SUFFIX:SDL2_image>"
        # BYPRODUCTS "libSDL2_image$<$<CONFIG:Debug>:${SDL2IMAGE_DEBUG_POSTFIX}>$<TARGET_FILE_SUFFIX:SDL2_image>" # Needs CMake 3.20
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    )
endif()
if(SDL2IMAGE_BUILD_SHARED_LIBS)
    if(WIN32 OR OS2)
        set_target_properties(SDL2_image PROPERTIES
            PREFIX ""
        )
    endif()
    if(OS2)
        # OS/2 doesn't support a DLL name longer than 8 characters.
        set_target_properties(SDL2_image PROPERTIES
            OUTPUT_NAME "SDL2img"
        )
    elseif(UNIX AND NOT ANDROID)
        set_target_properties(SDL2_image PROPERTIES
            OUTPUT_NAME "SDL2_image-${LT_RELEASE}"
        )
    endif()
else()
    if(MSVC OR (WATCOM AND (WIN32 OR OS2)))
        set_target_properties(SDL2_image PROPERTIES
            OUTPUT_NAME "SDL2_image-static"
        )
    endif()
endif()

# Use `Compatible Interface Properties` to ensure a shared SDL2_image is built with a shared SDL2
if(SDL2IMAGE_BUILD_SHARED_LIBS)
    set_property(TARGET SDL2_image PROPERTY INTERFACE_SDL2_SHARED ${SDL2IMAGE_BUILD_SHARED_LIBS})
    set_property(TARGET SDL2_image APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL2_SHARED)
endif()

if(SDL2IMAGE_BUILD_SHARED_LIBS)
    sdl_target_link_options_no_undefined(SDL2_image)
endif()

if(SDL2IMAGE_BUILD_SHARED_LIBS)
    # Make sure static library dependencies are built with -fPIC when building a shared SDL2_image
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(INSTALL_EXTRA_TARGETS)
set(PC_LIBS)
set(PC_REQUIRES)

if(SDL2IMAGE_BACKEND_STB)
    target_compile_definitions(SDL2_image PRIVATE USE_STBIMAGE)
endif()

if(APPLE)
    if(SDL2IMAGE_BACKEND_IMAGEIO)
        target_link_options(SDL2_image PRIVATE -Wl,-framework,ApplicationServices)
        target_link_libraries(SDL2_image PRIVATE objc)
        target_sources(SDL2_image PRIVATE
            IMG_ImageIO.m
        )
    else()
        target_compile_definitions(SDL2_image PRIVATE SDL_IMAGE_USE_COMMON_BACKEND)
    endif()
endif()

if(SDL2IMAGE_BACKEND_WIC)
    target_compile_definitions(SDL2_image PRIVATE SDL_IMAGE_USE_WIC_BACKEND)
endif()

if(SDL2IMAGE_ZLIB)
    if(SDL2IMAGE_ZLIB_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored zlib")
        sdl_check_project_in_subfolder(external/zlib zlib SDL2IMAGE_VENDORED)
        add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
        # PNG_BUILD_ZLIB variable is used by vendored libpng
        set(PNG_BUILD_ZLIB ON CACHE BOOL "libpng option to tell it should use 'our' vendored ZLIB library" FORCE)
        # ZLIB_INCLUDE_DIR variable is used by vendored libpng
        set(ZLIB_INCLUDE_DIR "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/zlib;${CMAKE_CURRENT_BINARY_DIR}/external/zlib>" CACHE STRING "path of zlib, passed to libpng" FORCE)
        # ZLIB_LIBRARY variable is used by vendored libpng
        if(SDL2IMAGE_ZLIB_SHARED)
            set(ZLIB_LIBRARY zlib)
        else()
            set(ZLIB_LIBRARY zlibstatic)
        endif()
        list(APPEND INSTALL_EXTRA_TARGETS ${ZLIB_LIBRARY})
    else()
        message(STATUS "${PROJECT_NAME}: Using system zlib")
        find_package(ZLIB REQUIRED)
    endif()
endif()

if(SDL2IMAGE_AVIF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_AVIF)
    if(SDL2IMAGE_AVIF_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libavif")
        message(FATAL_ERROR "libavif is not vendored (yet)")
        sdl_check_project_in_subfolder(external/libavif libavif SDL2IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_AVIF_SHARED})
        add_subdirectory(external/libavif EXCLUDE_FROM_ALL)
        list(APPEND INSTALL_EXTRA_TARGETS libavif)
        if(NOT SDL2IMAGE_AVIF_SHARED)
            list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:avif>)
        endif()
    else()
        message(STATUS "${PROJECT_NAME}: Using system libavif")
        find_package(libavif ${LIBAVIF_MINIMUM_VERSION} REQUIRED)
        list(APPEND PC_REQUIRES libavif)
    endif()
    if(SDL2IMAGE_AVIF_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:avif,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:avif,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:avif,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_avif avif)
        message(STATUS "Dynamic libavif: ${dynamic_avif}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_AVIF_DYNAMIC=\"${dynamic_avif}\"")
        if(SDL2IMAGE_AVIF_VENDORED)
            add_dependencies(SDL2_image avif)
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE avif)
    endif()
endif()

if(SDL2IMAGE_BMP)
    target_compile_definitions(SDL2_image PRIVATE LOAD_BMP)
endif()

if(SDL2IMAGE_GIF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_GIF)
endif()

if(SDL2IMAGE_JPG)
    target_compile_definitions(SDL2_image PRIVATE
        LOAD_JPG
        SDL_IMAGE_SAVE_JPG=$<BOOL:${SDL2IMAGE_JPG_SAVE}>
    )
    if(NOT SDL2IMAGE_BACKEND_STB AND NOT SDL2IMAGE_BACKEND_WIC AND NOT SDL2IMAGE_BACKEND_IMAGEIO)
        if(SDL2IMAGE_JPG_VENDORED)
            message(STATUS "${PROJECT_NAME}: Using vendored libjpeg")
            sdl_check_project_in_subfolder(external/jpeg libjpeg SDL2IMAGE_VENDORED)
            set(BUILD_SHARED_LIBS ${SDL2IMAGE_JPG_SHARED})
            add_subdirectory(external/jpeg EXCLUDE_FROM_ALL)
            list(APPEND INSTALL_EXTRA_TARGETS jpeg)
            if(NOT SDL2IMAGE_JPG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:jpeg>)
            endif()
        else()
            message(STATUS "${PROJECT_NAME}: Using system libjpeg")
            find_package(JPEG REQUIRED)
            list(APPEND PC_REQUIRES libjpeg)
        endif()
        if(SDL2IMAGE_JPG_SHARED)
            target_include_directories(SDL2_image PRIVATE
                $<TARGET_PROPERTY:JPEG::JPEG,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
            target_get_dynamic_library(dynamic_jpeg JPEG::JPEG)
            message(STATUS "Dynamic libjpeg: ${dynamic_jpeg}")
            target_compile_definitions(SDL2_image PRIVATE "LOAD_JPG_DYNAMIC=\"${dynamic_jpeg}\"")
            if(SDL2IMAGE_JPG_VENDORED)
                add_dependencies(SDL2_image JPEG::JPEG)
            endif()
        else()
            target_link_libraries(SDL2_image PRIVATE JPEG::JPEG)
        endif()
    endif()
endif()

if(SDL2IMAGE_JXL)
    target_compile_definitions(SDL2_image PRIVATE LOAD_JXL)
    if(SDL2IMAGE_JXL_VENDORED)
        enable_language(CXX)
        message(STATUS "${PROJECT_NAME}: Using vendored libjxl")
        set(BUILD_TESTING OFF)
        # JPEGXL_ENABLE_MANPAGES variable is used by libjxl
        set(JPEGXL_ENABLE_MANPAGES OFF CACHE BOOL "libjxl manpage option" FORCE)
        # JPEGXL_ENABLE_PLUGINS variable is used by libjxl
        set(JPEGXL_ENABLE_PLUGINS OFF CACHE BOOL "libjxl manpage option" FORCE)
        # JPEGXL_ENABLE_SKCMS variable is used by libjxl
        set(JPEGXL_ENABLE_SKCMS OFF CACHE BOOL "libjxl skcms option" FORCE)
        # JPEGXL_FORCE_SYSTEM_HWY variable is used by libjxl
        set(JPEGXL_FORCE_SYSTEM_HWY OFF CACHE BOOL "libjxl highway option" FORCE)
        sdl_check_project_in_subfolder(external/libjxl libjxl SDL2IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_JXL_SHARED})
        add_subdirectory(external/libjxl EXCLUDE_FROM_ALL)
        if(BUILD_SHARED_LIBS)
            set(jxl_lib jxl)
            list(APPEND INSTALL_EXTRA_TARGETS brotlidec brotlicommon brotlienc hwy ${jxl_lib})
            if(NOT SDL2IMAGE_JXL_SHARED)
                list(APPEND PC_LIBS
                    -l$<TARGET_FILE_BASE_NAME:brotlidec> -l$<TARGET_FILE_BASE_NAME:brotlicommon>
                    -l$<TARGET_FILE_BASE_NAME:brotlienc> -l$<TARGET_FILE_BASE_NAME:${jxl_lib}>
                )
            endif()
        else()
            set(jxl_lib jxl_dec-static)
            list(APPEND INSTALL_EXTRA_TARGETS brotlidec-static brotlicommon-static hwy ${jxl_lib})
        endif()
        if(NOT TARGET libjxl::libjxl)
            add_library(libjxl::libjxl ALIAS ${jxl_lib})
        endif()
    else()
        message(STATUS "${PROJECT_NAME}: Using system libjxl")
        list(APPEND PC_REQUIRES libjxl)
        find_package(libjxl REQUIRED)
    endif()
    if(SDL2IMAGE_JXL_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:libjxl::libjxl,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:libjxl::libjxl,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:libjxl::libjxl,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_jxl libjxl::libjxl)
        message(STATUS "Dynamic libjxl: ${dynamic_jxl}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_JXL_DYNAMIC=\"${dynamic_jxl}\"")
        if(SDL2IMAGE_JXL_VENDORED)
            add_dependencies(SDL2_image libjxl::libjxl)
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE libjxl::libjxl)
    endif()
endif()

if(SDL2IMAGE_LBM)
    target_compile_definitions(SDL2_image PRIVATE LOAD_LBM)
endif()

if(SDL2IMAGE_PCX)
    target_compile_definitions(SDL2_image PRIVATE LOAD_PCX)
endif()

if(SDL2IMAGE_PNG)
    target_compile_definitions(SDL2_image PRIVATE
        LOAD_PNG
        SDL_IMAGE_SAVE_PNG=$<BOOL:${SDL2IMAGE_PNG_SAVE}>
    )
    if(NOT SDL2IMAGE_BACKEND_STB AND NOT SDL2IMAGE_BACKEND_WIC AND NOT SDL2IMAGE_BACKEND_IMAGEIO)
        set(HAVE_LD_VERSION_SCRIPT OFF CACHE BOOL "" FORCE)
        if(SDL2IMAGE_PNG_VENDORED)
            message(STATUS "${PROJECT_NAME}: Using vendored libpng")
            sdl_check_project_in_subfolder(external/libpng libpng SDL2IMAGE_VENDORED)
            add_subdirectory(external/libpng EXCLUDE_FROM_ALL)
            if(SDL2IMAGE_PNG_SHARED)
                set(PNG_LIBRARY png)
            else()
                set(PNG_LIBRARY png_static)
            endif()
            add_library(PNG::PNG ALIAS ${PNG_LIBRARY})
            target_include_directories(SDL2_image PRIVATE external/libpng)
            list(APPEND INSTALL_EXTRA_TARGETS ${PNG_LIBRARY})
            if(NOT SDL2IMAGE_PNG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${PNG_LIBRARY}>)
                if(SDL2IMAGE_ZLIB_VENDORED)
                    list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${ZLIB_LIBRARY}>)
                else()
                    list(APPEND PC_REQUIRES zlib)
                endif()
            endif()
        else()
            message(STATUS "${PROJECT_NAME}: Using system libpng")
            find_package(PNG REQUIRED)
            list(APPEND PC_REQUIRES libpng)
        endif()
        if(SDL2IMAGE_PNG_SHARED)
            target_include_directories(SDL2_image PRIVATE
                $<TARGET_PROPERTY:PNG::PNG,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:PNG::PNG,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:PNG::PNG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
            target_get_dynamic_library(dynamic_png PNG::PNG)
            message(STATUS "Dynamic libpng: ${dynamic_png}")
            target_compile_definitions(SDL2_image PRIVATE "LOAD_PNG_DYNAMIC=\"${dynamic_png}\"")
            if(SDL2IMAGE_PNG_VENDORED)
                add_dependencies(SDL2_image PNG::PNG)
            endif()
        else()
            target_link_libraries(SDL2_image PRIVATE PNG::PNG)
        endif()
    endif()
endif()

if(SDL2IMAGE_PNM)
    target_compile_definitions(SDL2_image PRIVATE LOAD_PNM)
endif()

if(SDL2IMAGE_QOI)
    target_compile_definitions(SDL2_image PRIVATE LOAD_QOI)
endif()

if(SDL2IMAGE_SVG)
    target_compile_definitions(SDL2_image PRIVATE LOAD_SVG)
endif()

if(SDL2IMAGE_TGA)
    target_compile_definitions(SDL2_image PRIVATE LOAD_TGA)
endif()

if(SDL2IMAGE_TIF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_TIF)
    if(SDL2IMAGE_TIF_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libtiff")
        # jpeg variable is used by vendored libtiff
        set(jpeg OFF CACHE BOOL "libtiff: jpeg option" FORCE)
        # libdeflate variable is used by vendored libtiff
        set(libdeflate OFF CACHE BOOL "libtiff: libdeflate option" FORCE)
        # DEFLATE_FOUND variable is used by vendored libtiff
        set(DEFLATE_FOUND OFF CACHE BOOL "libtiff: libdeflate option" FORCE)
        # zlib variable is used by vendored libtiff (controls use of `find_package`)
        set(zlib OFF CACHE BOOL "libtiff: find zlib using find_package" FORCE)
        # ZLIB_FOUND is used by vendored libtiff
        set(ZLIB_FOUND "")
        # lzma variable is used by vendored libtiff
        set(lzma OFF CACHE BOOL "libtiff: lzma option" FORCE)
        # webp variable is used by vendored libtiff
        set(webp OFF CACHE BOOL "libtiff: webp option" FORCE)
        # zstd variable is used by vendored libtiff
        set(zstd OFF CACHE BOOL "libtiff: zstd option" FORCE)
        # ZSTD_FOUND variable is used by vendored libtiff
        set(ZSTD_FOUND OFF)
        # WEBP_LIBRARY variable is used by vendored libtiff
        set(WEBP_LIBRARY "")
        sdl_check_project_in_subfolder(external/libtiff libtiff SDL2IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_TIF_SHARED})
        add_subdirectory(external/libtiff EXCLUDE_FROM_ALL)
        add_library(TIFF::TIFF ALIAS tiff)
        list(APPEND INSTALL_EXTRA_TARGETS tiff)
        if(NOT SDL2IMAGE_TIF_SHARED)
            list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:tiff>)
        endif()
    else()
        message(STATUS "${PROJECT_NAME}: Using system libtiff")
        find_package(TIFF REQUIRED)
        list(APPEND PC_REQUIRES libtiff-4)
    endif()
    if(SDL2IMAGE_TIF_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:TIFF::TIFF,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_tif TIFF::TIFF)
        message(STATUS "Dynamic libtiff: ${dynamic_tif}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_TIF_DYNAMIC=\"${dynamic_tif}\"")
        if(SDL2IMAGE_TIF_VENDORED)
            add_dependencies(SDL2_image TIFF::TIFF)
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE TIFF::TIFF)
    endif()
endif()

if(SDL2IMAGE_WEBP)
    target_compile_definitions(SDL2_image PRIVATE LOAD_WEBP)
    # missing cpufeatures
    if(SDL2IMAGE_WEBP_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libwebp")
        sdl_check_project_in_subfolder(external/libwebp libwebp SDL2IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_WEBP_SHARED})
        add_subdirectory(external/libwebp EXCLUDE_FROM_ALL)
        target_include_directories(SDL2_image PRIVATE external/libwebp/src)
        add_library(WebP::webp ALIAS webp)
        list(APPEND INSTALL_EXTRA_TARGETS webp)
    else()
        message(STATUS "${PROJECT_NAME}: Using system libwebp")
        find_package(webp REQUIRED)
        list(APPEND PC_REQUIRES libwebp)
    endif()
    if(SDL2IMAGE_WEBP_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:WebP::webp,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webp,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webp,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_webp WebP::webp)
        message(STATUS "Dynamic libwebp: ${dynamic_webp}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_WEBP_DYNAMIC=\"${dynamic_webp}\"")
        if(SDL2IMAGE_WEBP_VENDORED)
            add_dependencies(SDL2_image WebP::webp)
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE WebP::webp)
    endif()
endif()

if(SDL2IMAGE_XCF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_XCF)
endif()

if(SDL2IMAGE_XPM)
    target_compile_definitions(SDL2_image PRIVATE LOAD_XPM)
endif()

if(SDL2IMAGE_XV)
    target_compile_definitions(SDL2_image PRIVATE LOAD_XV)
endif()

# Restore BUILD_SHARED_LIBS
set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})

if(SDL2IMAGE_INSTALL)
    install(
        TARGETS SDL2_image
        EXPORT SDL2ImageExports
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL2" COMPONENT devel
    )

    if(INSTALL_EXTRA_TARGETS)
        install(
            TARGETS ${INSTALL_EXTRA_TARGETS}
            EXPORT SDL2ImageExports
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT devel
        )
    endif()

    if(APPLE)
        set(PKG_PREFIX "SDL2_image.framework/Resources")
    elseif(WIN32 AND NOT MINGW)
        set(PKG_PREFIX "cmake")
    else()
        set(PKG_PREFIX "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_image")
    endif()

    configure_package_config_file(SDL2_imageConfig.cmake.in SDL2_imageConfig.cmake
        INSTALL_DESTINATION "${PKG_PREFIX}"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL2_imageConfigVersion.cmake"
        VERSION ${FULL_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/SDL2_imageConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/SDL2_imageConfigVersion.cmake"
            cmake/Findlibjxl.cmake
            cmake/Findwebp.cmake
        DESTINATION "${PKG_PREFIX}"
        COMPONENT devel
    )
    install(EXPORT SDL2ImageExports
        FILE SDL2_image-${sdl2_image_install_name_infix}-targets.cmake
        NAMESPACE SDL2_image::
        DESTINATION "${PKG_PREFIX}"
        COMPONENT devel
    )

    if(SDL2IMAGE_BUILD_SHARED_LIBS)
        # Only create a .pc file for a shared SDL2_image
        set(prefix "${CMAKE_INSTALL_PREFIX}")
        set(exec_prefix "\${prefix}")
        set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
        set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
        set(PACKAGE "${PROJECT_NAME}")
        set(VERSION "${FULL_VERSION}")
        set(SDL_VERSION "${SDL_REQUIRED_VERSION}")
        string(JOIN " " PC_REQUIRES ${PC_REQUIRES})
        string(JOIN " " PC_LIBS ${PC_LIBS})
        configure_file("${PROJECT_SOURCE_DIR}/SDL2_image.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/SDL2_image.pc.intermediate" @ONLY)
        file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SDL2_image-$<CONFIG>.pc" INPUT "${CMAKE_CURRENT_BINARY_DIR}/SDL2_image.pc.intermediate")

        set(PC_DESTDIR)
        if(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
            # FreeBSD uses ${PREFIX}/libdata/pkgconfig
            set(PC_DESTDIR "libdata/pkgconfig")
        else()
            set(PC_DESTDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
        endif()
        # Only install a SDL2_image.pc file in Release mode
        install(CODE "
            # FIXME: use file(COPY_FILE) if minimum CMake version >= 3.21
            execute_process(COMMAND \"\${CMAKE_COMMAND}\" -E copy_if_different
                \"${CMAKE_CURRENT_BINARY_DIR}/SDL2_image-$<CONFIG>.pc\"
                \"${CMAKE_CURRENT_BINARY_DIR}/SDL2_image.pc\")
            file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${PC_DESTDIR}\"
                TYPE FILE
                FILES \"${CMAKE_CURRENT_BINARY_DIR}/SDL2_image.pc\")" CONFIG Release COMPONENT devel)
    endif()

    if(SDL2IMAGE_BUILD_SHARED_LIBS AND (APPLE OR (UNIX AND NOT ANDROID)))
        install(
            FILES
                "${PROJECT_BINARY_DIR}/libSDL2_image$<$<CONFIG:Debug>:${SDL2IMAGE_DEBUG_POSTFIX}>$<TARGET_FILE_SUFFIX:SDL2_image>"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT devel
        )
    endif()

    install(FILES "LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
        COMPONENT library
    )
endif()

if(SDL2IMAGE_SAMPLES)
    add_executable(showanim showanim.c)
    add_executable(showimage showimage.c)

    find_package(SDL2main)

    foreach(prog showanim showimage)
        # FIXME: mingw should be handled by SDL2::SDL2(-static) target
        if(MINGW)
            target_link_libraries(${prog} PRIVATE mingw32)
            target_link_options(${prog} PRIVATE -mwindows)
        endif()
        target_link_libraries(${prog} PRIVATE SDL2_image::${sdl2_image_export_name})
        if(TARGET SDL2::SDL2main)
            target_link_libraries(${prog} PRIVATE SDL2::SDL2main)
        endif()
        target_link_libraries(${prog} PRIVATE ${sdl2_target_name})

        if(SDL2IMAGE_SAMPLES_INSTALL)
            install(TARGETS ${prog}
                RUNTIME DESTINATION "{CMAKE_INSTALL_BINDIR}"
            )
        endif()
    endforeach()
endif()

add_library(SDL2::image INTERFACE IMPORTED GLOBAL)
set_target_properties(SDL2::image PROPERTIES
    INTERFACE_LINK_LIBRARIES "SDL2_image"
)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17")
    set_target_properties(SDL2::image PROPERTIES
        DEPRECATION "Use SDL2_image::SDL2_image or SDL2_image::SDL2_image-static instead"
    )
endif()

if(SDL2IMAGE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
