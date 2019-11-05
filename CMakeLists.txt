cmake_minimum_required(VERSION 3.3.2)
project(SDL_image C)

if (ANDROID)

else()
    find_package(SDL2 REQUIRED)
endif()

option(SUPPORT_JPG "Support loading JPEG images" ON)
option(SUPPORT_PNG "Support loading PNG images" ON)
option(SUPPORT_WEBP "Support loading WEBP images" OFF)
option(BUILD_SHOWIMAGE "Build the showimage sample program" OFF)
option(BUILD_SHARED_LIBS "Build the library as a shared library" ON)

add_library(SDL2_image SHARED)
target_sources(SDL2_image PRIVATE IMG.c IMG_png.c IMG_bmp.c IMG_gif.c
		IMG_jpg.c IMG_lbm.c IMG_pcx.c IMG_pnm.c IMG_svg.c IMG_tga.c
		IMG_tif.c IMG_webp.c IMG_WIC.c IMG_xcf.c IMG_xpm.c IMG_xv.c IMG_xxx.c)

target_compile_definitions(SDL2_image PRIVATE
		-DLOAD_BMP -DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM
		-DLOAD_TGA -DLOAD_XCF -DLOAD_XPM -DLOAD_XV -DLOAD_XPM)

if (SUPPORT_JPG)
	target_compile_definitions(SDL2_image PRIVATE -DLOAD_JPG)
	add_subdirectory(external/jpeg-9b)
	target_link_libraries(SDL2_image PRIVATE jpeg)
endif()

if (SUPPORT_PNG)
	# missing libpng.vers
	set(HAVE_LD_VERSION_SCRIPT OFF CACHE BOOL "" FORCE)
	target_compile_definitions(SDL2_image PRIVATE -DLOAD_PNG)

	add_subdirectory(external/libpng-1.6.37)
	include_directories(external/libpng-1.6.37)
	target_link_libraries(SDL2_image PRIVATE png)
endif()

if (SUPPORT_WEBP)
	target_compile_definitions(SDL2_image PRIVATE -DLOAD_WEBP)
	# missing cpufeatures
	add_subdirectory(external/libwebp-1.0.3)
	target_link_libraries(SDL2_image PRIVATE webp)
endif()

add_library(SDL2::image ALIAS SDL2_image)

target_include_directories(SDL2_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (ANDROID)
    target_link_libraries(SDL2_image PRIVATE SDL2)
else()
    target_link_libraries(SDL2_image PRIVATE SDL2::SDL2)
endif()

if(BUILD_SHOWIMAGE)
	add_executable(showimage showimage.c)
	target_link_libraries(showimage PRIVATE SDL2::SDL2 SDL2::image)
endif()
