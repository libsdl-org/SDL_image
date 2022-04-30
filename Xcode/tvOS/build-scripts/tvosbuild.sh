#!/bin/sh
#
# Build a fat binary for tvOS

TARGET=webp

# Number of CPUs (for make -j)
NCPU=`sysctl -n hw.ncpu`
if test x$NJOB = x; then
    NJOB=$NCPU
fi

SRC_DIR=$(cd `dirname $0`/..; pwd)
if [ "$PWD" = "$SRC_DIR" ]; then
    PREFIX=$SRC_DIR/tvos-build
    mkdir $PREFIX
else
    PREFIX=$PWD
fi

#BUILD_X86_64_TVOSSIM=YES
BUILD_TVOS_ARM64=YES

# 13.4.0 - Mavericks
# 14.0.0 - Yosemite
# 15.0.0 - El Capitan
DARWIN=darwin15.0.0

XCODEDIR=`xcode-select --print-path`
TVOS_SDK_VERSION=`xcrun --sdk appletvos --show-sdk-version`
MIN_SDK_VERSION=9.0

APPLETVOS_PLATFORM=`xcrun --sdk appletvos --show-sdk-platform-path`
APPLETVOS_SYSROOT=`xcrun --sdk appletvos --show-sdk-path`

APPLETVSIMULATOR_PLATFORM=`xcrun --sdk appletvsimulator --show-sdk-platform-path`
APPLETVSIMULATOR_SYSROOT=`xcrun --sdk appletvsimulator --show-sdk-path`

# Uncomment if you want to see more information about each invocation
# of clang as the builds proceed.
# CLANG_VERBOSE="--verbose"

CC=clang
CXX=clang

SILENCED_WARNINGS="-Wno-unused-local-typedef -Wno-unused-function"

CFLAGS="${CLANG_VERBOSE} ${SILENCED_WARNINGS} -DNDEBUG -g -O0 -pipe -fPIC -fobjc-arc"

echo "PREFIX ..................... ${PREFIX}"
echo "BUILD_MACOSX_X86_64 ........ ${BUILD_MACOSX_X86_64}"
echo "BUILD_X86_64_TVOSSIM ........ ${BUILD_X86_64_TVOSSIM}"
echo "BUILD_TVOS_ARM64 ............ ${BUILD_TVOS_ARM64}"
echo "DARWIN ..................... ${DARWIN}"
echo "XCODEDIR ................... ${XCODEDIR}"
echo "TVOS_SDK_VERSION ............ ${TVOS_SDK_VERSION}"
echo "MIN_SDK_VERSION ............ ${MIN_SDK_VERSION}"
echo "APPLETVOS_PLATFORM .......... ${APPLETVOS_PLATFORM}"
echo "APPLETVOS_SYSROOT ........... ${APPLETVOS_SYSROOT}"
echo "APPLETVSIMULATOR_PLATFORM ... ${APPLETVSIMULATOR_PLATFORM}"
echo "APPLETVSIMULATOR_SYSROOT .... ${APPLETVSIMULATOR_SYSROOT}"
echo "CC ......................... ${CC}"
echo "CFLAGS ..................... ${CFLAGS}"
echo "CXX ........................ ${CXX}"
echo "CXXFLAGS ................... ${CXXFLAGS}"
echo "LDFLAGS .................... ${LDFLAGS}"

###################################################################
# This section contains the build commands for each of the 
# architectures that will be included in the universal binaries.
###################################################################

echo "$(tput setaf 2)"
echo "#############################"
echo "# x86_64 for AppleTV Simulator"
echo "#############################"
echo "$(tput sgr0)"

if [ "${BUILD_X86_64_TVOSSIM}" == "YES" ]
then
    (
        cd ${PREFIX}
        make clean
        ../configure --build=x86_64-apple-${DARWIN} --host=x86_64-appletvos-${DARWIN} --prefix=${PREFIX}/platform/x86_64-sim "CC=${CC}" "CFLAGS=${CFLAGS} -mtvos-simulator-version-min=${MIN_SDK_VERSION} -fembed-bitcode -arch x86_64 -isysroot ${APPLETVSIMULATOR_SYSROOT}" "CXX=${CXX}" "CXXFLAGS=${CXXFLAGS} -mtvos-simulator-version-min=${MIN_SDK_VERSION} -fembed-bitcode -arch x86_64 -isysroot ${APPLETVSIMULATOR_SYSROOT}" LDFLAGS="-Wc,-fembed-bitcode -arch x86_64 -mtvos-simulator-version-min=${MIN_SDK_VERSION} ${LDFLAGS} -L${APPLETVSIMULATOR_SYSROOT}/usr/lib/ -L${APPLETVSIMULATOR_SYSROOT}/usr/lib/system" || exit 2
        make -j$NJOB || exit 3
        make install
    ) || exit $?
fi

echo "$(tput setaf 2)"
echo "##################"
echo "# arm64 for AppleTV"
echo "##################"
echo "$(tput sgr0)"

if [ "${BUILD_TVOS_ARM64}" == "YES" ]
then
    (
        cd ${PREFIX}
        make clean
        ../configure --build=x86_64-apple-${DARWIN} --host=arm-appletvos-${DARWIN} --prefix=${PREFIX}/platform/arm64-appletvos "CC=${CC}" "CFLAGS=${CFLAGS} -mappletvos-version-min=${MIN_SDK_VERSION} -fembed-bitcode -arch arm64 -isysroot ${APPLETVOS_SYSROOT}" "CXX=${CXX}" "CXXFLAGS=${CXXFLAGS} -mappletvos-version-min=${MIN_SDK_VERSION} -fembed-bitcode -arch arm64 -isysroot ${APPLETVOS_SYSROOT}" LDFLAGS="-Wc,-fembed-bitcode -arch arm64 -mappletvos-version-min=${MIN_SDK_VERSION} ${LDFLAGS}" || exit 2
        make -j$NJOB || exit 3
        make install
    ) || exit $?
fi

echo "$(tput setaf 2)"
echo "###################################################################"
echo "# Create Universal Libraries and Finalize the packaging"
echo "###################################################################"
echo "$(tput sgr0)"

(
    cd ${PREFIX}/platform
    mkdir -p universal
    #lipo x86_64-sim/lib/lib$TARGET.a arm64-appletvos/lib/lib$TARGET.a -create -output universal/lib$TARGET.a
    #lipo x86_64-sim/lib/lib$TARGET.dylib arm64-appletvos/lib/lib$TARGET.dylib -create -output universal/lib$TARGET.dylib
    cp arm64-appletvos/lib/lib$TARGET.a universal/lib$TARGET.a
    cp arm64-appletvos/lib/lib$TARGET.dylib universal/lib$TARGET.dylib
)

(
    cd ${PREFIX}
    mkdir -p lib
    cp -r platform/universal/* lib
    #rm -rf platform
    lipo -info lib/*
)

echo Done!
