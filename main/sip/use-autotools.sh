#!/bin/bash
# $Id:$

# We 'remove' these files in SVN when using autotools.
AUTOTOOLS_CONFLICTS="
        Makefile.in
        Makefile 
        configure 
        configure.in  
        config.sub 
        resiprocate/Makefile 
        resiprocate/Makefile.in 
        resiprocate/configure.in 
        resiprocate/config.hxx 
        missing 
        aclocal.m4
"

# These are files that we care about and that
# are part of the autotools generation stuff.
# These files are source files in autotools/* parallel tree.

# Need to add resiprocate/dum/Makefile.am resiprocate/dum/test/Makefile.am

AUTOTOOLS_USED="Makefile.am
                resiprocate/Makefile.am 
                resiprocate/test/Makefile.am 
                configure.ac 
                acinclude.m4
                autogen.sh
            "

AUTOTOOLS_ARTIFACTS="config.guess
                     config.sub
                     ltmain.sh
                     mkinstalldirs
                     acinclude.m4
                     "
                     
                     



if [ "$1"x != x ]; then
    case "${1}" in
    --show-c*|-c) echo "${AUTOTOOLS_CONFLICTS}" ;;
    --show-u*|-u) echo "${AUTOTOOLS_USED}" ;;
    *) ;;
    esac
    exit 0
fi


if [ ! -f use-autotools.sh ]; then
cat <<EOF
You must run this script from the repository's sip directory.
The directory that contains use-autotools.sh
EOF
exit 1
fi

    # Determine your repository URI.
    URI=$(svn info |  grep '^URL:' | sed 's,.*\(http.*://.*\),\1,g')
    URI=${URI//\/rep\/resiprocate\/*}/rep/resiprocate
    SCHEME=${URI//:*}
    if [ "${SCHEME}" != "https" ]; then
        cat <<EOF

WARNING: Your repository URI is based on a '${SCHEME}:' checkout.
WARNING: In order to check any changes back in, you need to be
WARNING: using https.  Use 'svn switch' to alter the URI for this
WARNING: working view before attempting any commits.


EOF
    fi

    cat <<EOF
This script will prep a 'main' reSIProcate checkout to
use the autotools (new) build mechanism.

After running this script, the working directory will
have files related to the build process moved to the
$AUTOTOOLS_BRANCH branch.

Also, files related to the OLD build mechanism will be 
removed.  They are only removed on this branch in SVN.
A vanilla 'main' checkout will still contain these files.

Enter 'yes' to continue.
EOF


read DUMMY
[ "${DUMMY}x" == "yesx" ] || exit -1

ATCF=.autotools-conflicts-list

# Reset the conflicts list
find . -name ${ATCF} -exec cp /dev/null {} \;

# Remove conflicting files.
for F in ${AUTOTOOLS_CONFLICTS} ${AUTOTOOLS_USED}; do
    for E in echo "" ; do
        ${E} rm ${F}
    done
    Fd=$(dirname ${F})
    Fb=$(basename ${F})
    echo ${Fb} >> ${Fd}/${ATCF}
done

for B in ${AUTOTOOLS_USED}; do
    F=autotools/sip/${B}
    if [ -e ${F} ]; then
        for E in echo "" ; do
            ${E} ln -s ${F} ${B}
        done
    else
        echo ${F}: file not found.
    fi
done

for F in ${ATCF} ${AUTOTOOLS_ARTIFACTS}; do
    echo ${F} >> ${ATCF}
done

for D in $( find . -name ${ATCF} -print ) ;do
    ( cd $(dirname $D) ; pwd ; \
        cat .cvsignore ${ATCF} > /tmp/$$.atcf ;\
        svn propset svn:ignore -F /tmp/$$.atcf . ;\
        rm /tmp/$$.atcf)
done


