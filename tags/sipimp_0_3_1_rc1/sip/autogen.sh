#!/bin/bash
intver() {
    local V=$( $1 --version | head -1 | sed 's/^.* \([0-9][0-9.]*\)[^0-9.]*$/\1/g' )
    local i=$( echo $V | cut -d. -f1)
    local t=$( echo $V | cut -d. -f2)
    local h=$( echo $V | cut -d. -f3)
    expr $i \* 10000 + 100 \* $t + $h
}

echo +++ Removing old build files.
rm -rf build configure.in  Makefile
echo +++ Looking for a version of aclocal >= 1.7.5
irep=$(intver aclocal)
if [ $irep -lt 10705 ]; then
    echo --- aclocal not new enough: expecting 1.75 or greater.
    aclocal --version
    exit 1
fi
echo +++ Running aclocal
aclocal
echo +++ Running autoheader
autoheader
echo +++ Running automake
automake
echo +++ Running autoconf
autoconf
echo +++ Configuring and building ares.
(cd contrib/ares ; ./configure && make) 
echo +++ There will be errors from this autoreconf
autoreconf
export PATH=/usr/local/bin:$PATH
mkdir build
echo +++ Manually run : ../configure -C --enable-scanner --enable-ipv6
echo +++ From the build subdirectory.
