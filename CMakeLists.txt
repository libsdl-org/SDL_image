cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# See docs/release_checklist.md
set(MAJOR_VERSION 3)
set(MINOR_VERSION 0)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION 3.0.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the SDL_image source code and call cmake from there")
endif()

project(SDL3_image
    LANGUAGES C
    VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
)

include(PrivateSdlFunctions)
sdl_calculate_derived_version_variables(${MAJOR_VERSION} ${MINOR_VERSION} ${MICRO_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SDL3IMAGE_ROOTPROJECT ON)
else()
    set(SDL3IMAGE_ROOTPROJECT OFF)
endif()

set(SDL3IMAGE_SAMPLES_DEFAULT ${SDL3IMAGE_ROOTPROJECT})
if(ANDROID)
    set(SDL3IMAGE_SAMPLES_DEFAULT OFF)
endif()

# Assume MSVC projects don't have a package manager and need vendored dependencies (by default).
# Most other platforms have some kind of package manager.
# FIXME: consider a package manager such as conan/vcpkg instead of vendoring
if(MSVC)
    set(vendored_default ON)
else()
    set(vendored_default OFF)
endif()

set(sdl3image_install_enableable ON)
if ((TARGET SDL3 OR TARGET SDL3-static) AND SDL3_DISABLE_INSTALL)
    # Cannot install SDL3_image when SDL3 is built in same built, and is not installed.
    set(sdl3image_install_enableable OFF)
endif()

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ON)
option(BUILD_SHARED_LIBS "Build the library as a shared library" ON)

cmake_dependent_option(SDL3IMAGE_INSTALL "Enable SDL3_image install target" ${SDL3IMAGE_ROOTPROJECT} "${sdl3image_install_enableable}" OFF)
option(SDL3IMAGE_DEPS_SHARED "Load dependencies dynamically" ON)
option(SDL3IMAGE_VENDORED "Use vendored third-party libraries" ${vendored_default})
option(SDL3IMAGE_WERROR "Treat warnings as errors" OFF)

option(SDL3IMAGE_SAMPLES "Build the SDL3_image sample program(s)" ${SDL3IMAGE_SAMPLES_DEFAULT})
cmake_dependent_option(SDL3IMAGE_SAMPLES_INSTALL "Install the SDL3_image sample program(s)" OFF "SDL3IMAGE_SAMPLES;SDL3IMAGE_INSTALL" OFF)

option(SDL3IMAGE_TESTS "Build unit tests?" OFF)

option(SDL3IMAGE_BACKEND_STB "Use stb_image for loading JPEG and PNG files" ON)
cmake_dependent_option(SDL3IMAGE_BACKEND_WIC "Add WIC backend (Windows Imaging Component)" OFF WIN32 OFF)
cmake_dependent_option(SDL3IMAGE_BACKEND_IMAGEIO "Use native Mac OS X frameworks for loading images" ON APPLE OFF)

option(SDL3IMAGE_AVIF "Support loading AVIF images" OFF)
option(SDL3IMAGE_BMP "Support loading BMP images" ON)
option(SDL3IMAGE_GIF "Support loading GIF images" ON)
option(SDL3IMAGE_JPG "Support loading JPEG images" ON)
option(SDL3IMAGE_JXL "Support loading JXL images" OFF)
option(SDL3IMAGE_LBM "Support loading LBM images" ON)
option(SDL3IMAGE_PCX "Support loading PCX images" ON)
option(SDL3IMAGE_PNG "Support loading PNG images" ON)
option(SDL3IMAGE_PNM "Support loading PNM images" ON)
option(SDL3IMAGE_QOI "Support loading QOI images" ON)
option(SDL3IMAGE_SVG "Support loading SVG images" ON)
option(SDL3IMAGE_TGA "Support loading TGA images" ON)
option(SDL3IMAGE_TIF "Support loading TIFF images" OFF)
option(SDL3IMAGE_WEBP "Support loading WEBP images" OFF)
option(SDL3IMAGE_XCF "Support loading XCF images" ON)
option(SDL3IMAGE_XPM "Support loading XPM images" ON)
option(SDL3IMAGE_XV "Support loading XV images" ON)

cmake_dependent_option(SDL3IMAGE_JPG_SAVE "Add JPEG save support" ON SDL3IMAGE_JPG OFF)
cmake_dependent_option(SDL3IMAGE_PNG_SAVE "Add PNG save support" ON SDL3IMAGE_PNG OFF)

set(LIBAVIF_MINIMUM_VERSION "0.9.1")
if(SDL3IMAGE_VENDORED AND SDL3IMAGE_AVIF)
    set(SDL3IMAGE_AVIF_VENDORED ON)
else()
    set(SDL3IMAGE_AVIF_VENDORED OFF)
endif()
cmake_dependent_option(SDL3IMAGE_AVIF_SHARED "Dynamically load AVIF support (requires shared libavif)"
    ${SDL3IMAGE_DEPS_SHARED} SDL3IMAGE_AVIF OFF)

if(SDL3IMAGE_VENDORED AND SDL3IMAGE_JPG AND NOT (SDL3IMAGE_BACKEND_WIC OR SDL3IMAGE_BACKEND_STB OR SDL3IMAGE_BACKEND_IMAGEIO))
    set(SDL3IMAGE_JPG_VENDORED ON)
else()
    set(SDL3IMAGE_JPG_VENDORED OFF)
endif()
cmake_dependent_option(SDL3IMAGE_JPG_SHARED "Dynamically load JPG support (requires shared libjpeg)"
    ${SDL3IMAGE_DEPS_SHARED} "SDL3IMAGE_JPG;NOT SDL3IMAGE_BACKEND_WIC;NOT SDL3IMAGE_BACKEND_STB;NOT SDL3IMAGE_BACKEND_IMAGEIO" OFF)

if(SDL3IMAGE_VENDORED AND SDL3IMAGE_JXL)
    set(SDL3IMAGE_JXL_VENDORED ON)
else()
    set(SDL3IMAGE_JXL_VENDORED OFF)
endif()
cmake_dependent_option(SDL3IMAGE_JXL_SHARED "Dynamically load JXL support (requires shared libjxl)"
    ${SDL3IMAGE_DEPS_SHARED} SDL3IMAGE_JXL OFF)

if(SDL3IMAGE_VENDORED AND SDL3IMAGE_PNG AND NOT (SDL3IMAGE_BACKEND_WIC OR SDL3IMAGE_BACKEND_STB OR SDL3IMAGE_BACKEND_IMAGEIO))
    set(SDL3IMAGE_PNG_VENDORED ON)
else()
    set(SDL3IMAGE_PNG_VENDORED OFF)
endif()
cmake_dependent_option(SDL3IMAGE_PNG_SHARED "Dynamically load PNG support (requires shared libpng)"
    ${SDL3IMAGE_DEPS_SHARED} "SDL3IMAGE_PNG;NOT SDL3IMAGE_BACKEND_WIC;NOT SDL3IMAGE_BACKEND_STB;NOT SDL3IMAGE_BACKEND_IMAGEIO" OFF)

if(SDL3IMAGE_VENDORED AND SDL3IMAGE_TIF)
    set(SDL3IMAGE_TIF_VENDORED ON)
else()
    set(SDL3IMAGE_TIF_VENDORED OFF)
endif()
cmake_dependent_option(SDL3IMAGE_TIF_SHARED "Dynamically load TIFF support (requires shared libtiff)"
    ${SDL3IMAGE_DEPS_SHARED} SDL3IMAGE_TIF OFF)

if(SDL3IMAGE_VENDORED AND SDL3IMAGE_WEBP)
    set(SDL3IMAGE_WEBP_VENDORED ON)
else()
    set(SDL3IMAGE_WEBP_VENDORED OFF)
endif()
cmake_dependent_option(SDL3IMAGE_WEBP_SHARED "Dynamically load WEBP support (requires shared libwebp)"
    ${SDL3IMAGE_DEPS_SHARED} SDL3IMAGE_WEBP OFF)

if(SDL3IMAGE_PNG_VENDORED)
    set(SDL3IMAGE_ZLIB ON)
else()
    set(SDL3IMAGE_ZLIB OFF)
endif()

if(SDL3IMAGE_VENDORED AND SDL3IMAGE_PNG_VENDORED)
    set(SDL3IMAGE_ZLIB_VENDORED ON)
else()
    set(SDL3IMAGE_ZLIB_VENDORED OFF)
endif()
if(SDL3IMAGE_PNG_SHARED)
    set(SDL3IMAGE_ZLIB_SHARED ON)
else()
    set(SDL3IMAGE_ZLIB_SHARED OFF)
endif()

# Save BUILD_SHARED_LIBS variable
set(SDL3IMAGE_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

if(SDL3IMAGE_BUILD_SHARED_LIBS)
    set(sdl3_image_export_name SDL3_image)
    set(sdl3_image_install_name_infix shared)
    set(sdl3_target_name SDL3::SDL3)
else()
    set(sdl3_image_export_name SDL3_image-static)
    set(sdl3_image_install_name_infix static)
    set(sdl3_target_name SDL3::SDL3-static)
endif()

if(NOT TARGET ${sdl3_target_name})
    find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED)
endif()

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

add_library(sdl3image_build_options INTERFACE)
if(MSVC)
    target_compile_options(sdl3image_build_options INTERFACE /W2)
else()
    target_compile_options(sdl3image_build_options INTERFACE -Wall -Wextra)
endif()
if(SDL3IMAGE_WERROR)
    if(MSVC)
        target_compile_options(sdl3image_build_options INTERFACE /WX)
    else()
        target_compile_options(sdl3image_build_options INTERFACE -Werror)
    endif()
endif()

set(BUILD_SHARED_LIBS ${SDL3IMAGE_BUILD_SHARED_LIBS})
add_library(SDL3_image
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
add_library(SDL3_image::${sdl3_image_export_name} ALIAS SDL3_image)
target_include_directories(SDL3_image PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL3>"
)
target_compile_definitions(SDL3_image PRIVATE
    BUILD_SDL
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
)
target_link_libraries(SDL3_image PRIVATE $<BUILD_INTERFACE:${sdl3_target_name}>)
target_link_libraries(SDL3_image PRIVATE $<BUILD_INTERFACE:sdl3image_build_options>)
if(WIN32 AND SDL3IMAGE_BUILD_SHARED_LIBS)
    target_sources(SDL3_image PRIVATE
        version.rc
    )
endif()
set_target_properties(SDL3_image PROPERTIES
    DEFINE_SYMBOL DLL_EXPORT
    EXPORT_NAME ${sdl3_image_export_name}
    C_VISIBILITY_PRESET "hidden"
)

sdl_target_link_option_version_file(SDL3_image "${CMAKE_CURRENT_SOURCE_DIR}/SDL_image.sym")

if(NOT ANDROID)
    if(APPLE)
        # the SOVERSION property corresponds to the compatibility version and VERSION corresponds to the current version
        # https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html#mach-o-versions
        set_target_properties(SDL3_image PROPERTIES
            SOVERSION "${DYLIB_COMPAT_VERSION}"
            VERSION "${DYLIB_CURRENT_VERSION}"
        )
    else()
        set_target_properties(SDL3_image PROPERTIES
            SOVERSION "${SO_VERSION_MAJOR}"
            VERSION "${SO_VERSION}"
        )
    endif()
endif()
if(SDL3IMAGE_BUILD_SHARED_LIBS)
    if(WIN32)
        set_target_properties(SDL3_image PROPERTIES
            PREFIX ""
        )
    endif()
else()
    if(MSVC)
        set_target_properties(SDL3_image PROPERTIES
            OUTPUT_NAME "SDL3_image-static"
        )
    endif()
endif()

# Use `Compatible Interface Properties` to ensure a shared SDL3_image is built with a shared SDL3
if(SDL3IMAGE_BUILD_SHARED_LIBS)
    set_property(TARGET SDL3_image PROPERTY INTERFACE_SDL3_SHARED ${SDL3IMAGE_BUILD_SHARED_LIBS})
    set_property(TARGET SDL3_image APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL3_SHARED)
endif()

if(SDL3IMAGE_BUILD_SHARED_LIBS)
    sdl_target_link_options_no_undefined(SDL3_image)
endif()

if(SDL3IMAGE_BUILD_SHARED_LIBS)
    # Make sure static library dependencies are built with -fPIC when building a shared SDL3_image
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(INSTALL_EXTRA_TARGETS)
set(PC_LIBS)
set(PC_REQUIRES)

if(SDL3IMAGE_BACKEND_STB)
    target_compile_definitions(SDL3_image PRIVATE USE_STBIMAGE)
endif()

if(APPLE)
    if(SDL3IMAGE_BACKEND_IMAGEIO)
        target_link_libraries(SDL3_image PRIVATE -Wl,-framework,ApplicationServices)
        target_link_libraries(SDL3_image PRIVATE objc)
        target_sources(SDL3_image PRIVATE
            IMG_ImageIO.m
        )

        if (SDL3IMAGE_PNG AND NOT SDL3IMAGE_BACKEND_STB)
            target_compile_definitions(SDL3_image PRIVATE PNG_USES_IMAGEIO)
        endif()
        if (SDL3IMAGE_JPG AND NOT SDL3IMAGE_BACKEND_STB)
            target_compile_definitions(SDL3_image PRIVATE JPG_USES_IMAGEIO)
        endif()
    else()
        target_compile_definitions(SDL3_image PRIVATE SDL_IMAGE_USE_COMMON_BACKEND)
    endif()
endif()

if(SDL3IMAGE_BACKEND_WIC)
    target_compile_definitions(SDL3_image PRIVATE SDL_IMAGE_USE_WIC_BACKEND)
endif()

if(SDL3IMAGE_ZLIB)
    if(SDL3IMAGE_ZLIB_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored zlib")
        sdl_check_project_in_subfolder(external/zlib zlib SDL3IMAGE_VENDORED)
        add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
        # PNG_BUILD_ZLIB variable is used by vendored libpng
        set(PNG_BUILD_ZLIB ON CACHE BOOL "libpng option to tell it should use 'our' vendored ZLIB library" FORCE)
        # ZLIB_INCLUDE_DIR variable is used by vendored libpng
        set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib;${CMAKE_CURRENT_BINARY_DIR}/external/zlib" CACHE STRING "path of zlib, passed to libpng" FORCE)
        # ZLIB_LIBRARY variable is used by vendored libpng
        if(SDL3IMAGE_ZLIB_SHARED)
            set(ZLIB_LIBRARY zlib)
        else()
            set(ZLIB_LIBRARY zlibstatic)
        endif()
        list(APPEND INSTALL_EXTRA_TARGETS ${ZLIB_LIBRARY})
        set_target_properties(${ZLIB_LIBRARY} PROPERTIES EXPORT_NAME external_zlib)
        add_library(SDL3_image::external_zlib ALIAS ${ZLIB_LIBRARY})
    else()
        message(STATUS "${PROJECT_NAME}: Using system zlib")
        find_package(ZLIB REQUIRED)
    endif()
endif()

if(SDL3IMAGE_AVIF)
    target_compile_definitions(SDL3_image PRIVATE LOAD_AVIF)
    if(SDL3IMAGE_AVIF_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libavif")
        message(FATAL_ERROR "libavif is not vendored (yet)")
        sdl_check_project_in_subfolder(external/libavif libavif SDL3IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL3IMAGE_AVIF_SHARED})
        add_subdirectory(external/libavif EXCLUDE_FROM_ALL)
        list(APPEND INSTALL_EXTRA_TARGETS libavif)
        set_target_properties(libavif PROPERTIES EXPORT_NAME external_libavif)
        add_library(SDL3_image::external_libavif ALIAS libavif)
        if(NOT SDL3IMAGE_AVIF_SHARED)
            list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:avif>)
        endif()
    else()
        message(STATUS "${PROJECT_NAME}: Using system libavif")
        find_package(libavif ${LIBAVIF_MINIMUM_VERSION} REQUIRED)
        if(NOT SDL3IMAGE_AVIF_SHARED)
            list(APPEND PC_REQUIRES libavif)
        endif()
    endif()
    if(SDL3IMAGE_AVIF_SHARED)
        target_include_directories(SDL3_image PRIVATE
            $<TARGET_PROPERTY:avif,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:avif,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:avif,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_avif avif)
        message(STATUS "Dynamic libavif: ${dynamic_avif}")
        target_compile_definitions(SDL3_image PRIVATE "LOAD_AVIF_DYNAMIC=\"${dynamic_avif}\"")
        if(SDL3IMAGE_AVIF_VENDORED)
            add_dependencies(SDL3_image avif)
        endif()
    else()
        target_link_libraries(SDL3_image PRIVATE avif)
    endif()
endif()

if(SDL3IMAGE_BMP)
    target_compile_definitions(SDL3_image PRIVATE LOAD_BMP)
endif()

if(SDL3IMAGE_GIF)
    target_compile_definitions(SDL3_image PRIVATE LOAD_GIF)
endif()

if(SDL3IMAGE_JPG)
    target_compile_definitions(SDL3_image PRIVATE
        LOAD_JPG
        SDL_IMAGE_SAVE_JPG=$<BOOL:${SDL3IMAGE_JPG_SAVE}>
    )
    if(NOT SDL3IMAGE_BACKEND_STB AND NOT SDL3IMAGE_BACKEND_WIC AND NOT SDL3IMAGE_BACKEND_IMAGEIO)
        if(SDL3IMAGE_JPG_VENDORED)
            message(STATUS "${PROJECT_NAME}: Using vendored libjpeg")
            sdl_check_project_in_subfolder(external/jpeg libjpeg SDL3IMAGE_VENDORED)
            set(BUILD_SHARED_LIBS ${SDL3IMAGE_JPG_SHARED})
            add_subdirectory(external/jpeg EXCLUDE_FROM_ALL)
            list(APPEND INSTALL_EXTRA_TARGETS jpeg)
            set_target_properties(jpeg PROPERTIES EXPORT_NAME external_libjpeg)
            add_library(SDL3_image::external_libjpeg ALIAS jpeg)
            if(NOT SDL3IMAGE_JPG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:jpeg>)
            endif()
        else()
            message(STATUS "${PROJECT_NAME}: Using system libjpeg")
            find_package(JPEG REQUIRED)
            if(NOT SDL3IMAGE_JPG_SHARED)
                list(APPEND PC_REQUIRES libjpeg)
            endif()
        endif()
        if(SDL3IMAGE_JPG_SHARED)
            target_include_directories(SDL3_image PRIVATE
                $<TARGET_PROPERTY:JPEG::JPEG,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
            target_get_dynamic_library(dynamic_jpeg JPEG::JPEG)
            message(STATUS "Dynamic libjpeg: ${dynamic_jpeg}")
            target_compile_definitions(SDL3_image PRIVATE "LOAD_JPG_DYNAMIC=\"${dynamic_jpeg}\"")
            if(SDL3IMAGE_JPG_VENDORED)
                add_dependencies(SDL3_image JPEG::JPEG)
            endif()
        else()
            target_link_libraries(SDL3_image PRIVATE JPEG::JPEG)
        endif()
    endif()
endif()

if(SDL3IMAGE_JXL)
    target_compile_definitions(SDL3_image PRIVATE LOAD_JXL)
    if(SDL3IMAGE_JXL_VENDORED)
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
        sdl_check_project_in_subfolder(external/libjxl libjxl SDL3IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL3IMAGE_JXL_SHARED})
        add_subdirectory(external/libjxl EXCLUDE_FROM_ALL)
        if(BUILD_SHARED_LIBS)
            set(jxl_lib jxl)
            list(APPEND INSTALL_EXTRA_TARGETS brotlidec brotlicommon brotlienc hwy ${jxl_lib})
            if(NOT SDL3IMAGE_JXL_SHARED)
                list(APPEND PC_LIBS
                    -l$<TARGET_FILE_BASE_NAME:brotlidec> -l$<TARGET_FILE_BASE_NAME:brotlicommon>
                    -l$<TARGET_FILE_BASE_NAME:brotlienc> -l$<TARGET_FILE_BASE_NAME:${jxl_lib}>
                )
            endif()
        else()
            set(jxl_lib jxl_dec-static)
            list(APPEND INSTALL_EXTRA_TARGETS brotlidec-static brotlicommon-static hwy ${jxl_lib})
        endif()
        set_target_properties(${jxl_lib} PROPERTIES EXPORT_NAME external_libjxl)
        add_library(SDL3_image::external_libjxl ALIAS ${jxl_lib})
        if(NOT TARGET libjxl::libjxl)
            add_library(libjxl::libjxl ALIAS ${jxl_lib})
        endif()
    else()
        message(STATUS "${PROJECT_NAME}: Using system libjxl")
        find_package(libjxl REQUIRED)
        if(NOT SDL3IMAGE_JXL_SHARED)
            list(APPEND PC_REQUIRES libjxl)
        endif()
    endif()
    if(SDL3IMAGE_JXL_SHARED)
        target_include_directories(SDL3_image PRIVATE
            $<TARGET_PROPERTY:libjxl::libjxl,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:libjxl::libjxl,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:libjxl::libjxl,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_jxl libjxl::libjxl)
        message(STATUS "Dynamic libjxl: ${dynamic_jxl}")
        target_compile_definitions(SDL3_image PRIVATE "LOAD_JXL_DYNAMIC=\"${dynamic_jxl}\"")
        if(SDL3IMAGE_JXL_VENDORED)
            add_dependencies(SDL3_image libjxl::libjxl)
        endif()
    else()
        target_link_libraries(SDL3_image PRIVATE libjxl::libjxl)
    endif()
endif()

if(SDL3IMAGE_LBM)
    target_compile_definitions(SDL3_image PRIVATE LOAD_LBM)
endif()

if(SDL3IMAGE_PCX)
    target_compile_definitions(SDL3_image PRIVATE LOAD_PCX)
endif()

if(SDL3IMAGE_PNG)
    target_compile_definitions(SDL3_image PRIVATE
        LOAD_PNG
        SDL_IMAGE_SAVE_PNG=$<BOOL:${SDL3IMAGE_PNG_SAVE}>
    )
    if(NOT SDL3IMAGE_BACKEND_STB AND NOT SDL3IMAGE_BACKEND_WIC AND NOT SDL3IMAGE_BACKEND_IMAGEIO)
        set(HAVE_LD_VERSION_SCRIPT OFF CACHE BOOL "" FORCE)
        if(SDL3IMAGE_PNG_VENDORED)
            message(STATUS "${PROJECT_NAME}: Using vendored libpng")
            sdl_check_project_in_subfolder(external/libpng libpng SDL3IMAGE_VENDORED)
            add_subdirectory(external/libpng EXCLUDE_FROM_ALL)
            if(SDL3IMAGE_PNG_SHARED)
                set(PNG_LIBRARY png)
            else()
                set(PNG_LIBRARY png_static)
            endif()
            add_library(PNG::PNG ALIAS ${PNG_LIBRARY})
            target_include_directories(SDL3_image PRIVATE external/libpng)
            list(APPEND INSTALL_EXTRA_TARGETS ${PNG_LIBRARY})
            set_target_properties(${PNG_LIBRARY} PROPERTIES EXPORT_NAME external_libpng)
            add_library(SDL3_image::external_libpng ALIAS ${PNG_LIBRARY})
            if(NOT SDL3IMAGE_PNG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${PNG_LIBRARY}>)
                if(SDL3IMAGE_ZLIB_VENDORED)
                    list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${ZLIB_LIBRARY}>)
                else()
                    list(APPEND PC_REQUIRES zlib)
                endif()
            endif()
        else()
            message(STATUS "${PROJECT_NAME}: Using system libpng")
            find_package(PNG REQUIRED)
            if(NOT SDL3IMAGE_PNG_SHARED)
                list(APPEND PC_REQUIRES libpng)
            endif()
        endif()
        if(SDL3IMAGE_PNG_SHARED)
            target_include_directories(SDL3_image PRIVATE
                $<TARGET_PROPERTY:PNG::PNG,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:PNG::PNG,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:PNG::PNG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
            target_get_dynamic_library(dynamic_png PNG::PNG)
            message(STATUS "Dynamic libpng: ${dynamic_png}")
            target_compile_definitions(SDL3_image PRIVATE "LOAD_PNG_DYNAMIC=\"${dynamic_png}\"")
            if(SDL3IMAGE_PNG_VENDORED)
                add_dependencies(SDL3_image PNG::PNG)
            endif()
        else()
            target_link_libraries(SDL3_image PRIVATE PNG::PNG)
        endif()
    endif()
endif()

if(SDL3IMAGE_PNM)
    target_compile_definitions(SDL3_image PRIVATE LOAD_PNM)
endif()

if(SDL3IMAGE_QOI)
    target_compile_definitions(SDL3_image PRIVATE LOAD_QOI)
endif()

if(SDL3IMAGE_SVG)
    target_compile_definitions(SDL3_image PRIVATE LOAD_SVG)
endif()

if(SDL3IMAGE_TGA)
    target_compile_definitions(SDL3_image PRIVATE LOAD_TGA)
endif()

if(SDL3IMAGE_TIF)
    target_compile_definitions(SDL3_image PRIVATE LOAD_TIF)
    if(SDL3IMAGE_TIF_VENDORED)
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
        sdl_check_project_in_subfolder(external/libtiff libtiff SDL3IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL3IMAGE_TIF_SHARED})
        add_subdirectory(external/libtiff EXCLUDE_FROM_ALL)
        add_library(TIFF::TIFF ALIAS tiff)
        list(APPEND INSTALL_EXTRA_TARGETS tiff)
        set_target_properties(tiff PROPERTIES EXPORT_NAME external_libtiff)
        add_library(SDL3_image::external_libtiff ALIAS tiff)
        if(NOT SDL3IMAGE_TIF_SHARED)
            list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:tiff>)
        endif()
    else()
        message(STATUS "${PROJECT_NAME}: Using system libtiff")
        find_package(TIFF REQUIRED)
        if(NOT SDL3IMAGE_TIF_SHARED)
            list(APPEND PC_REQUIRES libtiff-4)
        endif()
    endif()
    if(SDL3IMAGE_TIF_SHARED)
        target_include_directories(SDL3_image PRIVATE
            $<TARGET_PROPERTY:TIFF::TIFF,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_tif TIFF::TIFF)
        message(STATUS "Dynamic libtiff: ${dynamic_tif}")
        target_compile_definitions(SDL3_image PRIVATE "LOAD_TIF_DYNAMIC=\"${dynamic_tif}\"")
        if(SDL3IMAGE_TIF_VENDORED)
            add_dependencies(SDL3_image TIFF::TIFF)
        endif()
    else()
        target_link_libraries(SDL3_image PRIVATE TIFF::TIFF)
    endif()
endif()

if(SDL3IMAGE_WEBP)
    target_compile_definitions(SDL3_image PRIVATE LOAD_WEBP)
    # missing cpufeatures
    if(SDL3IMAGE_WEBP_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libwebp")
        sdl_check_project_in_subfolder(external/libwebp libwebp SDL3IMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDL3IMAGE_WEBP_SHARED})
        add_subdirectory(external/libwebp EXCLUDE_FROM_ALL)
        target_include_directories(SDL3_image PRIVATE external/libwebp/src)
        add_library(WebP::webp ALIAS webp)
        add_library(WebP::webpdemux ALIAS webpdemux)
        list(APPEND INSTALL_EXTRA_TARGETS webp webpdemux)
        set_target_properties(webp PROPERTIES EXPORT_NAME "external_libwebp")
        add_library(SDL3_image::external_libwebp ALIAS webp)
    else()
        message(STATUS "${PROJECT_NAME}: Using system libwebp")
        find_package(webp REQUIRED)
        if(NOT SDL3IMAGE_WEBP_SHARED)
            list(APPEND PC_REQUIRES libwebp)
        endif()
    endif()
    if(SDL3IMAGE_WEBP_SHARED)
        target_include_directories(SDL3_image PRIVATE
            $<TARGET_PROPERTY:WebP::webp,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webp,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webp,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webpdemux,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webpdemux,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webpdemux,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
        )
        target_get_dynamic_library(dynamic_webpdemux WebP::webpdemux)
        message(STATUS "Dynamic libwebpdemux: ${dynamic_webpdemux}")
        target_compile_definitions(SDL3_image PRIVATE "LOAD_WEBPDEMUX_DYNAMIC=\"${dynamic_webpdemux}\"")
        target_get_dynamic_library(dynamic_webp WebP::webp)
        message(STATUS "Dynamic libwebp: ${dynamic_webp}")
        target_compile_definitions(SDL3_image PRIVATE "LOAD_WEBP_DYNAMIC=\"${dynamic_webp}\"")
        if(SDL3IMAGE_WEBP_VENDORED)
            add_dependencies(SDL3_image WebP::webp WebP::webpdemux)
        endif()
    else()
        target_link_libraries(SDL3_image PRIVATE WebP::webp WebP::webpdemux)
    endif()
endif()

if(SDL3IMAGE_XCF)
    target_compile_definitions(SDL3_image PRIVATE LOAD_XCF)
endif()

if(SDL3IMAGE_XPM)
    target_compile_definitions(SDL3_image PRIVATE LOAD_XPM)
endif()

if(SDL3IMAGE_XV)
    target_compile_definitions(SDL3_image PRIVATE LOAD_XV)
endif()

# Restore BUILD_SHARED_LIBS
set(BUILD_SHARED_LIBS ${SDL3IMAGE_BUILD_SHARED_LIBS})

if(SDL3IMAGE_INSTALL)
    install(
        TARGETS SDL3_image
        EXPORT SDL3ImageExports
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3/SDL_image.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3" COMPONENT DEVEL
    )

    if(INSTALL_EXTRA_TARGETS)
        install(
            TARGETS ${INSTALL_EXTRA_TARGETS}
            EXPORT SDL3ImageExports
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT devel
        )
    endif()

    if(WIN32 AND NOT MINGW)
        set(SDL3IMAGE_INSTALL_CMAKEDIR_ROOT_DEFAULT "cmake")
    else()
        set(SDL3IMAGE_INSTALL_CMAKEDIR_ROOT_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake")
    endif()
    set(SDL3IMAGE_INSTALL_CMAKEDIR_ROOT "${SDL3IMAGE_INSTALL_CMAKEDIR_ROOT_DEFAULT}" CACHE STRING "Root folder where to install SDL3Config.cmake related files (SDL3 subfolder for MSVC projects)")

    if(WIN32 AND NOT MINGW)
        set(SDL3IMAGE_INSTALL_CMAKEDIR "${SDL3IMAGE_INSTALL_CMAKEDIR_ROOT}")
        set(LICENSES_PREFIX "licenses/SDL3_image")
    else()
        set(SDL3IMAGE_INSTALL_CMAKEDIR "${SDL3IMAGE_INSTALL_CMAKEDIR_ROOT}/SDL3_image")
        set(LICENSES_PREFIX "${CMAKE_INSTALL_DATAROOTDIR}/licenses/SDL3_image")
    endif()

    configure_package_config_file(cmake/SDL3_imageConfig.cmake.in SDL3_imageConfig.cmake
        INSTALL_DESTINATION "${SDL3IMAGE_INSTALL_CMAKEDIR}"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL3_imageConfigVersion.cmake"
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_imageConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_imageConfigVersion.cmake"
            cmake/Findlibjxl.cmake
            cmake/Findwebp.cmake
        DESTINATION "${SDL3IMAGE_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )
    install(EXPORT SDL3ImageExports
        FILE SDL3_image-${sdl3_image_install_name_infix}-targets.cmake
        NAMESPACE SDL3_image::
        DESTINATION "${SDL3IMAGE_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )

    string(JOIN " " PC_REQUIRES ${PC_REQUIRES})
    string(JOIN " " PC_LIBS ${PC_LIBS})
    configure_file(cmake/sdl3-image.pc.in sdl3-image.pc @ONLY)

    if(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
        # FreeBSD uses ${PREFIX}/libdata/pkgconfig
        set(PC_DESTDIR "libdata/pkgconfig")
    else()
        set(PC_DESTDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    endif()
    # Always install sdl3-image.pc file: libraries might be different between config modes
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl3-image.pc"
        DESTINATION "${PC_DESTDIR}" COMPONENT devel)

    install(FILES "LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
        COMPONENT library
    )
endif()

if(SDL3IMAGE_SAMPLES)
    add_executable(showanim showanim.c)
    add_executable(showimage showimage.c)

    foreach(prog showanim showimage)
        target_link_libraries(${prog} PRIVATE SDL3_image::${sdl3_image_export_name})
        target_link_libraries(${prog} PRIVATE ${sdl3_target_name})

        if(SDL3IMAGE_SAMPLES_INSTALL)
            install(TARGETS ${prog}
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            )
        endif()
    endforeach()
endif()

if(SDL3IMAGE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
