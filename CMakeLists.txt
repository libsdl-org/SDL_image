cmake_minimum_required(VERSION 3.3.2)
project(SDL_image C)

# See docs/release_checklist.md
set(MAJOR_VERSION 2)
set(MINOR_VERSION 5)
set(MICRO_VERSION 0)
set(FULL_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}")

# FIXME: CMAKE SUPPORT IN SDL2_image IS VERY INCOMPLETE YET !!!
#
# FIXME: make it able build against system codec libraries, too.
# FIXME: handle library versioning.
# FIXME: test accross different target platforms.

if (NOT ANDROID AND NOT (TARGET SDL2 OR TARGET SDL2-static))
	find_package(SDL2 REQUIRED)
endif()

# Workaround for Ubuntu 20.04's SDL being older than
# https://github.com/libsdl-org/SDL/issues/3531
if (NOT TARGET SDL2::SDL2)
	add_library(SDL2::SDL2 SHARED IMPORTED)
	find_library(SDL2_LIBRARY
		NAMES SDL2
		HINTS "${SDL2_EXEC_PREFIX}"
		REQUIRED)
	set_target_properties(SDL2::SDL2 PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
		IMPORTED_LINK_INTERFACE_LANGUAGES "C"
		IMPORTED_LOCATION "${SDL2_LIBRARY}")
endif()

# Calculate a libtool-like version number
math(EXPR BINARY_AGE "${MINOR_VERSION} * 100 + ${MICRO_VERSION}")
if(MINOR_VERSION MATCHES "[02468]$")
    # Stable branch, 2.6.1 -> libSDL2_image-2.0.so.0.600.1
    set(INTERFACE_AGE ${MICRO_VERSION})
else()
    # Development branch, 2.5.1 -> libSDL2_image-2.0.so.0.501.0
    set(INTERFACE_AGE 0)
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

option(SUPPORT_JPG "Support loading JPEG images" ON)
option(SUPPORT_PNG "Support loading PNG images" ON)
option(SUPPORT_WEBP "Support loading WEBP images" OFF)
option(BUILD_SHOWIMAGE "Build the showimage sample program" OFF)
option(BUILD_SHARED_LIBS "Build the library as a shared library" ON)
if (APPLE)
	option(ENABLE_APPLE_IMAGEIO "Use native Mac OS X frameworks for loading images" ON)
endif()

# FIXME: ????
if (NOT BUILD_SHARED_LIBS)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

add_library(SDL2_image)

if (UNIX AND NOT APPLE AND NOT ANDROID)
	# This is compatible with the libtool build
	set_target_properties(SDL2_image PROPERTIES
		VERSION ${LT_VERSION}
		SOVERSION ${LT_MAJOR}
		OUTPUT_NAME "SDL2_image-${LT_RELEASE}"
	)
endif()

set(IMAGEIO_SOURCES)
if (APPLE)
	# TODO: Use DYLIB_COMPATIBILITY_VERSION, DYLIB_CURRENT_VERSION here
	if (ENABLE_APPLE_IMAGEIO)
		target_link_options(SDL2_image PRIVATE -Wl,-framework,ApplicationServices)
		target_link_libraries(SDL2_image PRIVATE objc)
		set(IMAGEIO_SOURCES "IMG_ImageIO.m")
	else()
		target_compile_definitions(SDL2_image PRIVATE SDL_IMAGE_USE_COMMON_BACKEND)
	endif()
endif()

target_sources(SDL2_image PRIVATE IMG.c IMG_png.c IMG_bmp.c IMG_gif.c
		IMG_jpg.c IMG_lbm.c IMG_pcx.c IMG_pnm.c IMG_svg.c IMG_tga.c
		IMG_tif.c IMG_webp.c IMG_WIC.c IMG_xcf.c IMG_xpm.c IMG_xv.c
		IMG_qoi.c IMG_xxx.c ${IMAGEIO_SOURCES})

target_compile_definitions(SDL2_image PRIVATE
		-DLOAD_BMP -DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM
		-DLOAD_TGA -DLOAD_XCF -DLOAD_XPM -DLOAD_XV -DLOAD_XPM
		-DLOAD_QOI)

if (SUPPORT_JPG)
	target_compile_definitions(SDL2_image PRIVATE -DLOAD_JPG)
	add_subdirectory(external/jpeg-9e)
	target_link_libraries(SDL2_image PRIVATE jpeg)
endif()

if (SUPPORT_PNG)
	# missing libpng.vers
	set(HAVE_LD_VERSION_SCRIPT OFF CACHE BOOL "" FORCE)
	target_compile_definitions(SDL2_image PRIVATE -DLOAD_PNG)

	if (NOT TARGET zlib)
		# SDL_image doesn't support installing currently
		set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)

		# if zlib not included from another source, add_subdirectory
		add_subdirectory(external/zlib-1.2.12)

		# libpng find_package(zlib) requires ZLIB_INCLUDE_DIR set
		get_target_property(ZLIB_INCLUDE_DIR zlib INCLUDE_DIRECTORIES)

		# libpng find_package(zlib) requires ZLIB_LIBRARY
		if (BUILD_SHARED_LIBS)
			set(ZLIB_LIBRARY zlib)
		else()
			set(ZLIB_LIBRARY zlibstatic)
		endif()
	endif()

	add_subdirectory(external/libpng-1.6.37)
	include_directories(external/libpng-1.6.37)
	if(BUILD_SHARED_LIBS)
		target_link_libraries(SDL2_image PRIVATE png)
	else()
		target_link_libraries(SDL2_image PRIVATE png_static)
	endif()
endif()

if (SUPPORT_WEBP)
	target_compile_definitions(SDL2_image PRIVATE -DLOAD_WEBP)
	# missing cpufeatures
	add_subdirectory(external/libwebp-1.0.3)
	include_directories(external/libwebp-1.0.3/src)
	target_link_libraries(SDL2_image PRIVATE webp)
endif()

add_library(SDL2::image ALIAS SDL2_image)

target_include_directories(SDL2_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_SHARED_LIBS)
	if(WIN32)
	  target_compile_definitions(SDL2_image PRIVATE -DDLL_EXPORT)
	  target_sources(SDL2_image PRIVATE version.rc)
	endif()
	target_link_libraries(SDL2_image PUBLIC SDL2::SDL2)
else()
	target_link_libraries(SDL2_image PUBLIC SDL2::SDL2-static)
endif()

if(BUILD_SHOWIMAGE)
	add_executable(showimage showimage.c)
	target_link_libraries(showimage PRIVATE SDL2::image)
	if (WIN32)
		target_link_libraries(showimage PRIVATE SDL2::SDL2main)
	endif()
endif()
