cmake_minimum_required(VERSION 3.14)

# See docs/release_checklist.md
set(MAJOR_VERSION 2)
set(MINOR_VERSION 5)
set(MICRO_VERSION 0)
set(FULL_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}")
set(SDL_REQUIRED_VERSION 2.0.8)

# Set defaults preventing destination file conflicts
set(SDL_CMAKE_DEBUG_POSTFIX "d"
    CACHE STRING "Name suffix for debug builds")

mark_as_advanced(CMAKE_IMPORT_LIBRARY_SUFFIX SDL_CMAKE_DEBUG_POSTFIX)

# Calculate a libtool-like version number
math(EXPR BINARY_AGE "${MINOR_VERSION} * 100 + ${MICRO_VERSION}")
math(EXPR SDL2_IMAGE_DEVELOPMENT "${MINOR_VERSION} % 2")
if (SDL2_IMAGE_DEVELOPMENT)
    # Development branch, 2.5.1 -> libSDL2_image-2.0.so.0.501.0
    set(INTERFACE_AGE 0)
else()
    # Stable branch, 2.6.1 -> libSDL2_image-2.0.so.0.600.1
    set(INTERFACE_AGE ${MICRO_VERSION})
endif()

# Increment this if there is an incompatible change - but if that happens,
# we should rename the library from SDL2 to SDL3, at which point this would
# reset to 0 anyway.
set(LT_MAJOR "0")

math(EXPR LT_AGE "${BINARY_AGE} - ${INTERFACE_AGE}")
math(EXPR LT_CURRENT "${LT_MAJOR} + ${LT_AGE}")
set(LT_REVISION "${INTERFACE_AGE}")
# For historical reasons, the library name redundantly includes the major
# version twice: libSDL2_image-2.0.so.0.
# TODO: in SDL 3, set the OUTPUT_NAME to plain SDL3_image, which will simplify
# it to libSDL3_image.so.0
set(LT_RELEASE "2.0")
set(LT_VERSION "${LT_MAJOR}.${LT_AGE}.${LT_REVISION}")

# The following should match the versions in the Xcode project file.
# Each version is 1 higher than you might expect, for compatibility
# with libtool: macOS ABI versioning is 1-based, unlike other platforms
# which are normally 0-based.
math(EXPR DYLIB_CURRENT_VERSION_MAJOR "${LT_MAJOR} + ${LT_AGE} + 1")
math(EXPR DYLIB_CURRENT_VERSION_MINOR "${LT_REVISION}")
math(EXPR DYLIB_COMPAT_VERSION_MAJOR "${LT_MAJOR} + 1")
set(DYLIB_CURRENT_VERSION "${DYLIB_CURRENT_VERSION_MAJOR}.${DYLIB_CURRENT_VERSION_MINOR}.0")
# For historical reasons this is 3.0.0 rather than the expected 1.0.0
set(DYLIB_COMPATIBILITY_VERSION "3.0.0")

project(SDL2_image LANGUAGES C
    VERSION "${FULL_VERSION}")

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

if (NOT (TARGET SDL2::SDL2 OR TARGET SDL2::SDL2-static))
    find_package(SDL2 REQUIRED)
endif()

function(read_absolute_symlink DEST PATH)
    file(READ_SYMLINK "${PATH}" p)
    if (NOT IS_ABSOLUTE p)
        get_filename_component(pdir "${PATH}" DIRECTORY)
        set(p "${pdir}/${p}")
    endif()
    set("${DEST}" "${p}" PARENT_SCOPE)
endfunction()

function(win32_implib_identify_dll DEST IMPLIB)
    cmake_parse_arguments(ARGS "NOTFATAL" "" "" ${ARGN})
    if (CMAKE_DLLTOOL)
        execute_process(
            COMMAND "${CMAKE_DLLTOOL}" --identify "${IMPLIB}"
            RESULT_VARIABLE retcode
            OUTPUT_VARIABLE stdout
            ERROR_VARIABLE stderr)
        if (NOT retcode EQUAL 0)
            if (NOT ARGS_NOTFATAL)
                message(FATAL_ERROR "${CMAKE_DLLTOOL} failed.")
            else()
                set("${DEST}" "${DEST}-NOTFOUND" PARENT_SCOPE)
                return()
            endif()
        endif()
        string(STRIP "${stdout}" result)
        set(${DEST} "${result}" PARENT_SCOPE)
    elseif (MSVC)
        get_filename_component(CMAKE_C_COMPILER_DIRECTORY "${CMAKE_C_COMPILER}" DIRECTORY CACHE)
        find_program(CMAKE_DUMPBIN NAMES dumpbin PATHS "${CMAKE_C_COMPILER_DIRECTORY}")
        if (CMAKE_DUMPBIN)
            execute_process(
                COMMAND "${CMAKE_DUMPBIN}" "-headers" "${IMPLIB}"
                RESULT_VARIABLE retcode
                OUTPUT_VARIABLE stdout
                ERROR_VARIABLE stderr)
            if (NOT retcode EQUAL 0)
                if (NOT ARGS_NOTFATAL)
                    message(FATAL_ERROR "dumpbin failed.")
                else()
                    set(${DEST} "${DEST}-NOTFOUND" PARENT_SCOPE)
                    return()
                endif()
            endif()
            string(REGEX MATCH "DLL name[ ]+:[ ]+([^\n]+)\n" match "${stdout}")
            if (NOT match)
                if (NOT ARGS_NOTFATAL)
                    message(FATAL_ERROR "dumpbin did not find any associated dll for ${IMPLIB}.")
                else()
                    set(${DEST} "${DEST}-NOTFOUND" PARENT_SCOPE)
                    return()
                endif()
            endif()
            set(result "${CMAKE_MATCH_1}")
            set(${DEST} "${result}" PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Cannot find dumpbin, please set CMAKE_DUMPBIN cmake variable")
        endif()
    else()
        if (NOT ARGS_NOTFATAL)
            message(FATAL_ERROR "Don't know how to identify dll from import library. Set CMAKE_DLLTOOL (for mingw) or CMAKE_DUMPBIN (for MSVC)")
        else()
            set(${DEST} "${DEST}-NOTFOUND")
        endif()
    endif()
endfunction()

function(target_get_dynamic_library DEST TARGET)
    set(result)
    get_target_property(alias "${TARGET}" ALIASED_TARGET)
    while (alias)
        set(TARGET "${alias}")
        get_target_property(alias "${TARGET}" ALIASED_TARGET)
    endwhile()
    if (WIN32)
        # Use the target dll of the import library
        set(props_to_check IMPORTED_IMPLIB)
        if (CMAKE_BUILD_TYPE)
            list(APPEND props_to_check IMPORTED_IMPLIB_${CMAKE_BUILD_TYPE})
        endif()
        list(APPEND props_to_check IMPORTED_LOCATION)
        if (CMAKE_BUILD_TYPE)
            list(APPEND props_to_check IMPORTED_LOCATION_${CMAKE_BUILD_TYPE})
        endif()
        foreach (config_type ${CMAKE_CONFIGURATION_TYPES} RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
            list(APPEND props_to_check IMPORTED_IMPLIB_${config_type})
            list(APPEND props_to_check IMPORTED_LOCATION_${config_type})
        endforeach()

        foreach(prop_to_check ${props_to_check})
            if (NOT result)
                get_target_property(propvalue "${TARGET}" ${prop_to_check})
                if (propvalue AND EXISTS "${propvalue}")
                    win32_implib_identify_dll(result "${propvalue}" NOTFATAL)
                endif()
            endif()
        endforeach()
    else()
        # 1. find the target library a file might be symbolic linking to
        # 2. find all other files in the same folder that symolic link to it
        # 3. sort all these files, and select the 2nd item
        set(props_to_check IMPORTED_LOCATION)
        if (CMAKE_BUILD_TYPE)
            list(APPEND props_to_check IMPORTED_LOCATION_${CMAKE_BUILD_TYPE})
        endif()
        foreach (config_type ${CMAKE_CONFIGURATION_TYPES} RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
            list(APPEND props_to_check IMPORTED_LOCATION_${config_type})
        endforeach()
        foreach(prop_to_check ${props_to_check})
            if (NOT result)
                get_target_property(propvalue "${TARGET}" ${prop_to_check})
                if (EXISTS "${propvalue}")
                    while (IS_SYMLINK "${propvalue}")
                        read_absolute_symlink(propvalue "${propvalue}")
                    endwhile()
                    get_filename_component(libdir "${propvalue}" DIRECTORY)
                    file(GLOB subfiles "${libdir}/*")
                    set(similar_files "${propvalue}")
                    foreach(subfile ${subfiles})
                        if (IS_SYMLINK "${subfile}")
                            read_absolute_symlink(subfile_target "${subfile}")
                            if (subfile_target STREQUAL propvalue)
                                list(APPEND similar_files "${subfile}")
                            endif()
                        endif()
                    endforeach()
                    list(SORT similar_files)
                    list(LENGTH similar_files eq_length)
                    if (eq_length GREATER 1)
                        list(GET similar_files 1 item)
                    else()
                        list(GET similar_files 0 item)
                    endif()
                    get_filename_component(result "${item}" NAME)
                endif()
            endif()
        endforeach()
    endif()
    if (NOT result)
        set (result "$<TARGET_FILE_NAME:${TARGET}>")
    endif()
    set(${DEST} ${result} PARENT_SCOPE)
endfunction()

# Workaround for Ubuntu 20.04's SDL being older than
# https://github.com/libsdl-org/SDL/issues/3531
if (NOT (TARGET SDL2::SDL2 OR TARGET SDL2::SDL2-static))
    find_library(SDL2_LIBRARY
        NAMES SDL2
        HINTS "${SDL2_EXEC_PREFIX}"
        REQUIRED)
    if (NOT SDL2_LIBRARY)
        message(FATAL_ERROR "Could not find SDL2 library. Please define SDL2_EXEC_PREFIX and/or SLD2_LIBRARY")
    endif()
    add_library(SDL2::SDL2 SHARED IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${SDL2_LIBRARY}")
endif()

# Set PROJECT_VERSION of subprojects to "" if it's project call does not set VERSION
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

# Alloc cmake_dependent_option to use "Full Condition Syntax"
if (POLICY CMP0127)
    cmake_policy(SET CMP0127 NEW)
endif()

# OpenGL is required by dependencies of (dependencies of) some vendored libraries
if (NOT DEFINED OpenGL_GL_PREFERENCE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(BUILD_TESTS "Build unit tests?" OFF)
cmake_dependent_option(INSTALL_TESTS "Install unit tests?" OFF BUILD_TESTS OFF)
option(VENDORED_DEFAULT "Default value for *_VENDORED options. Can be overridden for each library. Is only used in the first configure run." ON)
option(SHARED_DEFAULT "Default value for *_SHARED options. Can be overridden for each library. Is only used in the first configure run." ON)
option(SDL2_IMAGE_DISABLE_INSTALL "Disable installing SDL2_image" OFF)

option(BACKEND_STB "Use stb_image for loading JPEG and PNG files" ON)
cmake_dependent_option(BACKEND_WIC "Add WIC backend (Windows Imaging Component)" OFF "WIN32" OFF)
cmake_dependent_option(BACKEND_IMAGEIO "Use native Mac OS X frameworks for loading images" ON APPLE OFF)

option(SUPPORT_AVIF "Support loading AVIF images" OFF)
option(SUPPORT_BMP "Support loading BMP images" ON)
option(SUPPORT_GIF "Support loading GIF images" ON)
option(SUPPORT_JPG "Support loading JPEG images" ON)
option(SUPPORT_JXL "Support loading JXL images" OFF)
option(SUPPORT_LBM "Support loading LBM images" ON)
option(SUPPORT_PCX "Support loading PCX images" ON)
option(SUPPORT_PNG "Support loading PNG images" ON)
option(SUPPORT_PNM "Support loading PNM images" ON)
option(SUPPORT_QOI "Support loading QOI images" ON)
option(SUPPORT_SVG "Support loading SVG images" ON)
option(SUPPORT_TGA "Support loading TGA images" ON)
option(SUPPORT_TIF "Support loading TIFF images" OFF)
option(SUPPORT_WEBP "Support loading WEBP images" OFF)
option(SUPPORT_XCF "Support loading XCF images" ON)
option(SUPPORT_XPM "Support loading XPM images" ON)
option(SUPPORT_XV "Support loading XV images" ON)

option(BUILD_SAMPLES "Build the SDL2_image sample program(s)" ON)
option(BUILD_SHARED_LIBS "Build the library as a shared library" ON)
# FIXME: use vendored libavif when available
set(SUPPORT_AVIF_VENDORED OFF)
#cmake_dependent_option(SUPPORT_AVIF_VENDORED "Use vendored libavif" ${VENDORED_DEFAULT} SUPPORT_AVIF OFF)
cmake_dependent_option(SUPPORT_AVIF_SHARED "Dynamically load AVIF support (requires shared libavif)" ${SHARED_DEFAULT}
    "SUPPORT_AVIF;(SUPPORT_AVIF_VENDORED AND BUILD_SHARED_LIBS) OR NOT SUPPORT_AVIF_VENDORED" OFF)

cmake_dependent_option(SUPPORT_JPG_VENDORED "Use vendored libjpeg" ${VENDORED_DEFAULT} "SUPPORT_JPG;NOT BACKEND_WIC;NOT BACKEND_STB;NOT BACKEND_IMAGEIO;" OFF)
cmake_dependent_option(SUPPORT_JPG_SHARED "Dynamically load JPG support (requires shared libjpeg)" ${SHARED_DEFAULT}
    "SUPPORT_JPG;NOT BACKEND_WIC;NOT BACKEND_STB;NOT BACKEND_IMAGEIO;(SUPPORT_JPG_VENDORED AND BUILD_SHARED_LIBS) OR NOT SUPPORT_JPG_VENDORED" OFF)

cmake_dependent_option(SUPPORT_JXL_VENDORED "Use vendored libjxl" ${VENDORED_DEFAULT} SUPPORT_JXL OFF)
cmake_dependent_option(SUPPORT_JXL_SHARED "Dynamically load JXL support (requires shared libjxl)" ${SHARED_DEFAULT}
    "SUPPORT_JXL;(SUPPORT_JXL_VENDORED AND BUILD_SHARED_LIBS) OR NOT SUPPORT_JXL_VENDORED" OFF)

cmake_dependent_option(SUPPORT_PNG_VENDORED "Use vendored PNG" ${VENDORED_DEFAULT} "SUPPORT_PNG;NOT BACKEND_WIC;NOT BACKEND_STB;NOT BACKEND_IMAGEIO" OFF)
cmake_dependent_option(SUPPORT_PNG_SHARED "Dynamically load PNG support (requires shared libpng)" ${SHARED_DEFAULT}
    "SUPPORT_PNG;NOT BACKEND_WIC;NOT BACKEND_STB;NOT BACKEND_IMAGEIO;(SUPPORT_PNG_VENDORED AND BUILD_SHARED_LIBS) OR NOT SUPPORT_PNG_VENDORED" OFF)
cmake_dependent_option(SUPPORT_ZLIB_VENDORED "Use vendored ZLIB (required by PNG)" ${VENDORED_DEFAULT} SUPPORT_PNG_VENDORED OFF)

cmake_dependent_option(SUPPORT_TIF_VENDORED "Use vendored libtiff" ${VENDORED_DEFAULT} SUPPORT_TIF OFF)
cmake_dependent_option(SUPPORT_TIF_SHARED "Dynamically load TIFF support (requires shared libtiff)" ${SHARED_DEFAULT}
    "SUPPORT_TIF;(SUPPORT_TIF_VENDORED AND BUILD_SHARED_LIBS) OR NOT SUPPORT_TIF_VENDORED" OFF)

cmake_dependent_option(SUPPORT_WEBP_VENDORED "Use vendored WEBP" ${VENDORED_DEFAULT} SUPPORT_WEBP OFF)
cmake_dependent_option(SUPPORT_WEBP_SHARED "Dynamically load WEBP support (requires shared libwebp)" ${SHARED_DEFAULT}
    "SUPPORT_WEBP;(SUPPORT_WEBP_VENDORED AND BUILD_SHARED_LIBS) OR NOT SUPPORT_WEBP_VENDORED" OFF)

# FIXME: ????
if (NOT BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(BUILD_SHARED_LIBS)
    set(sdl2_image_export_name SDL2_image)
    set(sdl2_image_install_name_infix shared)
else()
    set(sdl2_image_export_name SDL2_image-static)
    set(sdl2_image_install_name_infix static)
endif()

add_library(SDL2_image
    IMG.c
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
    IMG_WIC.c
    IMG_webp.c
    IMG_xcf.c
    IMG_xpm.c
    IMG_xv.c
    IMG_xxx.c
    )
add_library(SDL2_image::${sdl2_image_export_name} ALIAS SDL2_image)

target_compile_definitions(SDL2_image PRIVATE
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
    )

if (BACKEND_STB)
    target_compile_definitions(SDL2_image PRIVATE USE_STBIMAGE)
endif()

if (APPLE)
    if (BACKEND_IMAGEIO)
        target_link_options(SDL2_image PRIVATE -Wl,-framework,ApplicationServices)
        target_link_libraries(SDL2_image PRIVATE objc)
        target_sources(SDL2_image PRIVATE
            IMG_ImageIO.m
            )
    else()
        target_compile_definitions(SDL2_image PRIVATE SDL_IMAGE_USE_COMMON_BACKEND)
    endif()
endif()

if (BACKEND_WIC)
    target_compile_definitions(SDL2_image PRIVATE SDL_IMAGE_USE_WIC_BACKEND)
endif()

if (WIN32 AND BUILD_SHARED_LIBS)
    target_sources(SDL2_image PRIVATE
        version.rc
        )
endif()

set(INSTALL_EXTRA_TARGETS)
set(PC_REQUIRES)
set(PC_LIBS)

# Save BUILD_SHARED_LIBS variable. We'll put this back after we have
# set up the vendored libraries (if any).
set(SDL2IMAGE_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

if (SUPPORT_AVIF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_AVIF)
    if (SUPPORT_AVIF_VENDORED)
        message(FATAL_ERROR "libavif is not vendored yet")
        if(NOT SUPPORT_AVIF_SHARED)
            set(BUILD_SHARED_LIBS OFF)
        endif()
        add_subdirectory(external/libavif EXCLUDE_FROM_ALL)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})
        list(APPEND INSTALL_EXTRA_TARGETS libavif)
        if (NOT SUPPORT_AVIF_SHARED)
            list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:avif>)
        endif()
    else()
        find_package(libavif REQUIRED)
        list(APPEND PC_REQUIRES libavif)
    endif()
    if (SUPPORT_AVIF_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:avif,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:avif,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:avif,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
        target_get_dynamic_library(dynamic_avif avif)
        message(STATUS "Dynamic libavif: ${dynamic_avif}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_AVIF_DYNAMIC=\"${dynamic_avif}\"")
        if (SUPPORT_AVIF_VENDORED)
            add_dependencies(SDL2_image avif)
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE avif)
    endif()
endif()

if (SUPPORT_BMP)
    target_compile_definitions(SDL2_image PRIVATE LOAD_BMP)
endif()

if (SUPPORT_GIF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_GIF)
endif()

if (SUPPORT_JPG)
    target_compile_definitions(SDL2_image PRIVATE LOAD_JPG)
    if (NOT BACKEND_STB AND NOT BACKEND_WIC AND NOT BACKEND_IMAGEIO)
        if (SUPPORT_JPG_VENDORED)
            set(jpeg OFF)
            if(NOT SUPPORT_JPG_SHARED)
                set(BUILD_SHARED_LIBS OFF)
            endif()
            add_subdirectory(external/jpeg EXCLUDE_FROM_ALL)
            set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})
            add_library(JPEG::JPEG ALIAS jpeg)
            list(APPEND INSTALL_EXTRA_TARGETS jpeg)
            if (NOT SUPPORT_JPG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:jpeg>)
            endif()
        else()
            find_package(JPEG REQUIRED)
            list(APPEND PC_REQUIRES libjpeg)
        endif()
        if (SUPPORT_JPG_SHARED)
            target_include_directories(SDL2_image PRIVATE
                $<TARGET_PROPERTY:JPEG::JPEG,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:JPEG::JPEG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                )
            target_get_dynamic_library(dynamic_jpeg JPEG::JPEG)
            message(STATUS "Dynamic libjpeg: ${dynamic_jpeg}")
            target_compile_definitions(SDL2_image PRIVATE "LOAD_JPG_DYNAMIC=\"${dynamic_jpeg}\"")
            if (SUPPORT_JPG_VENDORED)
                add_dependencies(SDL2_image JPEG::JPEG)
            endif()
        else()
            target_link_libraries(SDL2_image PRIVATE JPEG::JPEG)
        endif()
    endif()
endif()

if (SUPPORT_JXL)
    target_compile_definitions(SDL2_image PRIVATE LOAD_JXL)
    if (SUPPORT_JXL_VENDORED)
        set(BUILD_TESTING OFF)
        if(NOT SUPPORT_JXL_SHARED)
            set(BUILD_SHARED_LIBS OFF)
        endif()
        add_subdirectory(external/libjxl EXCLUDE_FROM_ALL)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})
        if (BUILD_SHARED_LIBS AND SUPPORT_JXL_SHARED)
            set(jxl_lib jxl)
            list(APPEND INSTALL_EXTRA_TARGETS brotlidec brotlicommon brotlienc ${jxl_lib})
            list(APPEND PC_LIBS
                -l$<TARGET_FILE_BASE_NAME:brotlidec> -l$<TARGET_FILE_BASE_NAME:brotlicommon>
                -l$<TARGET_FILE_BASE_NAME:brotlienc> -l$<TARGET_FILE_BASE_NAME:${jxl_lib}>)
        else()
            set(jxl_lib jxl-static)
            list(APPEND INSTALL_EXTRA_TARGETS brotlidec-static brotlicommon-static brotlienc-static hwy ${jxl_lib})
        endif()
    else()
        find_library(LIBJXL_LIBRARY NAMES jxl)
        if (NOT LIBJXL_LIBRARY)
            message(FATAL_ERROR "Could not find jxl library")
        endif()
        find_path(LIBJXL_INCLUDE NAMES jxl/decode.h)
        if (NOT LIBJXL_INCLUDE)
            message(FATAL_ERROR "Could not find jxl include directory")
        endif()
        add_library(jxl UNKNOWN IMPORTED)
        set_target_properties(jxl PROPERTIES
            IMPORTED_LOCATION "${LIBJXL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBJXL_INCLUDE}"
            )
        list(APPEND PC_REQUIRES libjxl)
        set(jxl_lib jxl)
    endif()
    if (SUPPORT_JXL_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:${jxl_lib},INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:${jxl_lib},INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:${jxl_lib},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
        target_get_dynamic_library(dynamic_jxl ${jxl_lib})
        message(STATUS "Dynamic libjxl: ${dynamic_jxl}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_JXL_DYNAMIC=\"${dynamic_jxl}\"")
        if (SUPPORT_JXL_VENDORED)
            add_dependencies(SDL2_image brotlidec brotlicommon brotlienc ${jxl_lib})
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE ${jxl_lib})
    endif()
endif()

if (SUPPORT_LBM)
    target_compile_definitions(SDL2_image PRIVATE LOAD_LBM)
endif()

if (SUPPORT_PCX)
    target_compile_definitions(SDL2_image PRIVATE LOAD_PCX)
endif()

if (SUPPORT_PNG)
    target_compile_definitions(SDL2_image PRIVATE LOAD_PNG)
    if (NOT BACKEND_STB AND NOT BACKEND_WIC AND NOT BACKEND_IMAGEIO)
        set(HAVE_LD_VERSION_SCRIPT OFF CACHE BOOL "" FORCE)
        if (SUPPORT_PNG_VENDORED)
            if(NOT SUPPORT_PNG_SHARED)
                set(BUILD_SHARED_LIBS OFF)
            endif()
            if (SUPPORT_ZLIB_VENDORED)
                add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
                set(PNG_BUILD_ZLIB ON)  # Used by vendored libpng
                set(ZLIB_INCLUDE_DIR "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/zlib;${CMAKE_CURRENT_BINARY_DIR}/external/zlib>")
                if (BUILD_SHARED_LIBS)
                    set(ZLIB_LIBRARY zlib)
                else()
                    set(ZLIB_LIBRARY zlibstatic)
                endif()
                list(APPEND INSTALL_EXTRA_TARGETS ${ZLIB_LIBRARY})
                if (NOT SUPPORT_PNG_SHARED)
                    list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${ZLIB_LIBRARY}>)
                endif()
            endif()
            set(SKIP_INSTALL_EXPORT ON)
            add_subdirectory(external/libpng EXCLUDE_FROM_ALL)
            set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})
            if (BUILD_SHARED_LIBS AND SUPPORT_PNG_SHARED)
                set(PNG_LIBRARY png)
            else()
                set(PNG_LIBRARY png_static)
            endif()
            add_library(PNG::PNG ALIAS ${PNG_LIBRARY})
            target_include_directories(SDL2_image PRIVATE external/libpng)
            list(APPEND INSTALL_EXTRA_TARGETS ${PNG_LIBRARY})
            if (NOT SUPPORT_PNG_SHARED)
                list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:${PNG_LIBRARY}>)
            endif()
        else()
            find_package(PNG REQUIRED)
            list(APPEND PC_REQUIRES libpng)
        endif()
        if (SUPPORT_PNG_SHARED)
            target_include_directories(SDL2_image PRIVATE
                $<TARGET_PROPERTY:PNG::PNG,INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:PNG::PNG,INTERFACE_INCLUDE_DIRECTORIES>
                $<TARGET_PROPERTY:PNG::PNG,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
                )
            target_get_dynamic_library(dynamic_png PNG::PNG)
            message(STATUS "Dynamic libpng: ${dynamic_png}")
            target_compile_definitions(SDL2_image PRIVATE "LOAD_PNG_DYNAMIC=\"${dynamic_png}\"")
            if (SUPPORT_PNG_VENDORED)
                add_dependencies(SDL2_image PNG::PNG)
            endif()
        else()
            target_link_libraries(SDL2_image PRIVATE PNG::PNG)
        endif()
    endif()
endif()

if (SUPPORT_PNM)
    target_compile_definitions(SDL2_image PRIVATE LOAD_PNM)
endif()

if (SUPPORT_QOI)
    target_compile_definitions(SDL2_image PRIVATE LOAD_QOI)
endif()

if (SUPPORT_SVG)
    target_compile_definitions(SDL2_image PRIVATE LOAD_SVG)
endif()

if (SUPPORT_TGA)
    target_compile_definitions(SDL2_image PRIVATE LOAD_TGA)
endif()

if (SUPPORT_TIF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_TIF)
    # FIXME: libtiff might need zlib
    if (SUPPORT_TIF_VENDORED)
        if(NOT SUPPORT_TIF_SHARED)
            set(BUILD_SHARED_LIBS OFF)
        endif()
        add_subdirectory(external/libtiff EXCLUDE_FROM_ALL)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})
        add_library(TIFF::TIFF ALIAS tiff)
        list(APPEND INSTALL_EXTRA_TARGETS tiff)
        if (NOT SUPPORT_TIF_SHARED)
            list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:tiff>)
        endif()
    else()
        find_package(TIFF REQUIRED)
        list(APPEND PC_REQUIRES libtiff-4)
    endif()
    if (SUPPORT_TIF_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:TIFF::TIFF,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
        target_get_dynamic_library(dynamic_tif TIFF::TIFF)
        message(STATUS "Dynamic libtiff: ${dynamic_tif}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_TIF_DYNAMIC=\"${dynamic_tif}\"")
        if (SUPPORT_TIF_VENDORED)
            add_dependencies(SDL2_image TIFF::TIFF)
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE TIFF::TIFF)
    endif()
endif()

if (SUPPORT_WEBP)
    target_compile_definitions(SDL2_image PRIVATE LOAD_WEBP)
    # missing cpufeatures
    if (SUPPORT_WEBP_VENDORED)
        if(NOT SUPPORT_WEBP_SHARED)
            set(BUILD_SHARED_LIBS OFF)
        endif()
        add_subdirectory(external/libwebp EXCLUDE_FROM_ALL)
        set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})
        target_include_directories(SDL2_image PRIVATE external/libwebp/src)
        add_library(WebP::webp ALIAS webp)
        list(APPEND INSTALL_EXTRA_TARGETS webp)
    else()
        find_library(WEBP_LIBRARY NAMES webp)
        if (NOT WEBP_LIBRARY)
            message(FATAL_ERROR "Could not find webp library")
        endif()
        find_path(WEBP_INCLUDE NAMES webp/decode.h)
        if (NOT WEBP_INCLUDE)
            message(FATAL_ERROR "Could not find webp include directory")
        endif()
        add_library(webp UNKNOWN IMPORTED)
        set_target_properties(webp PROPERTIES
            IMPORTED_LOCATION "${WEBP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${WEBP_INCLUDE}"
            )
        list(APPEND PC_REQUIRES libwebp)
        add_library(WebP::webp ALIAS webp)
    endif()
    if (SUPPORT_WEBP_SHARED)
        target_include_directories(SDL2_image PRIVATE
            $<TARGET_PROPERTY:WebP::webp,INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webp,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:WebP::webp,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
            )
        target_get_dynamic_library(dynamic_webp WebP::webp)
        message(STATUS "Dynamic libwebp: ${dynamic_webp}")
        target_compile_definitions(SDL2_image PRIVATE "LOAD_WEBP_DYNAMIC=\"${dynamic_webp}\"")
        if (SUPPORT_WEBP_VENDORED)
            add_dependencies(SDL2_image WebP::webp)
        endif()
    else()
        target_link_libraries(SDL2_image PRIVATE WebP::webp)
    endif()
endif()

if (SUPPORT_XCF)
    target_compile_definitions(SDL2_image PRIVATE LOAD_XCF)
endif()

if (SUPPORT_XPM)
    target_compile_definitions(SDL2_image PRIVATE LOAD_XPM)
endif()

if (SUPPORT_XV)
    target_compile_definitions(SDL2_image PRIVATE LOAD_XV)
endif()

# Restore saved BUILD_SHARED_LIBS variable to build our library
set(BUILD_SHARED_LIBS ${SDL2IMAGE_BUILD_SHARED_LIBS})

target_include_directories(SDL2_image
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>
    )

set_target_properties(SDL2_image PROPERTIES
    DEFINE_SYMBOL DLL_EXPORT
    PUBLIC_HEADER SDL_image.h
    EXPORT_NAME ${sdl2_image_export_name}
    C_VISIBILITY_PRESET "hidden"
    )
if (NOT ANDROID)
    set_target_properties(SDL2_image PROPERTIES
        DEBUG_POSTFIX "${SDL_CMAKE_DEBUG_POSTFIX}")
endif()
if (UNIX AND NOT APPLE AND NOT ANDROID)
    set_target_properties(SDL2_image PROPERTIES
        SOVERSION "${LT_MAJOR}"
        VERSION "${LT_VERSION}"
        )
endif()
if (BUILD_SHARED_LIBS)
    if (WIN32 OR OS2)
        set_target_properties(SDL2_image PROPERTIES
            PREFIX ""
            )
    endif()
    if (OS2)
        # OS/2 doesn't support a DLL name longer than 8 characters.
        set_target_properties(SDL2_image PROPERTIES
            OUTPUT_NAME "SDL2img"
            )
    elseif (UNIX AND NOT APPLE AND NOT ANDROID)
        set_target_properties(SDL2_image PROPERTIES
            OUTPUT_NAME "SDL2_image-${LT_RELEASE}"
            )
    endif()
endif()

if (NOT SDL2_IMAGE_DISABLE_INSTALL)
    if (BUILD_SHARED_LIBS OR NOT TARGET SDL2::SDL2-static)
        target_link_libraries(SDL2_image PRIVATE SDL2::SDL2)
    else()
        target_link_libraries(SDL2_image PRIVATE SDL2::SDL2-static)
    endif()

    install(TARGETS SDL2_image EXPORT SDL2ImageExports
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL2"
        )

    if (INSTALL_EXTRA_TARGETS)
        install(TARGETS ${INSTALL_EXTRA_TARGETS} EXPORT SDL2ImageExports
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/external_include"
            )
    endif()

    configure_package_config_file(SDL2_imageConfig.cmake.in SDL2_imageConfig.cmake
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_image")

    set(prefix "${CMAKE_INSTALL_PREFIX}")
    set(exec_prefix "\${prefix}")
    set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    set(PACKAGE "${PROJECT_NAME}")
    set(VERSION "${FULL_VERSION}")
    set(SDL_VERSION "${SDL_REQUIRED_VERSION}")
    string(JOIN " " PC_REQUIRES ${PC_REQUIRES})
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SDL2_image.pc" INPUT "${PROJECT_SOURCE_DIR}/SDL2_image.pc.in")

    install(EXPORT SDL2ImageExports NAMESPACE SDL2_image::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_image" FILE SDL2_image-${sdl2_image_install_name_infix}-targets.cmake)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SDL2_imageConfig.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_image")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SDL2_image.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    install(FILES "LICENSE.txt" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}")
    if (NOT (WIN32 OR CYGWIN OR MINGW))
        if(BUILD_SHARED_LIBS)
            set(SOEXT ${CMAKE_SHARED_LIBRARY_SUFFIX}) # ".so", ".dylib", etc.
            get_target_property(SONAME SDL2_image OUTPUT_NAME)
            if(NOT ANDROID)
                install(CODE "
                    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                        \"lib${SONAME}${SOEXT}\" \"libSDL2_image${SOEXT}\"
                        WORKING_DIRECTORY \"${PROJECT_BINARY_DIR}\")")
                install(FILES ${PROJECT_BINARY_DIR}/libSDL2_image${SOEXT} DESTINATION "${CMAKE_INSTALL_LIBDIR}")
            endif()
        endif()
    endif()
endif()

if (BUILD_SAMPLES)
    add_executable(showanim showanim.c)
    add_executable(showimage showimage.c)

    foreach(prog showanim showimage)
        if (MINGW)
            target_link_libraries(${prog} PRIVATE mingw32)
            target_link_options(${prog} PRIVATE -mwindows)
        endif()
        target_link_libraries(${prog} PRIVATE SDL2_image::${sdl2_image_export_name})
        if (TARGET SDL2::SDL2main)
            target_link_libraries(${prog} PRIVATE SDL2::SDL2main)
        endif()
        if (BUILD_SHARED_LIBS OR NOT TARGET SDL2::SDL2-static)
            target_link_libraries(${prog} PRIVATE SDL2::SDL2)
        else()
            target_link_libraries(${prog} PRIVATE SDL2::SDL2-static)
        endif()
    endforeach()
endif()

if (BUILD_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()
