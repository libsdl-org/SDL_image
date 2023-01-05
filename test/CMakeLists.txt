cmake_minimum_required(VERSION 3.0)
project(SDL3_image_tests)

enable_testing()

option(SDL3IMAGE_TESTS_LINK_SHARED "link tests to shared SDL library" ON)
option(SDL3IMAGE_TESTS_INSTALL "Install unit tests?" OFF)

if(SDL3IMAGE_TESTS_LINK_SHARED)
    set(sdl_name_component SDL3)
    set(sdl_target_name SDL3::SDL3)
    set(sdlimage_target_name SDL3_image::SDL3_image)
else()
    set(sdl_name_component SDL3-static)
    set(sdl_target_name SDL3::SDL3-static)
    set(sdlimage_target_name SDL3_image::SDL3_image-static)
endif()

if(NOT TARGET ${sdl_target_name})
    find_package(SDL3 3.0.0 REQUIRED)
    if(NOT TARGET ${sdl_target_name})
        message(FATAL_ERROR "find_package(SDL3) did not create ${sdl_target_name}.")
    endif()
endif()
if(NOT TARGET ${sdlimage_target_name})
    find_package(SDL3_image 3.0.0 REQUIRED)
    if(NOT TARGET ${sdlimage_target_name})
        message(FATAL_ERROR "find_package(SDL3_image) did not create ${sdlimage_target_name}.")
    endif()
endif()

add_executable(testimage main.c)

set(ALL_TESTS
    testimage
)
set(RESOURCE_FILES
    palette.bmp
    palette.gif
    sample.avif
    sample.bmp
    sample.cur
    sample.ico
    sample.jpg
    sample.jxl
    sample.pcx
    sample.png
    sample.pnm
    sample.qoi
    sample.tga
    sample.tif
    sample.webp
    sample.xcf
    sample.xpm
    svg-class.bmp
    svg-class.svg
    svg.bmp
    svg.svg
    svg64.bmp
)

set(TESTS_ENVIRONMENT
    "SDL_TEST_SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "SDL_TEST_BUILDDIR=${CMAKE_CURRENT_BINARY_DIR}"
    "SDL_VIDEO_DRIVER=dummy"
)

foreach(prog ${ALL_TESTS})
    target_compile_definitions(${prog} PRIVATE $<TARGET_PROPERTY:SDL3_image,COMPILE_DEFINITIONS>)
    target_link_libraries(${prog} PRIVATE ${sdlimage_target_name})
    target_link_libraries(${prog} PRIVATE SDL3::SDL3_test ${sdl_target_name})
    if(TARGET sdl3image_build_options)
        target_link_libraries(${prog} PRIVATE $<BUILD_INTERFACE:sdl3image_build_options>)
    endif()

    add_test(
        NAME ${prog}
        COMMAND ${prog}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(
        ${prog}
        PROPERTIES
        ENVIRONMENT "${TESTS_ENVIRONMENT}"
        TIMEOUT 30
    )
    if(SDL3IMAGE_TESTS_INSTALL)
        set(exe ${prog})
        set(installedtestsdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/installed-tests/${PROJECT_NAME}")
        configure_file(template.test.in "${exe}.test" @ONLY)
        install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/${exe}.test"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/installed-tests/${PROJECT_NAME}"
        )
    endif()
endforeach()

if(SDL3IMAGE_TESTS_INSTALL)
    install(
        TARGETS ${ALL_TESTS}
        DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/installed-tests/${PROJECT_NAME}"
    )
    install(
        FILES ${RESOURCE_FILES}
        DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/installed-tests/${PROJECT_NAME}"
    )
endif()
