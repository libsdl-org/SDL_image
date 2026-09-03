include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBJXL QUIET libjxl)

find_library(libjxl_LIBRARY
    NAMES jxl
)

find_path(libjxl_INCLUDE_PATH
    NAMES jxl/decode.h
)

if(PC_LIBJXL_FOUND)
  get_flags_from_pkg_config("${libjxl_LIBRARY}" "PC_LIBJXL" "_libjxl")
endif()

set(libjxl_COMPILE_OPTIONS "${_libjxl_compile_options}" CACHE STRING "Extra compile options of libjxl")

set(libjxl_LINK_LIBRARIES "${_libjxl_link_libraries}" CACHE STRING "Extra link libraries of libjxl")

set(libjxl_LINK_OPTIONS "${_libjxl_link_options}" CACHE STRING "Extra link options of libjxl")

set(libjxl_LINK_DIRECTORIES "${_libjxl_link_directories}" CACHE PATH "Extra link flags of libjxl")

find_package_handle_standard_args(libjxl
    REQUIRED_VARS libjxl_LIBRARY libjxl_INCLUDE_PATH
)

if (libjxl_FOUND)
    if (NOT TARGET libjxl::libjxl)
        add_library(libjxl::libjxl UNKNOWN IMPORTED)
        set_target_properties(libjxl::libjxl PROPERTIES
            IMPORTED_LOCATION "${libjxl_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${libjxl_INCLUDE_PATH}"
            INTERFACE_COMPILE_OPTIONS "${libjxl_COMPILE_OPTIONS}"
            INTERFACE_LINK_LIBRARIES "${libjxl_LINK_LIBRARIES}"
            INTERFACE_LINK_OPTIONS "${libjxl_LINK_OPTIONS}"
            INTERFACE_LINK_DIRECTORIES "${libjxl_LINK_DIRECTORIES}"
        )
    endif()
endif()
