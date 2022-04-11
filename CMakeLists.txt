cmake_minimum_required(VERSION 3.3.2)
project(SDL_image C)

# FIXME: CMAKE SUPPORT IN SDL2_image IS VERY INCOMPLETE YET !!!
#
# FIXME: make it able build against system codec libraries, too.
# FIXME: handle library versioning.
# FIXME: test accross different target platforms.

if (NOT ANDROID AND NOT (TARGET SDL2 OR TARGET SDL2-static))
	find_package(SDL2 REQUIRED)
endif()

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

set(IMAGEIO_SOURCES)
if (APPLE)
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

		# SDL_image doesn't support installing currently
		set(SKIP_INSTALL_ALL ON)
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

if(SDL2_FOUND)
	target_link_libraries(SDL2_image PUBLIC ${SDL2_LIBRARIES})
	target_include_directories(SDL2_image PUBLIC ${SDL2_INCLUDE_DIRS})
else(SDL2_FOUND)
	if (BUILD_SHARED_LIBS)
		if(WIN32)
		  target_compile_definitions(SDL2_image PRIVATE -DDLL_EXPORT)
      target_sources(SDL2_image PRIVATE version.rc)
		endif()
		target_link_libraries(SDL2_image PUBLIC SDL2::SDL2)
	else()
		target_link_libraries(SDL2_image PUBLIC SDL2::SDL2-static)
	endif()
endif(SDL2_FOUND)

if(BUILD_SHOWIMAGE)
	add_executable(showimage showimage.c)
	target_link_libraries(showimage PRIVATE SDL2::image)
	if (WIN32)
		target_link_libraries(showimage PRIVATE SDL2::SDL2main)
	endif()
endif()
