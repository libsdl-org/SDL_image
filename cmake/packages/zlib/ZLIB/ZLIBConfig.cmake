if(
  NOT TARGET zlib AND
  NOT TARGET ZLIB::ZLIB AND
  NOT TARGET zlibstatic AND
  NOT TARGET ZLIB::zlibstatic
)
  message(FATAL_ERROR "Vendored ZLIB target(s) missing")
endif()

set(ZLIB_FOUND TRUE)
