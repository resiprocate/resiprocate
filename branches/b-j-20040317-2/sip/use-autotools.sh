#!/bin/bash
# $Id: use-autotools.sh,v 1.8 2004/02/25 18:45:40 alan Exp $

AUTOTOOLS_BRANCH=b-autotools
# We 'remove' these files in CVS.

AUTOTOOLS_ARTIFACTS="
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
        build/
        build/Makefile.all 
        build/Makefile.buildpaths 
        build/Makefile.conf 
        build/Makefile.conf.in 
        build/Makefile.osarch 
        build/Makefile.pkg 
        build/Makefile.post 
        build/Makefile.pre 
        build/Makefile.tools 
        build/fakepre 
        build/hacksol 
        build/lndir.sh 
        build/mkbuildlinks 
        build/mkbuildpaths
"

# These are files that we care about and that
# are part of the autotools generation stuff.
# These files are source files.
# There will still be a bootstrap required -- I think.

AUTOTOOLS_USED="Makefile.am 
          Makefile.am  
         resiprocate/Makefile.am 
            resiprocate/test/Makefile.am 
            configure.ac 
            acinclude.m4
            autogen.sh
            "



if [ "$1"x == x ]; then

    cat <<EOF
This script will prep a 'HEAD' reSIProcate checkout to
use the autotools (new) build mechanism.
After running this script, the working directory will
have files related to the build process moved to the
$AUTOTOOLS_BRANCH branch with the 'sticky-tag' set to 
this same branch.

Also, files related to the OLD build mechanism will be 
removed.  They are only removed on this branch in CVS.
A vanilla HEAD checkout will still contain these files.

Enter 'yes' to continue.
EOF
    read DUMMY
    [ "${DUMMY}x" == "yesx" ] || exit -1

    cvs up -r ${AUTOTOOLS_BRANCH} ${AUTOTOOLS_USED} ${AUTOTOOLS_ARTIFACTS}
    # Remove the 'old' build directory -- only if it's got a CVS dir in it.
    [ -d build/CVS ] && rm -rf build
else
    case "${1}" in
    --show-d*|-d) echo "${AUTOTOOLS_ARTIFACTS}" ;;
    --show-a*|-u) echo "${AUTOTOOLS_USED}" ;;
    *) ;;
    esac
fi
