include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_WEBP QUIET libwebp)
pkg_check_modules(PC_WEBPDEMUX QUIET libwebpdemux)
pkg_check_modules(PC_WEBPMUX QUIET libwebpmux)

find_library(webp_LIBRARY
    NAMES webp
    HINTS ${PC_WEBP_LIBDIR}
)

find_path(webp_INCLUDE_PATH
    NAMES webp/decode.h
    HINTS ${PC_WEBP_INCLUDEDIR}
)

if(PC_WEBP_FOUND)
    get_flags_from_pkg_config("${webp_LIBRARY}" "PC_WEBP" "_webp")
endif()

set(webp_COMPILE_OPTIONS "${_webp_compile_options}" CACHE STRING "Extra compile options of webp")

set(webp_LINK_LIBRARIES "${_webp_link_libraries}" CACHE STRING "Extra link libraries of webp")

set(webp_LINK_OPTIONS "${_webp_link_options}" CACHE STRING "Extra link options of webp")

set(webp_LINK_DIRECTORIES "${_webp_link_directories}" CACHE STRING "Extra link directories of webp")

find_library(webpdemux_LIBRARY
    NAMES webpdemux
    HINTS ${PC_WEBPDEMUX_LIBDIR}
)

find_path(webpdemux_INCLUDE_PATH
    NAMES webp/demux.h
    HINTS ${PC_WEBPDEMUX_INCLUDEDIR}
)

if(PC_WEBPDEMUX_FOUND)
  get_flags_from_pkg_config("${webpdemux_LIBRARY}" "PC_WEBPDEMUX" "_webpdemux")
endif()

set(webpdemux_COMPILE_OPTIONS "${_webpdemux_compile_options}" CACHE STRING "Extra compile options of webpdemux")

set(webpdemux_LINK_LIBRARIES "${_webpdemux_link_libraries}" CACHE STRING "Extra link libraries of webpdemux")

set(webpdemux_LINK_OPTIONS "${_webpdemux_link_options}" CACHE STRING "Extra link options of webpdemux")

set(webpdemux_LINK_DIRECTORIES "${_webpdemux_link_directories}" CACHE STRING "Extra link directories of webpdemux")

find_library(webpmux_LIBRARY
    NAMES webpmux
    HINTS ${PC_WEBPMUX_LIBDIR}
)

find_path(webpmux_INCLUDE_PATH
    NAMES webp/mux.h
    HINTS ${PC_WEBPMUX_INCLUDEDIR}
)

if(PC_WEBPMUX_FOUND)
  get_flags_from_pkg_config("${webpmux_LIBRARY}" "PC_WEBPMUX" "_webpmux")
endif()

set(webpmux_COMPILE_OPTIONS "${_webpmux_compile_options}" CACHE STRING "Extra compile options of webpmux")

set(webpmux_LINK_LIBRARIES "${_webpmux_link_libraries}" CACHE STRING "Extra link libraries of webpmux")

set(webpmux_LINK_OPTIONS "${_webpmux_link_options}" CACHE STRING "Extra link options of webpmux")

set(webpmux_LINK_DIRECTORIES "${_webpmux_link_directories}" CACHE STRING "Extra link directories of webpmux")

find_package_handle_standard_args(webp
    REQUIRED_VARS webp_LIBRARY webp_INCLUDE_PATH webpdemux_LIBRARY webpdemux_INCLUDE_PATH webpmux_LIBRARY webpmux_INCLUDE_PATH
)

if (webp_FOUND)
    if (NOT TARGET WebP::webp)
        add_library(WebP::webp UNKNOWN IMPORTED)
        set_target_properties(WebP::webp PROPERTIES
            IMPORTED_LOCATION "${webp_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${webp_INCLUDE_PATH}"
            INTERFACE_COMPILE_OPTIONS "${webp_COMPILE_FLAGS}"
            INTERFACE_LINK_LIBRARIES "${webp_LINK_LIBRARIES}"
            INTERFACE_LINK_OPTIONS "${webp_LINK_OPTIONS}"
            INTERFACE_LINK_DIRECTORIES "${webp_LINK_DIRECTORIES}"
        )
    endif()
    if (NOT TARGET WebP::webpdemux)
        add_library(WebP::webpdemux UNKNOWN IMPORTED)
        set_target_properties(WebP::webpdemux PROPERTIES
            IMPORTED_LOCATION "${webpdemux_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${webpdemux_INCLUDE_PATH}"
            INTERFACE_COMPILE_OPTIONS "${webpdemux_COMPILE_FLAGS}"
            INTERFACE_LINK_LIBRARIES "${webpdemux_LINK_LIBRARIES}"
            INTERFACE_LINK_OPTIONS "${webpdemux_LINK_OPTIONS}"
            INTERFACE_LINK_DIRECTORIES "${webpdemux_LINK_DIRECTORIES}"
        )
    endif()
    if (NOT TARGET WebP::libwebpmux)
        add_library(WebP::libwebpmux UNKNOWN IMPORTED)
        set_target_properties(WebP::libwebpmux PROPERTIES
            IMPORTED_LOCATION "${webpmux_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${webpmux_INCLUDE_PATH}"
            INTERFACE_COMPILE_OPTIONS "${webpmux_COMPILE_FLAGS}"
            INTERFACE_LINK_LIBRARIES "${webpmux_LINK_LIBRARIES}"
            INTERFACE_LINK_OPTIONS "${webpmux_LINK_OPTIONS}"
            INTERFACE_LINK_DIRECTORIES "${webpmux_LINK_DIRECTORIES}"
        )
    endif()
endif()
