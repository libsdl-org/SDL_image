dnl Process this file with autoconf to produce a configure script.

dnl Set various version strings - taken gratefully from the GTk sources
# Making releases:
#   MICRO_VERSION_MACRO += 1;
#   INTERFACE_AGE_MACRO += 1;
#   BINARY_AGE_MACRO += 1;
# if any functions have been added, set INTERFACE_AGE_MACRO to 0.
# if backwards compatibility has been broken,
# set BINARY_AGE_MACRO and INTERFACE_AGE_MACRO to 0.
m4_define([MAJOR_VERSION_MACRO], [2])
m4_define([MINOR_VERSION_MACRO], [0])
m4_define([MICRO_VERSION_MACRO], [6])
m4_define([INTERFACE_AGE_MACRO], [0])
m4_define([BINARY_AGE_MACRO], [6])

AC_INIT([SDL2_image],
        MAJOR_VERSION_MACRO.MINOR_VERSION_MACRO.MICRO_VERSION_MACRO,
        [https://github.com/libsdl-org/SDL_image/issues],
        [SDL2_image])
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_SRCDIR([IMG.c])

AC_SUBST([MAJOR_VERSION], MAJOR_VERSION_MACRO)
AC_SUBST([MINOR_VERSION], MINOR_VERSION_MACRO)
AC_SUBST([MICRO_VERSION], MICRO_VERSION_MACRO)
AC_SUBST([INTERFACE_AGE], INTERFACE_AGE_MACRO)
AC_SUBST([BINARY_AGE], BINARY_AGE_MACRO)

dnl libtool versioning
LT_INIT([win32-dll])

LT_RELEASE=$MAJOR_VERSION.$MINOR_VERSION
LT_CURRENT=`expr $MICRO_VERSION - $INTERFACE_AGE`
LT_REVISION=$INTERFACE_AGE
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`
LT_EXTRA="" dnl for OS2 dll name

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_EXTRA)

dnl Detect the canonical build and host environments
AC_CANONICAL_HOST

dnl Setup for automake
AM_INIT_AUTOMAKE([foreign tar-ustar])

dnl Check for tools
AC_PROG_CC
AC_PROG_OBJC
AC_C_INLINE
AC_PROG_INSTALL
AC_PROG_MAKE_SET
if test -z "$host_alias"; then
    hostaliaswindres=
else
    hostaliaswindres="$host_alias-windres"
fi
AC_CHECK_PROGS(WINDRES, [windres $hostaliaswindres $host_os-windres])

case "$host" in
    *-*-beos*)
        ac_default_prefix=/boot/develop/tools/gnupro
        ;;
    *-*-cygwin* | *-*-mingw*)
        if test "$build" != "$host"; then # cross-compiling
            # Default cross-compile location
            ac_default_prefix=/usr/local/cross-tools/$host
        else
            # Look for the location of the tools and install there
            if test "$BUILD_PREFIX" != ""; then
                ac_default_prefix=$BUILD_PREFIX
            fi
        fi
        if test x$WINDRES != x; then
            use_version_rc=true
        fi
        ;;
    *-*-darwin*)
        AC_ARG_ENABLE([imageio], [AS_HELP_STRING([--enable-imageio], [use native Mac OS X frameworks for loading images [default=yes]])],
                      [], [enable_imageio=yes])
        dnl Show a message when we use ImageIO support so it's not a surprise
        AC_MSG_CHECKING([for ImageIO support])
        AC_MSG_RESULT($enable_imageio)
        if test x$enable_imageio = xyes; then
            IMG_LIBS="-Wl,-framework,ApplicationServices -lobjc $IMG_LIBS"
        else
            CFLAGS="$CFLAGS -DSDL_IMAGE_USE_COMMON_BACKEND"
        fi
        ;;
    *-*-os2*)
        # disable static builds on os/2
        enable_static=no
        # -DBUILD_SDL is needed for DECLSPEC
        CFLAGS="$CFLAGS -DBUILD_SDL"
        # OS/2 does not support a DLL name longer than 8 characters.
        LT_EXTRA="-os2dllname SDL2img"
        ;;
esac
AM_CONDITIONAL(USE_IMAGEIO, test x$enable_imageio = xyes)
AM_CONDITIONAL(USE_VERSION_RC, test x$use_version_rc = xtrue)

dnl set this to use on systems that use lib64 instead of lib
base_libdir=`echo \${libdir} | sed 's/.*\/\(.*\)/\1/; q'`

dnl Function to find a library in the compiler search path
find_lib()
{
    gcc_bin_path=[`$CC -print-search-dirs 2>/dev/null | fgrep programs: | sed 's/[^=]*=\(.*\)/\1/' | sed 's/:/ /g'`]
    gcc_lib_path=[`$CC -print-search-dirs 2>/dev/null | fgrep libraries: | sed 's/[^=]*=\(.*\)/\1/' | sed 's/:/ /g'`]
    env_lib_path=[`echo $LIBS $LDFLAGS $* | sed 's/-L[ ]*//g'`]
    if test "$cross_compiling" = yes; then
        host_lib_path=""
    else
        host_lib_path="/usr/$base_libdir /usr/local/$base_libdir"
    fi
    for path in $env_lib_path $gcc_bin_path $gcc_lib_path $host_lib_path; do
        lib=[`ls -- $path/$1 2>/dev/null | sed -e '/\.so\..*\./d' -e 's,.*/,,' | sort | tail -1`]
        if test x$lib != x; then
            echo $lib
            return
        fi
    done
}

dnl See if GCC's -fvisibility=hidden is supported (gcc4 and later, usually).
CheckVisibilityHidden()
{
    AC_MSG_CHECKING(for GCC -fvisibility=hidden option)
    have_gcc_fvisibility=no
    case "$host" in
    *-*-cygwin* | *-*-mingw* | *-*-os2*)
        AC_MSG_RESULT([ignored for $host_os])
        return
        ;;
    esac

    visibility_CFLAGS="-fvisibility=hidden"
    save_CFLAGS="$CFLAGS"
    CFLAGS="$save_CFLAGS $visibility_CFLAGS -Werror"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #if !defined(__GNUC__) || __GNUC__ < 4
    #error SDL only uses visibility attributes in GCC 4 or newer
    #endif
    ]],[])], [have_gcc_fvisibility=yes],[])
    AC_MSG_RESULT($have_gcc_fvisibility)
    CFLAGS="$save_CFLAGS"

    if test x$have_gcc_fvisibility = xyes; then
        CFLAGS="$CFLAGS $visibility_CFLAGS"
    fi
}

dnl Check command-line options
AC_ARG_ENABLE([bmp], [AS_HELP_STRING([--enable-bmp], [support loading BMP/ICO/CUR images [default=yes]])],
 [], [enable_bmp=yes])
AC_ARG_ENABLE([gif], [AS_HELP_STRING([--enable-gif], [support loading GIF images [default=yes]])],
 [], [enable_gif=yes])
AC_ARG_ENABLE([jpg], [AS_HELP_STRING([--enable-jpg], [support loading JPG images [default=yes]])],
 [], [enable_jpg=yes])
AC_ARG_ENABLE([jpg-shared], [AS_HELP_STRING([--enable-jpg-shared], [dynamically load JPG support [default=yes]])],
 [], [enable_jpg_shared=yes])
AC_ARG_ENABLE([lbm], [AS_HELP_STRING([--enable-lbm], [support loading LBM images [default=yes]])],
 [], [enable_lbm=yes])
AC_ARG_ENABLE([pcx], [AS_HELP_STRING([--enable-pcx], [support loading PCX images [default=yes]])],
 [], [enable_pcx=yes])
AC_ARG_ENABLE([png], [AS_HELP_STRING([--enable-png], [support loading PNG images [default=yes]])],
 [], [enable_png=yes])
AC_ARG_ENABLE([png-shared], [AS_HELP_STRING([--enable-png-shared], [dynamically load PNG support [default=yes]])],
 [], [enable_png_shared=yes])
AC_ARG_ENABLE([pnm], [AS_HELP_STRING([--enable-pnm], [support loading PNM images [default=yes]])],
 [], [enable_pnm=yes])
AC_ARG_ENABLE([svg], [AS_HELP_STRING([--enable-svg], [support loading SVG images [default=yes]])],
 [], [enable_svg=yes])
AC_ARG_ENABLE([tga], [AS_HELP_STRING([--enable-tga], [support loading TGA images [default=yes]])],
 [], [enable_tga=yes])
AC_ARG_ENABLE([tif], [AS_HELP_STRING([--enable-tif], [support loading TIFF images [default=yes]])],
 [], [enable_tif=yes])
AC_ARG_ENABLE([tif-shared], [AS_HELP_STRING([--enable-tif-shared], [dynamically load TIFF support [default=yes]])],
 [], [enable_tif_shared=yes])
AC_ARG_ENABLE([xcf], [AS_HELP_STRING([--enable-xcf], [support loading XCF images [default=yes]])],
 [], [enable_xcf=yes])
AC_ARG_ENABLE([xpm], [AS_HELP_STRING([--enable-xpm], [support loading XPM images [default=yes]])],
 [], [enable_xpm=yes])
AC_ARG_ENABLE([xv], [AS_HELP_STRING([--enable-xv], [support loading XV images [default=yes]])],
 [], [enable_xv=yes])
AC_ARG_ENABLE([webp], [AS_HELP_STRING([--enable-webp], [support loading WEBP images [default=yes]])],
 [], [enable_webp=yes])
AC_ARG_ENABLE([webpdec], [AS_HELP_STRING([--enable-webpdec], [support loading WEBP images via libwebpdecoder instead of libwebp [default=no]])],
 [], [enable_webpdec=no])
AC_ARG_ENABLE([webp-shared], [AS_HELP_STRING([--enable-webp-shared], [dynamically load WEBP support [default=yes]])],
 [], [enable_webp_shared=yes])
AC_ARG_ENABLE([qoi], [AS_HELP_STRING([--enable-qoi], [support loading QOI images [default=yes]])],
 [], [enable_qoi=yes])

dnl Check for SDL
SDL_VERSION=2.0.8
AC_SUBST(SDL_VERSION)
AM_PATH_SDL2($SDL_VERSION,
            :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

if (test x$enable_jpg = xyes || test x$enable_tif = xyes) && test x$enable_imageio != xyes; then
    AC_CHECK_HEADER([jpeglib.h], [have_jpg_hdr=yes])
    AC_CHECK_LIB([jpeg], [jpeg_CreateDecompress], [have_jpg_lib=yes])
    if test x$have_jpg_hdr = xyes -a x$have_jpg_lib = xyes; then
        if test x$enable_jpg = xyes; then
            AC_DEFINE([LOAD_JPG])
        fi

        case "$host" in
            *-*-darwin*)
                jpg_lib=[`find_lib libjpeg.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                jpg_lib=[`find_lib "libjpeg*.dll"`]
                ;;
            *)
                jpg_lib=[`find_lib "libjpeg[0-9]*.so.*"`]
                if test x$jpg_lib = x; then
                    jpg_lib=[`find_lib "libjpeg.so.*"`]
                fi
                ;;
        esac
    elif test x$enable_jpg = xyes; then
        AC_MSG_WARN([*** Unable to find JPEG library (http://www.ijg.org/)])
        AC_MSG_WARN([JPG image loading disabled])
    fi
fi

if test x$enable_png = xyes -a x$enable_imageio != xyes; then
    PKG_CHECK_MODULES([LIBPNG], [libpng], [dnl
        have_png_hdr=yes
        have_png_lib=yes
      ], [dnl
        AC_CHECK_HEADER([png.h], [
            have_png_hdr=yes
            LIBPNG_CFLAGS=""
        ])
        AC_CHECK_LIB([png], [png_create_read_struct], [
            have_png_lib=yes
            LIBPNG_LIBS="-lpng -lz"
        ], [], [-lz])
      ])
    if test x$have_png_hdr = xyes -a x$have_png_lib = xyes; then
        AC_DEFINE([LOAD_PNG])

        case "$host" in
            *-*-darwin*)
                png_lib=[`find_lib libpng.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                png_lib=[`find_lib "libpng*.dll"`]
                ;;
            *)
                png_lib=[`find_lib "libpng[0-9]*.so.*"`]
                if test x$png_lib = x; then
                    png_lib=[`find_lib "libpng.so.*"`]
                fi
                ;;
        esac
    else
        AC_MSG_WARN([*** Unable to find PNG library (http://www.libpng.org/pub/png/libpng.html)])
        AC_MSG_WARN([PNG image loading disabled])
    fi
fi

if test x$enable_tif = xyes -a x$enable_imageio != xyes; then
    AC_CHECK_HEADER([tiffio.h], [have_tif_hdr=yes])
    AC_CHECK_LIB([tiff], [TIFFClientOpen], [have_tif_lib=yes], [], [-lz])
    if test x$have_tif_hdr = xyes -a x$have_tif_lib = xyes; then
        AC_DEFINE([LOAD_TIF])

        case "$host" in
            *-*-darwin*)
                tif_lib=[`find_lib libtiff.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                tif_lib=[`find_lib "libtiff-*.dll"`]
                ;;
            *)
                tif_lib=[`find_lib "libtiff[0-9]*.so.*"`]
                if test x$tif_lib = x; then
                    tif_lib=[`find_lib "libtiff.so.*"`]
                fi
                ;;
        esac
    else
        AC_MSG_WARN([*** Unable to find Tiff library (http://www.remotesensing.org/libtiff/)])
        AC_MSG_WARN([TIF image loading disabled])
    fi
fi

if test x$enable_webpdec = xyes; then
  webplib=webpdecoder
else
  webplib=webp
fi
if test x$enable_webp = xyes; then
    PKG_CHECK_MODULES([LIBWEBP], [lib$webplib], [dnl
        have_webp_hdr=yes
        have_webp_lib=yes
      ], [dnl
        AC_CHECK_HEADER([webp/decode.h], [
            have_webp_hdr=yes
            LIBWEBP_CFLAGS=""
        ])
        AC_CHECK_LIB([$webplib], [WebPGetDecoderVersion], [
            have_webp_lib=yes
            LIBWEBP_LIBS="-l$webplib"
        ], [], [-lm])
      ])
    if test x$have_webp_hdr = xyes -a x$have_webp_lib = xyes; then
        AC_DEFINE([LOAD_WEBP])

        case "$host" in
            *-*-darwin*)
                webp_lib=[`find_lib lib$webplib.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                webp_lib=[`find_lib "lib$webplib-*.dll"`]
                ;;
            *)
                webp_lib=[`find_lib "lib$webplib[0-9]*.so.*"`]
                if test x$webp_lib = x; then
                    webp_lib=[`find_lib "lib$webplib.so.*"`]
                fi
                ;;
        esac
    else
        AC_MSG_WARN([*** Unable to find WEBP library (http://code.google.com/intl/en-US/speed/webp/index.html)])
        AC_MSG_WARN([WEBP Pimage loading disabled])
    fi
fi

if test x$enable_bmp = xyes; then
    AC_DEFINE([LOAD_BMP])
fi

if test x$enable_gif = xyes; then
    AC_DEFINE([LOAD_GIF])
fi

if test x$enable_lbm = xyes; then
    AC_DEFINE([LOAD_LBM])
fi

if test x$enable_pcx = xyes; then
    AC_DEFINE([LOAD_PCX])
fi

if test x$enable_pnm = xyes; then
    AC_DEFINE([LOAD_PNM])
fi

if test x$enable_svg = xyes; then
    AC_DEFINE([LOAD_SVG])
fi

if test x$enable_tga = xyes; then
    AC_DEFINE([LOAD_TGA])
fi

if test x$enable_xcf = xyes; then
    AC_DEFINE([LOAD_XCF])
fi

if test x$enable_xpm = xyes; then
    AC_DEFINE([LOAD_XPM])
fi

if test x$enable_xv = xyes; then
    AC_DEFINE([LOAD_XV])
fi

if test x$enable_qoi = xyes; then
    AC_DEFINE([LOAD_QOI])
fi

if test x$enable_webp = xyes -a x$have_webp_hdr = xyes -a x$have_webp_lib = xyes; then
    CFLAGS="$LIBWEBP_CFLAGS $CFLAGS"
    if test x$enable_webp_shared = xyes && test x$webp_lib != x; then
        echo "-- dynamic libwebp -> $webp_lib"
        AC_DEFINE_UNQUOTED(LOAD_WEBP_DYNAMIC, "$webp_lib")
    else
        IMG_LIBS="$LIBWEBP_LIBS $IMG_LIBS"
    fi
fi

if test x$enable_tif = xyes -a x$have_tif_hdr = xyes -a x$have_tif_lib = xyes; then
    if test x$enable_tif_shared = xyes && test x$tif_lib != x; then
        echo "-- dynamic libtiff -> $tif_lib"
        AC_DEFINE_UNQUOTED(LOAD_TIF_DYNAMIC, "$tif_lib")
    else
        if test x$have_libjpeg = xyes; then
            # Disable dynamic jpeg since we're linking it explicitly
            jpg_lib=''
        fi
        IMG_LIBS="-ltiff -lz $IMG_LIBS"
    fi
fi
if test x$enable_jpg = xyes -a x$have_jpg_hdr = xyes -a x$have_jpg_lib = xyes; then
    if test x$enable_jpg_shared = xyes && test x$jpg_lib != x; then
        echo "-- dynamic libjpeg -> $jpg_lib"
        AC_DEFINE_UNQUOTED(LOAD_JPG_DYNAMIC, "$jpg_lib")
    else
        IMG_LIBS="-ljpeg $IMG_LIBS"
    fi
fi
if test x$enable_png = xyes -a x$have_png_hdr = xyes -a x$have_png_lib = xyes; then
    CFLAGS="$LIBPNG_CFLAGS $CFLAGS"
    if test x$enable_png_shared = xyes && test x$png_lib != x; then
        echo "-- dynamic libpng -> $png_lib"
        AC_DEFINE_UNQUOTED(LOAD_PNG_DYNAMIC, "$png_lib")
    else
        IMG_LIBS="$LIBPNG_LIBS $IMG_LIBS"
    fi
fi

AC_SUBST([WINDRES])
AC_SUBST([IMG_LIBS])

dnl check for GCC visibility attributes
CheckVisibilityHidden

OBJCFLAGS=$CFLAGS

# Finally create all the generated files
AC_CONFIG_FILES([
Makefile
SDL2_image.spec
SDL2_image.pc
])
AC_OUTPUT
