cmake_minimum_required(VERSION 3.0)
project(SDL_image C)


if (ANDROID)

    option(SUPPORT_JPG "Support loading JPEG images" OFF)
    option(SUPPORT_PNG "Support loading PNG images" ON)
    option(SUPPORT_WEBP "Support loading WEBP images" OFF)

    add_library(SDL_image SHARED)
    target_sources(SDL_image PRIVATE IMG.c IMG_png.c IMG_bmp.c IMG_gif.c
            IMG_jpg.c IMG_lbm.c IMG_pcx.c IMG_pnm.c IMG_svg.c IMG_tga.c
            IMG_tif.c IMG_webp.c IMG_WIC.c IMG_xcf.c IMG_xpm.c IMG_xv.c IMG_xxx.c)

    add_definitions(
            -DLOAD_BMP -DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM
            -DLOAD_TGA -DLOAD_XCF -DLOAD_XPM -DLOAD_XV -DLOAD_XPM)

    if (SUPPORT_JPG)
        add_definitions(-DLOAD_JPG)
        # missing CMakeFiles
        add_subdirectory(external/jpeg-9b)

        target_link_libraries(SDL_image PRIVATE jpeg)
    endif()

    if (SUPPORT_PNG)
        # missing libpng.vers
        set(HAVE_LD_VERSION_SCRIPT OFF CACHE BOOL "" FORCE)
        add_definitions(-DLOAD_PNG)

        add_subdirectory(external/libpng-1.6.37)
        include_directories(external/libpng-1.6.37)
        target_link_libraries(SDL_image PRIVATE png)
    endif()

    if (SUPPORT_WEBP)
        add_definitions(-DLOAD_WEBP)
        # missing cpufeatures
        add_subdirectory(external/libwebp-1.0.2)
        target_link_libraries(SDL_image PRIVATE webp)
    endif()

    target_link_libraries(SDL_image PRIVATE SDL2)
else()

endif()

