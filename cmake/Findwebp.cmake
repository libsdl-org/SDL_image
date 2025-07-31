include(FindPackageHandleStandardArgs)

find_library(webp_LIBRARY
    NAMES webp
)

find_path(webp_INCLUDE_PATH
    NAMES webp/decode.h
)

set(webp_COMPILE_OPTIONS "" CACHE STRING "Extra compile options of webp")

set(webp_LINK_LIBRARIES "" CACHE STRING "Extra link libraries of webp")

set(webp_LINK_FLAGS "" CACHE STRING "Extra link flags of webp")

find_library(webpdemux_LIBRARY
    NAMES webpdemux
)

find_path(webpdemux_INCLUDE_PATH
    NAMES webp/demux.h
)

set(webpdemux_COMPILE_OPTIONS "" CACHE STRING "Extra compile options of webpdemux")

set(webpdemux_LINK_LIBRARIES "" CACHE STRING "Extra link libraries of webpdemux")

set(webpdemux_LINK_FLAGS "" CACHE STRING "Extra link flags of webpdemux")

find_library(webpmux_LIBRARY
    NAMES webpmux
)

find_path(webpmux_INCLUDE_PATH
    NAMES webp/mux.h
)

set(webpmux_COMPILE_OPTIONS "" CACHE STRING "Extra compile options of webpmux")

set(webpmux_LINK_LIBRARIES "" CACHE STRING "Extra link libraries of webpmux")

set(webpmux_LINK_FLAGS "" CACHE STRING "Extra link flags of webpmux")

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
            INTERFACE_LINK_FLAGS "${webp_LINK_FLAGS}"
        )
    endif()
    if (NOT TARGET WebP::webpdemux)
        add_library(WebP::webpdemux UNKNOWN IMPORTED)
        set_target_properties(WebP::webpdemux PROPERTIES
            IMPORTED_LOCATION "${webpdemux_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${webpdemux_INCLUDE_PATH}"
            INTERFACE_COMPILE_OPTIONS "${webpdemux_COMPILE_FLAGS}"
            INTERFACE_LINK_LIBRARIES "${webpdemux_LINK_LIBRARIES}"
            INTERFACE_LINK_FLAGS "${webpdemux_LINK_FLAGS}"
        )
    endif()
    if (NOT TARGET WebP::libwebpmux)
        add_library(WebP::libwebpmux UNKNOWN IMPORTED)
        set_target_properties(WebP::libwebpmux PROPERTIES
            IMPORTED_LOCATION "${webpmux_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${webpmux_INCLUDE_PATH}"
            INTERFACE_COMPILE_OPTIONS "${webpmux_COMPILE_FLAGS}"
            INTERFACE_LINK_LIBRARIES "${webpmux_LINK_LIBRARIES}"
            INTERFACE_LINK_FLAGS "${webpmux_LINK_FLAGS}"
        )
    endif()
endif()
