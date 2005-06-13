#!/bin/sh

DIRS=". resiprocate resiprocate/test resiprocate/dum resiprocate/dum/test resiprocate/dum/doc"
T=$(mktemp)

[ -z "${T}"] || [ ! -f "${T}" ] || T=/tmp/$$.uti

    
for D in ${DIRS}; do
    cp /dev/null "${T}"
    for I in .cvsignore .autotools-conflicts-list; do
        echo ${I} >> "${T}"
        [ -f "${D}/${I}" ] && cat ${D}/${I} >> "${T}"
    done
    echo "+++ ${D} ignore list:"
    awk '{print "\t"$0;}' "${T}"
    svn propset svn:ignore -F "${T}" "${D}"
done


rm "${T}"
