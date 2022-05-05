#!/bin/sh
# Copyright 2022 Collabora Ltd.
# SPDX-License-Identifier: Zlib

set -eu

ref_major=$(sed -ne 's/^#define SDL_IMAGE_MAJOR_VERSION  *//p' SDL_image.h)
ref_minor=$(sed -ne 's/^#define SDL_IMAGE_MINOR_VERSION  *//p' SDL_image.h)
ref_micro=$(sed -ne 's/^#define SDL_IMAGE_PATCHLEVEL  *//p' SDL_image.h)
ref_version="${ref_major}.${ref_minor}.${ref_micro}"

tests=0
failed=0

ok () {
    tests=$(( tests + 1 ))
    echo "ok - $*"
}

not_ok () {
    tests=$(( tests + 1 ))
    echo "not ok - $*"
    failed=1
}

major=$(sed -Ene 's/^m4_define\(\[MAJOR_VERSION_MACRO\], \[([0-9]+)\]\)$/\1/p' configure.ac)
minor=$(sed -Ene 's/^m4_define\(\[MINOR_VERSION_MACRO\], \[([0-9]+)\]\)$/\1/p' configure.ac)
micro=$(sed -Ene 's/^m4_define\(\[MICRO_VERSION_MACRO\], \[([0-9]+)\]\)$/\1/p' configure.ac)
version="${major}.${minor}.${micro}"

if [ "$ref_version" = "$version" ]; then
    ok "configure.ac $version"
else
    not_ok "configure.ac $version disagrees with SDL_image.h $ref_version"
fi

major=$(sed -ne 's/^set(MAJOR_VERSION \([0-9]\+\))$/\1/p' CMakeLists.txt)
minor=$(sed -ne 's/^set(MINOR_VERSION \([0-9]\+\))$/\1/p' CMakeLists.txt)
micro=$(sed -ne 's/^set(MICRO_VERSION \([0-9]\+\))$/\1/p' CMakeLists.txt)
version="${major}.${minor}.${micro}"

if [ "$ref_version" = "$version" ]; then
    ok "CMakeLists.txt $version"
else
    not_ok "CMakeLists.txt $version disagrees with SDL_image.h $ref_version"
fi

major=$(sed -ne 's/^MAJOR_VERSION *= *//p' Makefile.os2)
minor=$(sed -ne 's/^MINOR_VERSION *= *//p' Makefile.os2)
micro=$(sed -ne 's/^MICRO_VERSION *= *//p' Makefile.os2)
version="${major}.${minor}.${micro}"

if [ "$ref_version" = "$version" ]; then
    ok "Makefile.os2 $version"
else
    not_ok "Makefile.os2 $version disagrees with SDL_image.h $ref_version"
fi

tuple=$(sed -ne 's/^ *FILEVERSION *//p' VisualC/Version.rc | tr -d '\r')
ref_tuple="${ref_major},${ref_minor},${ref_micro},0"

if [ "$ref_tuple" = "$tuple" ]; then
    ok "Version.rc FILEVERSION $tuple"
else
    not_ok "Version.rc FILEVERSION $tuple disagrees with SDL_image.h $ref_tuple"
fi

tuple=$(sed -ne 's/^ *PRODUCTVERSION *//p' VisualC/Version.rc | tr -d '\r')

if [ "$ref_tuple" = "$tuple" ]; then
    ok "Version.rc PRODUCTVERSION $tuple"
else
    not_ok "Version.rc PRODUCTVERSION $tuple disagrees with SDL_image.h $ref_tuple"
fi

tuple=$(sed -Ene 's/^ *VALUE "FileVersion", "([0-9, ]+)\\0"\r?$/\1/p' VisualC/Version.rc | tr -d '\r')
ref_tuple="${ref_major}, ${ref_minor}, ${ref_micro}, 0"

if [ "$ref_tuple" = "$tuple" ]; then
    ok "Version.rc FileVersion $tuple"
else
    not_ok "Version.rc FileVersion $tuple disagrees with SDL_image.h $ref_tuple"
fi

tuple=$(sed -Ene 's/^ *VALUE "ProductVersion", "([0-9, ]+)\\0"\r?$/\1/p' VisualC/Version.rc | tr -d '\r')

if [ "$ref_tuple" = "$tuple" ]; then
    ok "Version.rc ProductVersion $tuple"
else
    not_ok "Version.rc ProductVersion $tuple disagrees with SDL_image.h $ref_tuple"
fi

version=$(sed -Ene '/CFBundleShortVersionString/,+1 s/.*<string>(.*)<\/string>.*/\1/p' Xcode/Info-Framework.plist)

if [ "$ref_version" = "$version" ]; then
    ok "Info-Framework.plist CFBundleShortVersionString $version"
else
    not_ok "Info-Framework.plist CFBundleShortVersionString $version disagrees with SDL_image.h $ref_version"
fi

version=$(sed -Ene '/CFBundleVersion/,+1 s/.*<string>(.*)<\/string>.*/\1/p' Xcode/Info-Framework.plist)

if [ "$ref_version" = "$version" ]; then
    ok "Info-Framework.plist CFBundleVersion $version"
else
    not_ok "Info-Framework.plist CFBundleVersion $version disagrees with SDL_image.h $ref_version"
fi

# For simplicity this assumes we'll never break ABI before SDL 3.
dylib_compat=$(sed -Ene 's/.*DYLIB_COMPATIBILITY_VERSION = (.*);$/\1/p' Xcode/SDL_image.xcodeproj/project.pbxproj)
ref='3.0.0
3.0.0'

if [ "$ref" = "$dylib_compat" ]; then
    ok "project.pbxproj DYLIB_COMPATIBILITY_VERSION is consistent"
else
    not_ok "project.pbxproj DYLIB_COMPATIBILITY_VERSION is inconsistent"
fi

dylib_cur=$(sed -Ene 's/.*DYLIB_CURRENT_VERSION = (.*);$/\1/p' Xcode/SDL_image.xcodeproj/project.pbxproj)

case "$ref_minor" in
    (*[02468])
        major="$(( ref_minor * 100 + 1 ))"
        minor="$ref_micro"
        ;;
    (*)
        major="$(( ref_minor * 100 + ref_micro + 1 ))"
        minor="0"
        ;;
esac

ref="${major}.${minor}.0
${major}.${minor}.0"

if [ "$ref" = "$dylib_cur" ]; then
    ok "project.pbxproj DYLIB_CURRENT_VERSION is consistent"
else
    not_ok "project.pbxproj DYLIB_CURRENT_VERSION is inconsistent"
fi

echo "1..$tests"
exit "$failed"
