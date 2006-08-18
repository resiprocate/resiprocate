#!/bin/sh
intver() {
    local V=`$1 --version | head -n 1 | sed 's/^.* \([0-9][0-9.]*\)[^0-9.]*$/\1/g'`
    local i=`echo $V | cut -d. -f1`
    local t=`echo $V | cut -d. -f2`
    local h=`echo $V | cut -d. -f3`
    expr $i \* 10000 + 100 \* $t + 0$h
}

echo '+++ Looking for a version of aclocal >= 1.7.5'
irep=$(intver aclocal)
if [ $irep -lt 10705 ]; then
    echo --- aclocal not new enough: expecting 1.75 or greater.
    aclocal --version
    exit 1
fi
echo +++ Running aclocal && \
aclocal && \
echo +++ Running libtoolize && \
(glibtoolize --copy --force 2> /dev/null || \
 libtoolize --copy --force  ) && \
echo +++ Running autoheader && \
autoheader && \
echo +++ Running automake  && \
automake  --foreign --copy --add-missing && \
echo +++ Running autoconf  && \
autoconf  && \

[ -d /usr/local/bin ] && export PATH=/usr/local/bin:$PATH

echo "+++ Ready to manually run : ./configure && make"
