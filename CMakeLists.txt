cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# See docs/release_checklist.md
set(MAJOR_VERSION 3)
set(MINOR_VERSION 1)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION 3.1.3)

project(SDL3_image
    LANGUAGES C
    VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/GetGitRevisionDescription.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/PrivateSdlFunctions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sdlcpu.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sdlplatform.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sdlmanpages.cmake")
sdl_calculate_derived_version_variables(${MAJOR_VERSION} ${MINOR_VERSION} ${MICRO_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SDLIMAGE_ROOTPROJECT ON)
else()
    set(SDLIMAGE_ROOTPROJECT OFF)
endif()

set(SDLIMAGE_SAMPLES_DEFAULT ${SDLIMAGE_ROOTPROJECT})
if(ANDROID)
    set(SDLIMAGE_SAMPLES_DEFAULT OFF)
endif()

# Assume MSVC projects don't have a package manager and need vendored dependencies (by default).
# Most other platforms have some kind of package manager.
if(MSVC)
    set(vendored_default ON)
else()
    set(vendored_default OFF)
endif()

set(sdlimage_install_enableable ON)
if((TARGET SDL3-shared OR TARGET SDL3-static) AND SDL_DISABLE_INSTALL)
    # Cannot install SDL3_image when SDL3 is built in same built, and is not installed.
    set(sdlimage_install_enableable OFF)
endif()

if(NOT DEFINED CMAKE_FIND_PACKAGE_PREFER_CONFIG)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
endif()

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(PLATFORM_SUPPORTS_SHARED ON)
if(EMSCRIPTEN OR VITA OR PSP OR PS2 OR N3DS OR RISCOS)
  set(PLATFORM_SUPPORTS_SHARED OFF)
endif()

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ${PLATFORM_SUPPORTS_SHARED})
cmake_dependent_option(BUILD_SHARED_LIBS "Build the library as a shared library" ON PLATFORM_SUPPORTS_SHARED OFF)

cmake_dependent_option(SDLIMAGE_INSTALL "Enable SDL3_image install target" ${SDLIMAGE_ROOTPROJECT} "${sdlimage_install_enableable}" OFF)
cmake_dependent_option(SDLIMAGE_INSTALL_CPACK "Create binary SDL3_image archive using CPack" ${SDLIMAGE_ROOTPROJECT} "SDLIMAGE_INSTALL" OFF)
cmake_dependent_option(SDLIMAGE_INSTALL_MAN "Install man pages for SDL3_image" OFF "SDLIMAGE_INSTALL" OFF)
cmake_dependent_option(SDLIMAGE_DEPS_SHARED "Load dependencies dynamically" ON PLATFORM_SUPPORTS_SHARED OFF)
option(SDLIMAGE_VENDORED "Use vendored third-party libraries" ${vendored_default})
option(SDLIMAGE_WERROR "Treat warnings as errors" OFF)

option(SDLIMAGE_STRICT "Fail when a dependency could not be found" OFF)
set(required "")
set(fatal_error "STATUS")
if(SDLIMAGE_STRICT)
  set(required "REQUIRED")
  set(fatal_error "FATAL_ERROR")
endif()

option(SDLIMAGE_SAMPLES "Build the SDL3_image sample program(s)" ${SDLIMAGE_SAMPLES_DEFAULT})
cmake_dependent_option(SDLIMAGE_SAMPLES_INSTALL "Install the SDL3_image sample program(s)" OFF "SDLIMAGE_SAMPLES;SDLIMAGE_INSTALL" OFF)

option(SDLIMAGE_TESTS "Build unit tests?" OFF)
option(SDLIMAGE_TESTS_INSTALL "Install unit tests?" OFF)

option(SDLIMAGE_BACKEND_STB "Use stb_image for loading JPEG and PNG files" ON)
cmake_dependent_option(SDLIMAGE_BACKEND_WIC "Add WIC backend (Windows Imaging Component)" OFF WIN32 OFF)
cmake_dependent_option(SDLIMAGE_BACKEND_IMAGEIO "Use native Mac OS X frameworks for loading images" ON APPLE OFF)

option(SDLIMAGE_AVIF "Support loading AVIF images" ON)
option(SDLIMAGE_BMP "Support loading BMP images" ON)
option(SDLIMAGE_GIF "Support loading GIF images" ON)
option(SDLIMAGE_JPG "Support loading JPEG images" ON)
option(SDLIMAGE_JXL "Support loading JXL images" OFF)
option(SDLIMAGE_LBM "Support loading LBM images" ON)
option(SDLIMAGE_PCX "Support loading PCX images" ON)
option(SDLIMAGE_PNG "Support loading PNG images" ON)
option(SDLIMAGE_PNM "Support loading PNM images" ON)
option(SDLIMAGE_QOI "Support loading QOI images" ON)
option(SDLIMAGE_SVG "Support loading SVG images" ON)
option(SDLIMAGE_TGA "Support loading TGA images" ON)
option(SDLIMAGE_TIF "Support loading TIFF images" ON)
option(SDLIMAGE_WEBP "Support loading WEBP images" ON)
option(SDLIMAGE_XCF "Support loading XCF images" ON)
option(SDLIMAGE_XPM "Support loading XPM images" ON)
option(SDLIMAGE_XV "Support loading XV images" ON)

cmake_dependent_option(SDLIMAGE_AVIF_SAVE "Add AVIF save support" ON SDLIMAGE_AVIF OFF)
cmake_dependent_option(SDLIMAGE_JPG_SAVE "Add JPEG save support" ON SDLIMAGE_JPG OFF)
cmake_dependent_option(SDLIMAGE_PNG_SAVE "Add PNG save support" ON SDLIMAGE_PNG OFF)

set(LIBAVIF_MINIMUM_VERSION "1.0")
if(SDLIMAGE_VENDORED AND SDLIMAGE_AVIF)
    set(SDLIMAGE_AVIF_VENDORED ON)
else()
    set(SDLIMAGE_AVIF_VENDORED OFF)
endif()
cmake_dependent_option(SDLIMAGE_AVIF_SHARED "Dynamically load AVIF support (requires shared libavif)"
    ${SDLIMAGE_DEPS_SHARED} SDLIMAGE_AVIF ON)

if(SDLIMAGE_AVIF_VENDORED)
  set(SDLIMAGE_DAV1D ON)
  set(SDLIMAGE_DAV1D_VENDORED ON)
  set(SDLIMAGE_AOM ON)
  set(SDLIMAGE_AOM_VENDORED ON)
else()
  set(SDLIMAGE_DAV1D OFF)
  set(SDLIMAGE_DAV1D_VENDORED OFF)
  set(SDLIMAGE_AOM OFF)
  set(SDLIMAGE_AOM_VENDORED OFF)
endif()
if(SDLIMAGE_AVIF_SHARED)
  set(SDLIMAGE_DAV1D_SHARED ON)
  set(SDLIMAGE_AOM_SHARED ON)
else()
  set(SDLIMAGE_DAV1D_SHARED OFF)
  set(SDLIMAGE_AOM_SHARED OFF)
endif()

if(SDLIMAGE_VENDORED AND SDLIMAGE_JPG AND NOT (SDLIMAGE_BACKEND_WIC OR SDLIMAGE_BACKEND_STB OR SDLIMAGE_BACKEND_IMAGEIO))
    set(SDLIMAGE_JPG_VENDORED ON)
else()
    set(SDLIMAGE_JPG_VENDORED OFF)
endif()
cmake_dependent_option(SDLIMAGE_JPG_SHARED "Dynamically load JPG support (requires shared libjpeg)"
    ${SDLIMAGE_DEPS_SHARED} "SDLIMAGE_JPG;NOT SDLIMAGE_BACKEND_WIC;NOT SDLIMAGE_BACKEND_STB;NOT SDLIMAGE_BACKEND_IMAGEIO" OFF)

if(SDLIMAGE_VENDORED AND SDLIMAGE_JXL)
    set(SDLIMAGE_JXL_VENDORED ON)
else()
    set(SDLIMAGE_JXL_VENDORED OFF)
endif()
cmake_dependent_option(SDLIMAGE_JXL_SHARED "Dynamically load JXL support (requires shared libjxl)"
    ${SDLIMAGE_DEPS_SHARED} SDLIMAGE_JXL OFF)

if(SDLIMAGE_VENDORED AND SDLIMAGE_PNG AND NOT (SDLIMAGE_BACKEND_WIC OR SDLIMAGE_BACKEND_STB OR SDLIMAGE_BACKEND_IMAGEIO))
    set(SDLIMAGE_PNG_VENDORED ON)
else()
    set(SDLIMAGE_PNG_VENDORED OFF)
endif()
cmake_dependent_option(SDLIMAGE_PNG_SHARED "Dynamically load PNG support (requires shared libpng)"
    ${SDLIMAGE_DEPS_SHARED} "SDLIMAGE_PNG;NOT SDLIMAGE_BACKEND_WIC;NOT SDLIMAGE_BACKEND_STB;NOT SDLIMAGE_BACKEND_IMAGEIO" OFF)

if(SDLIMAGE_VENDORED AND SDLIMAGE_TIF AND NOT (SDLIMAGE_BACKEND_IMAGEIO OR SDLIMAGE_BACKEND_WIC))
    set(SDLIMAGE_TIF_VENDORED ON)
else()
    set(SDLIMAGE_TIF_VENDORED OFF)
endif()
cmake_dependent_option(SDLIMAGE_TIF_SHARED "Dynamically load TIFF support (requires shared libtiff)"
    ${SDLIMAGE_DEPS_SHARED} SDLIMAGE_TIF OFF)

if(SDLIMAGE_VENDORED AND SDLIMAGE_WEBP)
    set(SDLIMAGE_WEBP_VENDORED ON)
else()
    set(SDLIMAGE_WEBP_VENDORED OFF)
endif()
cmake_dependent_option(SDLIMAGE_WEBP_SHARED "Dynamically load WEBP support (requires shared libwebp)"
    ${SDLIMAGE_DEPS_SHARED} SDLIMAGE_WEBP OFF)

if(SDLIMAGE_PNG_VENDORED)
    set(SDLIMAGE_ZLIB ON)
else()
    set(SDLIMAGE_ZLIB OFF)
endif()

if(SDLIMAGE_VENDORED AND SDLIMAGE_PNG_VENDORED)
    set(SDLIMAGE_ZLIB_VENDORED ON)
else()
    set(SDLIMAGE_ZLIB_VENDORED OFF)
endif()
if(SDLIMAGE_PNG_SHARED)
    set(SDLIMAGE_ZLIB_SHARED ON)
else()
    set(SDLIMAGE_ZLIB_SHARED OFF)
endif()

# Save BUILD_SHARED_LIBS variable
set(SDLIMAGE_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

set(sdl_required_components Headers)

if(SDLIMAGE_BUILD_SHARED_LIBS)
    set(sdl3_image_target_name SDL3_image-shared)
    set(sdl3_target_name SDL3::SDL3-shared)

    list(APPEND sdl_required_components SDL3-shared)
else()
    set(sdl3_image_target_name SDL3_image-static)
    set(sdl3_target_name SDL3::SDL3)
endif()

if(NOT TARGET SDL3::Headers OR NOT TARGET ${sdl3_target_name})
    find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED COMPONENTS ${sdl_required_components})
endif()

SDL_DetectTargetCPUArchitectures(SDL_CPU_NAMES)
SDL_DetectCMakePlatform()

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

set(BUILD_SHARED_LIBS ${SDLIMAGE_BUILD_SHARED_LIBS})
add_library(${sdl3_image_target_name}
    src/IMG.c
    src/IMG_WIC.c
    src/IMG_avif.c
    src/IMG_bmp.c
    src/IMG_gif.c
    src/IMG_jpg.c
    src/IMG_jxl.c
    src/IMG_lbm.c
    src/IMG_pcx.c
    src/IMG_png.c
    src/IMG_pnm.c
    src/IMG_qoi.c
    src/IMG_stb.c
    src/IMG_svg.c
    src/IMG_tga.c
    src/IMG_tif.c
    src/IMG_webp.c
    src/IMG_xcf.c
    src/IMG_xpm.c
    src/IMG_xv.c
)
add_library(SDL3_image::${sdl3_image_target_name} ALIAS ${sdl3_image_target_name})
if(NOT TARGET SDL3_image::SDL3_image)
    add_library(SDL3_image::SDL3_image ALIAS ${sdl3_image_target_name})
endif()
target_include_directories(${sdl3_image_target_name}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_compile_definitions(${sdl3_image_target_name} PRIVATE
    BUILD_SDL
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
)
target_link_libraries(${sdl3_image_target_name} PUBLIC SDL3::Headers)
if(SDLIMAGE_BUILD_SHARED_LIBS)
    target_link_libraries(${sdl3_image_target_name} PRIVATE SDL3::SDL3-shared)
endif()
sdl_add_warning_options(${sdl3_image_target_name} WARNING_AS_ERROR ${SDLIMAGE_WERROR})
if(WIN32 AND SDLIMAGE_BUILD_SHARED_LIBS)
    target_sources(${sdl3_image_target_name} PRIVATE
        src/version.rc
    )
endif()
set_target_properties(${sdl3_image_target_name} PROPERTIES
    OUTPUT_NAME "SDL3_image"
    DEFINE_SYMBOL DLL_EXPORT
    EXPORT_NAME ${sdl3_image_target_name}
    C_VISIBILITY_PRESET "hidden"
)

sdl_target_link_option_version_file(${sdl3_image_target_name} "${CMAKE_CURRENT_SOURCE_DIR}/src/SDL_image.sym")

if(NOT ANDROID)
    set_target_properties(${sdl3_image_target_name} PROPERTIES
        SOVERSION "${SO_VERSION_MAJOR}"
        VERSION "${SO_VERSION}"
    )
    if(APPLE)
        cmake_minimum_required(VERSION 3.17)
        set_target_properties(${sdl3_image_target_name} PROPERTIES
            MACHO_COMPATIBILITY_VERSION "${DYLIB_COMPAT_VERSION}"
            MACHO_CURRENT_VERSION "${DYLIB_CURRENT_VERSION}"
        )
        sdl_no_deprecated_errors(${sdl3_image_target_name})
    endif()
endif()
if(SDLIMAGE_BUILD_SHARED_LIBS)
    if(WIN32)
        set_target_properties(${sdl3_image_target_name} PROPERTIES
            PREFIX ""
            DLL_NAME_WITH_SOVERSION FALSE
        )
    endif()
else()
    if(MSVC)
        set_target_properties(${sdl3_image_target_name} PROPERTIES
            OUTPUT_NAME "SDL3_image-static"
        )
    endif()
endif()

# Use `Compatible Interface Properties` to ensure a shared SDL3_image is built with a shared SDL3
if(SDLIMAGE_BUILD_SHARED_LIBS)
    set_property(TARGET ${sdl3_image_target_name} PROPERTY INTERFACE_SDL3_SHARED TRUE)
    set_property(TARGET ${sdl3_image_target_name} APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL3_SHARED)
endif()

if(SDLIMAGE_BUILD_SHARED_LIBS)
    sdl_target_link_options_no_undefined(${sdl3_image_target_name})
endif()

if(SDLIMAGE_BUILD_SHARED_LIBS)
    # Make sure static library dependencies are built with -fPIC when building a shared SDL3_image
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(INSTALL_EXTRA_TARGETS)
set(INSTALL_EXTRA_CMAKE_MODULES)
set(PC_LIBS)
set(PC_REQUIRES)
set(SDLIMAGE_BACKENDS)

list(APPEND SDLIMAGE_BACKENDS STB)
set(SDLIMAGE_STB_ENABLED FALSE)
if(SDLIMAGE_BACKEND_STB)
    set(SDLIMAGE_STB_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE USE_STBIMAGE)
endif()

list(APPEND SDLIMAGE_BACKENDS IMAGEIO)
set(SDLIMAGE_IMAGEIO_ENABLED FALSE)
if(APPLE)
    if(SDLIMAGE_BACKEND_IMAGEIO)
        set(SDLIMAGE_IMAGEIO_ENABLED TRUE)
        target_link_libraries(${sdl3_image_target_name} PRIVATE -Wl,-framework,ApplicationServices)
        target_link_libraries(${sdl3_image_target_name} PRIVATE objc)
        target_sources(${sdl3_image_target_name} PRIVATE
            src/IMG_ImageIO.m
        )

        if (SDLIMAGE_PNG AND NOT SDLIMAGE_BACKEND_STB)
            target_compile_definitions(${sdl3_image_target_name} PRIVATE PNG_USES_IMAGEIO)
        endif()
        if (SDLIMAGE_JPG AND NOT SDLIMAGE_BACKEND_STB)
            target_compile_definitions(${sdl3_image_target_name} PRIVATE JPG_USES_IMAGEIO)
        endif()
    else()
        target_compile_definitions(${sdl3_image_target_name} PRIVATE SDL_IMAGE_USE_COMMON_BACKEND)
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS WIC)
set(SDLIMAGE_WIC_ENABLED FALSE)
if(SDLIMAGE_BACKEND_WIC)
    set(SDLIMAGE_WIC_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE SDL_IMAGE_USE_WIC_BACKEND)
endif()

if(SDLIMAGE_ZLIB)
    if(SDLIMAGE_ZLIB_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored zlib")
        # disable build of zlib example programs:
        set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "zlib examples" FORCE)
        sdl_check_project_in_subfolder(external/zlib zlib SDLIMAGE_VENDORED)
        add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
        # PNG_BUILD_ZLIB variable is used by vendored libpng
        set(PNG_BUILD_ZLIB ON CACHE BOOL "libpng option to tell it should use 'our' vendored ZLIB library" FORCE)
        # ZLIB_INCLUDE_DIR variable is used by vendored libpng
        set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib;${CMAKE_CURRENT_BINARY_DIR}/external/zlib" CACHE STRING "path of zlib, passed to libpng" FORCE)
        # ZLIB_LIBRARY variable is used by vendored libpng
        if(SDLIMAGE_ZLIB_SHARED)
            set(ZLIB_LIBRARY zlib)
        else()
            set(ZLIB_LIBRARY zlibstatic)
        endif()
        if(NOT TARGET ZLIB::ZLIB)
            add_library(ZLIB::ZLIB ALIAS ${ZLIB_LIBRARY})
        endif()
        # ZLIB_LIBRARIES variable is used by vendored libpng
        set(ZLIB_LIBRARIES "${ZLIB_LIBRARY}")
        # ZLIB_INCLUDE_DIRS variable is used by vendored libpng
        set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}")
        target_include_directories(${ZLIB_LIBRARY} INTERFACE $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIRS}>)
        if(SDLIMAGE_ZLIB_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
            list(APPEND INSTALL_EXTRA_TARGETS ${ZLIB_LIBRARY})
        endif()
        set_target_properties(${ZLIB_LIBRARY} PROPERTIES EXPORT_NAME external_zlib)
        add_library(SDL3_image::external_zlib ALIAS ${ZLIB_LIBRARY})
    else()
        message(FATAL_ERROR "Internal error (zlib is only required when requesting vendored dependencies)")
    endif()
endif()

if(SDLIMAGE_DAV1D)
    if(SDLIMAGE_DAV1D_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored dav1d")
        set(BUILD_SHARED_LIBS ${SDLIMAGE_DAV1D_SHARED})
        add_subdirectory(external/dav1d EXCLUDE_FROM_ALL)
        if(DAV1D_ASM AND (SDL_CPU_X64 OR SDL_CPU_X86))
            enable_language(ASM_NASM)
        endif()
        set(DAV1D_LIBRARY dav1d)
        if(SDLIMAGE_DAV1D_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
            list(APPEND INSTALL_EXTRA_TARGETS dav1d)
        endif()
    else()
        message(FATAL_ERROR "Internal error (dav1d is only required when requesting vendored dependencies)")
    endif()
endif()

if(SDLIMAGE_AOM)
    if(SDLIMAGE_AOM_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored aom")
        set(BUILD_SHARED_LIBS ${SDLIMAGE_AOM_SHARED})
        set(ENABLE_DOCS OFF CACHE BOOL "")
        set(ENABLE_EXAMPLES OFF CACHE BOOL "")
        set(ENABLE_TESTDATA OFF CACHE BOOL "")
        set(ENABLE_TESTS OFF CACHE BOOL "")
        set(ENABLE_TOOLS OFF CACHE BOOL "")
        if(SDLIMAGE_AOM_SHARED)
            set(CONFIG_AV1_DECODER 1 CACHE BOOL "" FORCE)
        else()
            set(CONFIG_AV1_DECODER 0 CACHE BOOL "" FORCE)
        endif()
        set(CONFIG_AV1_ENCODER 1 CACHE BOOL "" FORCE)
        # aom detects cpu using CMAKE_SYSTEM_NAME
        if(SDL_CPU_X86)
          set(AOM_TARGET_CPU "x86")
        elseif(SDL_CPU_X64)
          set(AOM_TARGET_CPU "x86_64")
        elseif(SDL_CPU_ARM32)
          set(AOM_TARGET_CPU "arm")
        elseif(SDL_CPU_ARM64)
          set(AOM_TARGET_CPU "aarch64")
        endif()
        add_subdirectory(external/aom EXCLUDE_FROM_ALL)
        set(AOM_LIBRARY aom)
        if(SDLIMAGE_AOM_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
            list(APPEND INSTALL_EXTRA_TARGETS aom)
        endif()
    else()
        message(FATAL_ERROR "Internal error (aom is only required when requesting vendored dependencies)")
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS AVIF)
set(SDLIMAGE_AVIF_ENABLED FALSE)
if(SDLIMAGE_AVIF)
    if(SDLIMAGE_AVIF_VENDORED)
        set(SDLIMAGE_AVIF_ENABLED TRUE)
        message(STATUS "${PROJECT_NAME}: Using vendored libavif")
        sdl_check_project_in_subfolder(external/libavif libavif SDLIMAGE_VENDORED)
        set(SKIP_INSTALL_LIBRARIES TRUE)
        set(BUILD_SHARED_LIBS ${SDLIMAGE_AVIF_SHARED})
        set(AVIF_CODEC_DAV1D ON CACHE BOOL "Use dav1d codec for decoding" FORCE)
        set(AVIF_LOCAL_DAV1D OFF CACHE BOOL "Build dav1d by libavif" FORCE)
        set(AVIF_CODEC_AOM ON CACHE BOOL "Use aom codec for decoding" FORCE)
        set(AVIF_LOCAL_AOM OFF CACHE BOOL "Build aom by libavif" FORCE)
        set(AVIF_CODEC_AOM_ENCODE ON CACHE BOOL "Enable aom encoder" FORCE)
        set(AVIF_CODEC_AOM_DECODE OFF CACHE BOOL "Enable aom decoder" FORCE)
        set(LIBAVIF_WITH_SHARPYUV_SDLIMAGE ${SDLIMAGE_WEBP} CACHE BOOL "Build libavif with sharpyuv support (re-use sharpyuv built by libwebp)" FORCE)
        set(BUILD_SHARED_LIBS ${SDLIMAGE_AVIF_SHARED})
        set(AOM_LIBRARIES "aom" CACHE STRING "aom libraries" FORCE)
        add_subdirectory(external/libavif EXCLUDE_FROM_ALL)
        if(SDLIMAGE_AVIF_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
            list(APPEND INSTALL_EXTRA_TARGETS avif)
        endif()
        set_target_properties(avif PROPERTIES EXPORT_NAME external_libavif)
        add_library(SDL3_image::external_libavif ALIAS avif)
        if(NOT SDLIMAGE_AVIF_SHARED)
            list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:avif>)
        endif()
        if(NOT MSVC)
          sdl_check_linker_flag("-Wl,--no-undefined" LINKER_SUPPORTS_WL_NO_UNDEFINED)
          if(LINKER_SUPPORTS_WL_NO_UNDEFINED)
            target_link_options(avif PRIVATE "-Wl,--no-undefined")
          endif()
        endif()
    else()
        find_package(libavif "${LIBAVIF_MINIMUM_VERSION}" QUIET ${required})
        if(libavif_FOUND)
            message(STATUS "${PROJECT_NAME}: Using system libavif")
            set(SDLIMAGE_AVIF_ENABLED TRUE)
            if(NOT SDLIMAGE_AVIF_SHARED)
                list(APPEND PC_REQUIRES "libavif >= ${LIBAVIF_MINIMUM_VERSION}")
            endif()
        else()
            message(STATUS "libavif-${LIBAVIF_MINIMUM_VERSION} or compatible not found")
            message(${fatal_error} "libavif NOT found")
        endif()
    endif()
    if(SDLIMAGE_AVIF_ENABLED)
        target_compile_definitions(${sdl3_image_target_name} PRIVATE
            LOAD_AVIF
            SDL_IMAGE_SAVE_AVIF=$<BOOL:${SDLIMAGE_AVIF_SAVE}>
        )
        if(SDLIMAGE_AVIF_SHARED)
            target_include_directories(${sdl3_image_target_name} PRIVATE
                $<TARGET_PROPERTY:avif,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:avif,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:avif,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
            target_get_dynamic_library(dynamic_avif avif)
            message(STATUS "Dynamic libavif: ${dynamic_avif}")
            target_compile_definitions(${sdl3_image_target_name} PRIVATE "LOAD_AVIF_DYNAMIC=\"${dynamic_avif}\"")
            if(SDLIMAGE_AVIF_VENDORED)
                add_dependencies(${sdl3_image_target_name} avif)
            endif()
        else()
            target_link_libraries(${sdl3_image_target_name} PRIVATE avif)
        endif()
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS BMP)
set(SDLIMAGE_BMP_ENABLED FALSE)
if(SDLIMAGE_BMP)
    set(SDLIMAGE_BMP_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_BMP)
endif()

list(APPEND SDLIMAGE_BACKENDS GIF)
set(SDLIMAGE_GIF_ENABLED FALSE)
if(SDLIMAGE_GIF)
    set(SDLIMAGE_GIF_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_GIF)
endif()

list(APPEND SDLIMAGE_BACKENDS JPG)
set(SDLIMAGE_JPG_ENABLED FALSE)
if(SDLIMAGE_JPG)
    if(SDLIMAGE_BACKEND_STB OR SDLIMAGE_BACKEND_WIC OR SDLIMAGE_BACKEND_IMAGEIO)
        set(SDLIMAGE_JPG_ENABLED TRUE)
    else()
        if(SDLIMAGE_JPG_VENDORED)
            set(SDLIMAGE_JPG_ENABLED TRUE)
            message(STATUS "${PROJECT_NAME}: Using vendored libjpeg")
            sdl_check_project_in_subfolder(external/jpeg libjpeg SDLIMAGE_VENDORED)
            set(BUILD_SHARED_LIBS ${SDLIMAGE_JPG_SHARED})
            add_subdirectory(external/jpeg EXCLUDE_FROM_ALL)
            if(SDLIMAGE_JPG_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
                list(APPEND INSTALL_EXTRA_TARGETS jpeg)
            endif()
            set_target_properties(jpeg PROPERTIES EXPORT_NAME external_libjpeg)
            add_library(SDL3_image::external_libjpeg ALIAS jpeg)
            if(NOT SDLIMAGE_JPG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:jpeg>)
            endif()
        else()
            find_package(JPEG ${required})
            if(JPEG_FOUND)
                message(STATUS "${PROJECT_NAME}: Using system libjpeg")
                set(SDLIMAGE_JPG_ENABLED TRUE)
                if(NOT SDLIMAGE_JPG_SHARED)
                    list(APPEND PC_REQUIRES libjpeg)
                endif()
            else()
                message(${fatal_error} "libjpeg NOT found")
            endif()
        endif()
        if(SDLIMAGE_JPG_ENABLED)
            if(SDLIMAGE_JPG_SHARED)
                target_include_directories(${sdl3_image_target_name} PRIVATE
                    $<TARGET_PROPERTY:JPEG::JPEG,INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                )
                target_get_dynamic_library(dynamic_jpeg JPEG::JPEG)
                message(STATUS "Dynamic libjpeg: ${dynamic_jpeg}")
                target_compile_definitions(${sdl3_image_target_name} PRIVATE "LOAD_JPG_DYNAMIC=\"${dynamic_jpeg}\"")
                if(SDLIMAGE_JPG_VENDORED)
                    add_dependencies(${sdl3_image_target_name} JPEG::JPEG)
                endif()
            else()
                target_link_libraries(${sdl3_image_target_name} PRIVATE JPEG::JPEG)
            endif()
      endif()
    endif()
    if(SDLIMAGE_JPG_ENABLED)
        target_compile_definitions(${sdl3_image_target_name} PRIVATE
            LOAD_JPG
            SDL_IMAGE_SAVE_JPG=$<BOOL:${SDLIMAGE_JPG_SAVE}>
        )
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS JXL)
set(SDLIMAGE_JXL_ENABLED FALSE)
if(SDLIMAGE_JXL)
    if(SDLIMAGE_JXL_VENDORED)
        set(SDLIMAGE_JXL_ENABLED TRUE)
        enable_language(CXX)
        message(STATUS "${PROJECT_NAME}: Using vendored libjxl")
        # BUILD_TESTING variable is used by libjxl
        set(BUILD_TESTING OFF CACHE BOOL "build testing")
        # JPEGXL_ENABLE_BENCHMARK variable is used by libjxl
        set(JPEGXL_ENABLE_BENCHMARK OFF CACHE BOOL "libjpegxl benchmark" FORCE)
        # JPEGXL_ENABLE_TOOLS variable is used by libjxl
        set(JPEGXL_ENABLE_JNI OFF CACHE BOOL "build jpegxl fni")
        # JPEGXL_ENABLE_MANPAGES variable is used by libjxl
        set(JPEGXL_ENABLE_MANPAGES OFF CACHE BOOL "libjxl manpage option" FORCE)
        # JPEGXL_ENABLE_PLUGINS variable is used by libjxl
        set(JPEGXL_ENABLE_PLUGINS OFF CACHE BOOL "libjxl manpage option" FORCE)
        # JPEGXL_ENABLE_SKCMS variable is used by libjxl
        set(JPEGXL_ENABLE_SKCMS OFF CACHE BOOL "libjxl skcms option" FORCE)
        # JPEGXL_FORCE_SYSTEM_HWY variable is used by libjxl
        set(JPEGXL_FORCE_SYSTEM_HWY OFF CACHE BOOL "libjxl highway option" FORCE)
        sdl_check_project_in_subfolder(external/libjxl libjxl SDLIMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDLIMAGE_JXL_SHARED})
        add_subdirectory(external/libjxl EXCLUDE_FROM_ALL)
        if(SDLIMAGE_JXL_SHARED)
            set(jxl_lib jxl)
            set(jxl_install_libs brotlidec brotlicommon brotlienc jxl)
        else()
            set(jxl_lib jxl_dec-static)
            set(jxl_install_libs brotlidec-static brotlicommon-static hwy jxl_dec-static)
            list(APPEND PC_LIBS
                -l$<TARGET_FILE_BASE_NAME:brotlidec> -l$<TARGET_FILE_BASE_NAME:brotlicommon>
                -l$<TARGET_FILE_BASE_NAME:brotlienc> -l$<TARGET_FILE_BASE_NAME:jxl>
            )
        endif()
        if(SDLIMAGE_JXL_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
            list(APPEND INSTALL_EXTRA_TARGETS ${jxl_install_libs})
        endif()
        set_target_properties(${jxl_lib} PROPERTIES EXPORT_NAME external_libjxl)
        add_library(SDL3_image::external_libjxl ALIAS ${jxl_lib})
        if(NOT TARGET libjxl::libjxl)
            add_library(libjxl::libjxl ALIAS ${jxl_lib})
        endif()
    else()
        find_package(libjxl ${required})
        if(libjxl_FOUND)
            message(STATUS "${PROJECT_NAME}: Using system libjxl")
            set(SDLIMAGE_JXL_ENABLED TRUE)
            if(NOT SDLIMAGE_JXL_SHARED)
                list(APPEND PC_REQUIRES libjxl)
                list(APPEND INSTALL_EXTRA_CMAKE_MODULES cmake/Findlibjxl.cmake)
            endif()
        else()
            message(${fatal_error} "libjxl NOT found")
        endif()
    endif()
    if(SDLIMAGE_JXL_ENABLED)
        target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_JXL)
        if(SDLIMAGE_JXL_SHARED)
            target_include_directories(${sdl3_image_target_name} PRIVATE
                $<TARGET_PROPERTY:libjxl::libjxl,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:libjxl::libjxl,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:libjxl::libjxl,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
            target_get_dynamic_library(dynamic_jxl libjxl::libjxl)
            message(STATUS "Dynamic libjxl: ${dynamic_jxl}")
            target_compile_definitions(${sdl3_image_target_name} PRIVATE "LOAD_JXL_DYNAMIC=\"${dynamic_jxl}\"")
            if(SDLIMAGE_JXL_VENDORED)
                add_dependencies(${sdl3_image_target_name} libjxl::libjxl)
            endif()
        else()
            target_link_libraries(${sdl3_image_target_name} PRIVATE libjxl::libjxl)
        endif()
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS LBM)
set(SDLIMAGE_LBM_ENABLED FALSE)
if(SDLIMAGE_LBM)
    set(SDLIMAGE_LBM_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_LBM)
endif()

list(APPEND SDLIMAGE_BACKENDS PCX)
set(SDLIMAGE_PCX_ENABLED FALSE)
if(SDLIMAGE_PCX)
    set(SDLIMAGE_PCX_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_PCX)
endif()

list(APPEND SDLIMAGE_BACKENDS PNG)
set(SDLIMAGE_PNG_ENABLED FALSE)
if(SDLIMAGE_PNG)
    if(SDLIMAGE_BACKEND_STB OR SDLIMAGE_BACKEND_WIC OR SDLIMAGE_BACKEND_IMAGEIO)
        set(SDLIMAGE_PNG_ENABLED TRUE)
    else()
        if(SDLIMAGE_PNG_VENDORED)
            set(SDLIMAGE_PNG_ENABLED TRUE)
            message(STATUS "${PROJECT_NAME}: Using vendored libpng")
            set(PNG_TESTS OFF CACHE BOOL "Build PNG Tests" FORCE)
            set(SKIP_INSTALL_EXPORT TRUE)
            sdl_check_project_in_subfolder(external/libpng libpng SDLIMAGE_VENDORED)
            add_subdirectory(external/libpng EXCLUDE_FROM_ALL)
            if(SDLIMAGE_PNG_SHARED)
                set(PNG_LIBRARY png_shared)
            else()
                set(PNG_LIBRARY png_static)
            endif()
            add_library(PNG::PNG ALIAS ${PNG_LIBRARY})
            set_property(TARGET ${PNG_LIBRARY} PROPERTY DEBUG_POSTFIX "")
            target_include_directories(${sdl3_image_target_name} PRIVATE external/libpng)
            if(SDLIMAGE_PNG_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
                list(APPEND INSTALL_EXTRA_TARGETS ${PNG_LIBRARY})
            endif()
            set_target_properties(${PNG_LIBRARY} PROPERTIES EXPORT_NAME external_libpng)
            add_library(SDL3_image::external_libpng ALIAS ${PNG_LIBRARY})
            if(NOT SDLIMAGE_PNG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${PNG_LIBRARY}>)
                if(SDLIMAGE_ZLIB_VENDORED)
                    list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${ZLIB_LIBRARY}>)
                else()
                    list(APPEND PC_REQUIRES zlib)
                endif()
            endif()
        else()
            find_package(PNG ${required})
            if(PNG_FOUND)
                message(STATUS "${PROJECT_NAME}: Using system libpng")
                set(SDLIMAGE_PNG_ENABLED TRUE)
                if(NOT SDLIMAGE_PNG_SHARED)
                    list(APPEND PC_REQUIRES libpng)
                endif()
            else()
                message(${FATAL_ERROR} "libpng NOT found")
            endif()
        endif()
        if(SDLIMAGE_PNG_ENABLED)
            if(SDLIMAGE_PNG_SHARED)
                target_include_directories(${sdl3_image_target_name} PRIVATE
                    $<TARGET_PROPERTY:PNG::PNG,INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:PNG::PNG,INTERFACE_INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:PNG::PNG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                )
                target_get_dynamic_library(dynamic_png PNG::PNG)
                message(STATUS "Dynamic libpng: ${dynamic_png}")
                target_compile_definitions(${sdl3_image_target_name} PRIVATE "LOAD_PNG_DYNAMIC=\"${dynamic_png}\"")
                if(SDLIMAGE_PNG_VENDORED)
                    add_dependencies(${sdl3_image_target_name} PNG::PNG)
                endif()
            else()
                target_link_libraries(${sdl3_image_target_name} PRIVATE PNG::PNG)
            endif()
        endif()
    endif()
    if(SDLIMAGE_PNG_ENABLED)
        target_compile_definitions(${sdl3_image_target_name} PRIVATE
            LOAD_PNG
            SDL_IMAGE_SAVE_PNG=$<BOOL:${SDLIMAGE_PNG_SAVE}>
        )
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS PNM)
set(SDLIMAGE_PNM_ENABLED FALSE)
if(SDLIMAGE_PNM)
    set(SDLIMAGE_PNM_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_PNM)
endif()

list(APPEND SDLIMAGE_BACKENDS QOI)
set(SDLIMAGE_QOI_ENABLED FALSE)
if(SDLIMAGE_QOI)
    set(SDLIMAGE_QOI_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_QOI)
endif()

list(APPEND SDLIMAGE_BACKENDS SVG)
set(SDLIMAGE_SVG_ENABLED FALSE)
if(SDLIMAGE_SVG)
    set(SDLIMAGE_SVG_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_SVG)
endif()

list(APPEND SDLIMAGE_BACKENDS TGA)
set(SDLIMAGE_TGA_ENABLED FALSE)
if(SDLIMAGE_TGA)
    set(SDLIMAGE_TGA_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_TGA)
endif()

list(APPEND SDLIMAGE_BACKENDS TIF)
set(SDLIMAGE_TIF_ENABLED FALSE)
if(SDLIMAGE_TIF)
    if(SDLIMAGE_BACKEND_IMAGEIO OR SDLIMAGE_BACKEND_WIC)
        set(SDLIMAGE_TIF_ENABLED TRUE)
    else()
        if(SDLIMAGE_TIF_VENDORED)
            set(SDLIMAGE_TIF_ENABLED TRUE)
            message(STATUS "${PROJECT_NAME}: Using vendored libtiff")
            # tiff-tests variable is used by vendored libtiff
            set(tiff-tests OFF CACHE BOOL "libtiff: tests" FORCE)
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
            sdl_check_project_in_subfolder(external/libtiff libtiff SDLIMAGE_VENDORED)
            set(BUILD_SHARED_LIBS ${SDLIMAGE_TIF_SHARED})
            add_subdirectory(external/libtiff EXCLUDE_FROM_ALL)
            add_library(TIFF::TIFF ALIAS tiff)
            set(SDL2IMAGE_TIF_TARGET "TIFF::tiff")
            if(SDLIMAGE_TIF_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
              list(APPEND INSTALL_EXTRA_TARGETS tiff)
            endif()
            set_target_properties(tiff PROPERTIES EXPORT_NAME external_libtiff)
            add_library(SDL3_image::external_libtiff ALIAS tiff)
            if(NOT SDLIMAGE_TIF_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:tiff>)
            endif()
        else()
            find_package(TIFF ${required})
            if(TIFF_FOUND)
                message(STATUS "${PROJECT_NAME}: Using system libtiff")
                set(SDLIMAGE_TIF_ENABLED TRUE)
                if(NOT SDLIMAGE_TIF_SHARED)
                    list(APPEND PC_REQUIRES libtiff-4)
                endif()
                if(TARGET TIFF::tiff)
                    # introduced in CMake 3.28
                    # TIFF::TIFF still exists, but it is an INTERFACE library linking to TIFF::tiff (no ALIAS library)
                    set(SDL2IMAGE_TIF_TARGET "TIFF::tiff")
                else()
                    set(SDL2IMAGE_TIF_TARGET "TIFF::TIFF")
                endif()
            else()
                message(${fatal_error} "libtiff NOT found")
            endif()
        endif()
        if(SDLIMAGE_TIF_ENABLED)
            target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_TIF)
            if(SDLIMAGE_TIF_SHARED)
                target_include_directories(${sdl3_image_target_name} PRIVATE
                    $<TARGET_PROPERTY:${SDL2IMAGE_TIF_TARGET},INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:${SDL2IMAGE_TIF_TARGET},INTERFACE_INCLUDE_DIRECTORIES>
                    $<TARGET_PROPERTY:${SDL2IMAGE_TIF_TARGET},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                )
                target_get_dynamic_library(dynamic_tif ${SDL2IMAGE_TIF_TARGET})
                message(STATUS "Dynamic libtiff: ${dynamic_tif}")
                target_compile_definitions(${sdl3_image_target_name} PRIVATE "LOAD_TIF_DYNAMIC=\"${dynamic_tif}\"")
                if(SDLIMAGE_TIF_VENDORED)
                    add_dependencies(${sdl3_image_target_name} ${SDL2IMAGE_TIF_TARGET})
                endif()
            else()
                target_link_libraries(${sdl3_image_target_name} PRIVATE ${SDL2IMAGE_TIF_TARGET})
            endif()
        endif()
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS WEBP)
set(SDLIMAGE_WEBP_ENABLED FALSE)
if(SDLIMAGE_WEBP)
    # missing cpufeatures
    if(SDLIMAGE_WEBP_VENDORED)
        set(SDLIMAGE_WEBP_ENABLED TRUE)
        message(STATUS "${PROJECT_NAME}: Using vendored libwebp")
        sdl_check_project_in_subfolder(external/libwebp libwebp SDLIMAGE_VENDORED)
        set(BUILD_SHARED_LIBS ${SDLIMAGE_WEBP_SHARED})
        add_subdirectory(external/libwebp EXCLUDE_FROM_ALL)
        target_include_directories(${sdl3_image_target_name} PRIVATE external/libwebp/src)
        add_library(WebP::webp ALIAS webp)
        add_library(WebP::webpdemux ALIAS webpdemux)
        if(SDLIMAGE_WEBP_SHARED OR NOT SDLIMAGE_BUILD_SHARED_LIBS)
            list(APPEND INSTALL_EXTRA_TARGETS webp webpdemux)
            if(NOT SDLIMAGE_WEBP_SHARED)
                list(APPEND INSTALL_EXTRA_TARGETS sharpyuv)
            endif()
        endif()
        set_target_properties(webp PROPERTIES EXPORT_NAME "external_libwebp")
        set_target_properties(webpdemux PROPERTIES EXPORT_NAME "external_webpdemux")
        set_target_properties(sharpyuv PROPERTIES EXPORT_NAME "external_sharpyuv")
        add_library(SDL3_image::external_libwebp ALIAS webp)
    else()
        find_package(webp ${required})
        if(webp_FOUND)
            message(STATUS "${PROJECT_NAME}: Using system libwebp")
            set(SDLIMAGE_WEBP_ENABLED TRUE)
            if(NOT SDLIMAGE_WEBP_SHARED)
              list(APPEND PC_REQUIRES libwebp libwebpdemux)
              list(APPEND INSTALL_EXTRA_CMAKE_MODULES cmake/Findwebp.cmake)
            endif()
        else()
            message(${fatal_error} "libwebp NOT found")
        endif()
    endif()
    if(SDLIMAGE_WEBP_ENABLED)
        target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_WEBP)
        if(SDLIMAGE_WEBP_SHARED)
            target_include_directories(${sdl3_image_target_name} PRIVATE
                $<TARGET_PROPERTY:WebP::webp,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:WebP::webp,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:WebP::webp,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:WebP::webpdemux,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:WebP::webpdemux,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:WebP::webpdemux,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
            target_get_dynamic_library(dynamic_webpdemux WebP::webpdemux)
            message(STATUS "Dynamic libwebpdemux: ${dynamic_webpdemux}")
            target_compile_definitions(${sdl3_image_target_name} PRIVATE "LOAD_WEBPDEMUX_DYNAMIC=\"${dynamic_webpdemux}\"")
            target_get_dynamic_library(dynamic_webp WebP::webp)
            message(STATUS "Dynamic libwebp: ${dynamic_webp}")
            target_compile_definitions(${sdl3_image_target_name} PRIVATE "LOAD_WEBP_DYNAMIC=\"${dynamic_webp}\"")
            if(SDLIMAGE_WEBP_VENDORED)
                add_dependencies(${sdl3_image_target_name} WebP::webp WebP::webpdemux)
            endif()
        else()
            target_link_libraries(${sdl3_image_target_name} PRIVATE WebP::webp WebP::webpdemux)
        endif()
    endif()
endif()

list(APPEND SDLIMAGE_BACKENDS XCF)
set(SDLIMAGE_XCF_ENABLED FALSE)
if(SDLIMAGE_XCF)
    set(SDLIMAGE_XCF_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_XCF)
endif()

list(APPEND SDLIMAGE_BACKENDS XPM)
set(SDLIMAGE_XPM_ENABLED FALSE)
if(SDLIMAGE_XPM)
    set(SDLIMAGE_XPM_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_XPM)
endif()

list(APPEND SDLIMAGE_BACKENDS XV)
set(SDLIMAGE_XV_ENABLED FALSE)
if(SDLIMAGE_XV)
    set(SDLIMAGE_XV_ENABLED TRUE)
    target_compile_definitions(${sdl3_image_target_name} PRIVATE LOAD_XV)
endif()

# Restore BUILD_SHARED_LIBS
set(BUILD_SHARED_LIBS ${SDLIMAGE_BUILD_SHARED_LIBS})

if(SDLIMAGE_INSTALL)
    install(
        TARGETS ${sdl3_image_target_name}
        EXPORT SDL3ImageExports
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/SDL3_image/SDL_image.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3_image" COMPONENT DEVEL
    )
    if(BUILD_SHARED_LIBS)
        set(pdbdir "${CMAKE_INSTALL_BINDIR}")
    else()
        set(pdbdir "${CMAKE_INSTALL_LIBDIR}")
    endif()
    if(MSVC)
        SDL_install_pdb("${sdl3_image_target_name}" "${pdbdir}")
    endif()

    if(INSTALL_EXTRA_TARGETS)
        set_property(TARGET ${INSTALL_EXTRA_TARGETS} PROPERTY PUBLIC_HEADER "")
        set(archive_destination)
        if(NOT BUILD_SHARED_LIBS)
            set(archive_destination ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel)
        endif()
        install(
            TARGETS ${INSTALL_EXTRA_TARGETS}
            EXPORT SDL3ImageExports
            ${archive_destination}
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
        )
        if(MSVC)
            foreach(tgt IN LISTS INSTALL_EXTRA_TARGETS)
                SDL_install_pdb("${tgt}" "${pdbdir}")
            endforeach()
        endif()
    endif()

    if(WIN32 AND NOT MINGW)
        set(SDLIMAGE_INSTALL_CMAKEDIR_ROOT_DEFAULT "cmake")
    else()
        set(SDLIMAGE_INSTALL_CMAKEDIR_ROOT_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake")
    endif()
    set(SDLIMAGE_INSTALL_CMAKEDIR_ROOT "${SDLIMAGE_INSTALL_CMAKEDIR_ROOT_DEFAULT}" CACHE STRING "Root folder where to install SDL3_imageConfig.cmake related files (SDL3_image subfolder for MSVC projects)")
    set(SDLIMAGE_PKGCONFIG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

    if(WIN32 AND NOT MINGW)
        set(SDLIMAGE_INSTALL_CMAKEDIR "${SDLIMAGE_INSTALL_CMAKEDIR_ROOT}")
    else()
        set(SDLIMAGE_INSTALL_CMAKEDIR "${SDLIMAGE_INSTALL_CMAKEDIR_ROOT}/SDL3_image")
    endif()

    configure_package_config_file(cmake/SDL3_imageConfig.cmake.in SDL3_imageConfig.cmake
        NO_SET_AND_CHECK_MACRO
        INSTALL_DESTINATION "${SDLIMAGE_INSTALL_CMAKEDIR}"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL3_imageConfigVersion.cmake"
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_imageConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_imageConfigVersion.cmake"
            ${INSTALL_EXTRA_CMAKE_MODULES}
        DESTINATION "${SDLIMAGE_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )
    install(EXPORT SDL3ImageExports
        FILE ${sdl3_image_target_name}-targets.cmake
        NAMESPACE SDL3_image::
        DESTINATION "${SDLIMAGE_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )

    file(RELATIVE_PATH SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${CMAKE_INSTALL_PREFIX}/${SDLIMAGE_PKGCONFIG_INSTALLDIR}" "${CMAKE_INSTALL_PREFIX}")
    string(REGEX REPLACE "[/]+$" "" SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
    set(SDL_PKGCONFIG_PREFIX "\${pcfiledir}/${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")

    string(JOIN " " PC_REQUIRES ${PC_REQUIRES})
    string(JOIN " " PC_LIBS ${PC_LIBS})
    configure_file(cmake/sdl3-image.pc.in sdl3-image.pc @ONLY)

    # Always install sdl3-image.pc file: libraries might be different between config modes
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl3-image.pc"
        DESTINATION "${SDLIMAGE_PKGCONFIG_INSTALLDIR}" COMPONENT devel)

    install(FILES "LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
        COMPONENT library
    )

    if(SDLIMAGE_INSTALL_CPACK)
        if(MSVC)
            set(CPACK_GENERATOR "ZIP")
        else()
            set(CPACK_GENERATOR "TGZ")
        endif()
        configure_file(cmake/CPackProjectConfig.cmake.in CPackProjectConfig.cmake @ONLY)
        set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/CPackProjectConfig.cmake")
        # CPACK_SOURCE_PACKAGE_FILE_NAME must end with "-src" (so we can block creating a source archive)
        set(CPACK_SOURCE_PACKAGE_FILE_NAME "SDL${PROJECT_VERSION_MAJOR}-${PROJECT_VERSION}-src")
        set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
        include(CPack)
    endif()

    if(SDLIMAGE_INSTALL_MAN)
        sdl_get_git_revision_hash(SDLIMAGE_REVISION)
        SDL_generate_manpages(
            HEADERS_DIR "${PROJECT_SOURCE_DIR}/include/SDL3_image"
            SYMBOL "IMG_Init"
            WIKIHEADERS_PL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build-scripts/wikiheaders.pl"
            REVISION "${SDLIMAGE_REVISION}"
        )
    endif()
endif()

if(SDLIMAGE_SAMPLES)
    add_executable(showanim examples/showanim.c)
    add_executable(showimage examples/showimage.c)

    foreach(prog showanim showimage)
        sdl_add_warning_options(${prog} WARNING_AS_ERROR ${SDLIMAGE_WERROR})
        target_link_libraries(${prog} PRIVATE SDL3_image::${sdl3_image_target_name})
        target_link_libraries(${prog} PRIVATE ${sdl3_target_name})

        if(SDLIMAGE_SAMPLES_INSTALL)
            install(TARGETS ${prog}
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            )
            if(MSVC)
                SDL_install_pdb("${prog}" "${pdbdir}")
            endif()
        endif()
    endforeach()
endif()

if(SDLIMAGE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()


set(available_deps)
set(unavailable_deps)
foreach(dep IN LISTS SDLIMAGE_BACKENDS)
  set(var SDLIMAGE_${dep}_ENABLED)
  if(NOT DEFINED ${var})
    message(AUTHOR_WARNING "${var} not defined")
  endif()
  if(${var})
    list(APPEND available_deps ${dep})
  else()
    list(APPEND unavailable_deps ${dep})
  endif()
endforeach()
string(JOIN " " avail_str ${available_deps})
string(TOLOWER "${avail_str}" avail_str)
string(JOIN " " unavail_str ${unavailable_deps})
string(TOLOWER "${unavail_str}" unavail_str)
message(STATUS "SDL3_image backends:")
message(STATUS "- enabled:  ${avail_str}")
message(STATUS "- disabled: ${unavail_str}")
