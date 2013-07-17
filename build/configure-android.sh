#!/bin/sh

# run this script with any normal configure options
# for reSIProcate

# Where the NDK is unpacked
export ANDROID_NDK=${HOME}/android/android-ndk-r7

# it may be necessary to have a full copy of the Android source tree
# or just take a copy of linker.h
# not currently needed at all for reSIProcate build
export ANDROID_SRC=${HOME}/android/full-source
 
export CROSS_COMPILE=arm-linux-androideabi
export CROSS_VERSION=4.4.3
export TOOLCHAIN_ROOT=${ANDROID_NDK}/toolchains/arm-linux-androideabi-${CROSS_VERSION}/prebuilt/linux-x86

export ANDROID_STL=${ANDROID_NDK}/sources/cxx-stl

export ANDROID_VERSION=8
export ANDROID_VERSION_S=android-${ANDROID_VERSION}
 
export SYSROOT=${ANDROID_NDK}/platforms/${ANDROID_VERSION_S}/arch-arm
 
export TOOLCHAIN_PREFIX=${TOOLCHAIN_ROOT}/bin/${CROSS_COMPILE}
 
# Declare any binutils we may need (there may be others)
export CPP=${TOOLCHAIN_PREFIX}-cpp
export AR=${TOOLCHAIN_PREFIX}-ar
export AS=${TOOLCHAIN_PREFIX}-as
export NM=${TOOLCHAIN_PREFIX}-nm
export CC=${TOOLCHAIN_PREFIX}-gcc
export CXX=${TOOLCHAIN_PREFIX}-g++
export LD=${TOOLCHAIN_PREFIX}-ld
export RANLIB=${TOOLCHAIN_PREFIX}-ranlib

# installation prefix
export PREFIX=/
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
 
export CFLAGS="${CFLAGS} --sysroot=${SYSROOT} -I${SYSROOT}/usr/include -I${TOOLCHAIN_ROOT}/include -I${ANDROID_SRC}/bionic -I${ANDROID_STL}/gnu-libstdc++/include -I${ANDROID_STL}/gnu-libstdc++/libs/armeabi/include"
export CPPFLAGS="${CFLAGS}"
export LDFLAGS="${LDFLAGS} -L${SYSROOT}/usr/lib -L${TOOLCHAIN_ROOT}/lib -L${ANDROID_STL}/gnu-libstdc++/lib"
 
./configure --host=${CROSS_COMPILE} --with-sysroot=${SYSROOT} --prefix=${PREFIX} "$@"

