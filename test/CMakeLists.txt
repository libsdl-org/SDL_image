# CMake script for building SDL_image tests

enable_testing()

set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

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

function(add_sdl_image_test_executable TARGET)
    add_executable(${TARGET} ${ARGN})
    target_compile_definitions(${TARGET}
        PRIVATE
            $<TARGET_PROPERTY:${sdl3_image_target_name},COMPILE_DEFINITIONS>
            "SDL_IMAGE_SAVE_JPG=$<BOOL:${SDL3IMAGE_JPG_SAVE}>"
            "SDL_IMAGE_SAVE_PNG=$<BOOL:${SDL3IMAGE_PNG_SAVE}>"
    )
    sdl_add_warning_options(${TARGET} WARNING_AS_ERROR ${SDL3IMAGE_WERROR})
    target_link_libraries(${TARGET} PRIVATE SDL3_image::SDL3_image SDL3::SDL3_test SDL3::SDL3)

    add_test(
        NAME ${TARGET}
        COMMAND ${TARGET}
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET}>"
    )

    set(TESTS_ENVIRONMENT
        "SDL_TEST_SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "SDL_TEST_BUILDDIR=$<TARGET_FILE_DIR:${TARGET}>"
        "SDL_VIDEO_DRIVER=dummy"
    )
    set_tests_properties(${TARGET}
        PROPERTIES
            ENVIRONMENT "${TESTS_ENVIRONMENT}"
            TIMEOUT 30
    )
    if(SDL3IMAGE_TESTS_INSTALL)
        set(installedtestsdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/installed-tests/${PROJECT_NAME}")
        configure_file(template.test.in "${TARGET}.test" @ONLY)
        install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.test"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/installed-tests/${PROJECT_NAME}"
        )
    endif()
    if(SDL3IMAGE_TESTS_INSTALL)
        install(
            TARGETS ${TARGET}
            DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/installed-tests/${PROJECT_NAME}"
        )
    endif()
endfunction()

add_sdl_image_test_executable(testimage main.c)

if(SDL3IMAGE_TESTS_INSTALL)
    install(
        FILES ${RESOURCE_FILES}
        DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/installed-tests/${PROJECT_NAME}"
    )
endif()
