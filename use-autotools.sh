#!/bin/bash
# $Id: use-autotools.sh,v 1.1 2004/02/24 00:46:31 alan Exp $

AUTOTOOLS_BRANCH=b-ah-atools

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
EOF
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

AUTOTOOLS_REAL="Makefile.am 
          Makefile.am  
         resiprocate/Makefile.am 
            resiprocate/test/Makefile.am 
            configure.ac 
            acinclude.m4
            "


cvs up -r ${AUTOTOOLS_BRANCH} ${AUTOTOOLS_REAL} ${AUTOTOOLS_ARTIFACTS}
if [ "$1"x == "--remove-artifactsx" ]; then
 cvs remove -f ${AUTOTOOLS_ARTIFACTS}
fi
