#!/bin/sh
# $Id:$

# We 'remove' these files in SVN when using autotools.
AUTOTOOLS_CONFLICTS="
        Makefile.in
        Makefile 
        configure 
        configure.in  
        config.sub 
        missing 
        aclocal.m4
	config.hxx
	config.hxx.in
"

# These are files that we care about and that
# are part of the autotools generation stuff.
# These files are source files in autotools/* parallel tree.

AUTOTOOLS_USED="Makefile.am
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
                     
makeLink () {
    RD=`echo ${1} | sed 's:[^/][^/]*:..:g;s:^\.\.:.:g'`/autotools/${1}
    echo Making link for $1
    [ -e $(dirname ${1})/${RD} ] || echo missing ${RD}
    if [ -h ${1} ]; then
        echo "${1}: already exists!" >&2
    else
        ln -s ${RD} ${1} || echo "${1}: error making link." >&2
    fi
}


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
You must run this script from the repro's top-level directory.
(The directory that contains use-autotools.sh)
EOF
exit 1
fi

    # Determine your repository URI.
    URI=`svn info | grep '^URL:' | sed 's,.*\(http.*://.*/rep/resiprocate\).*,\1,g'`
    SCHEME=`echo $URI | sed 's,\(./*\)://.*,\1,g'`
    if [ "${SCHEME}" != "https" ]; then
        cat <<EOF

WARNING: Your repository URI is based on a '${SCHEME}:' checkout.
WARNING: In order to check any changes back in, you need to be
WARNING: using https.  Use 'svn switch' to alter the URI for this
WARNING: working view before attempting any commits.


EOF
    fi

    cat <<EOF
This script will prep a 'main' repro checkout to
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
[ "${DUMMY}x" = "yesx" ] || exit -1

ATCF=.autotools-conflicts-list

# Reset the conflicts list
find . -name ${ATCF} -exec cp /dev/null {} \;

# Remove conflicting files -- and add to ignore list.
for F in ${AUTOTOOLS_CONFLICTS} ${AUTOTOOLS_USED}; do
    for E in echo "" ; do
        [ -e "${F}" ] && ${E} rm -f ${F}
    done
    Fd=$(dirname ${F})
    Fb=$(basename ${F})
    echo ${Fb} >> ${Fd}/${ATCF}
done


# Make links to autotools files as needed.
for B in ${AUTOTOOLS_USED}; do
        makeLink "${B}"
done


# Add artifacts (toplevel only) to ignore list.
for F in ${ATCF} ${AUTOTOOLS_ARTIFACTS}; do
    echo ${F} >> ${ATCF}
done

# Propset so autotools stuff is ignored for now.
for D in $( find . -name ${ATCF} -print ) ;do
    ( cd $(dirname $D) ; pwd ; echo ${ATCF} >> ${ATCF} ; \
        cat .cvsignore ${ATCF} > /tmp/$$.atcf ;\
        svn propset svn:ignore -F /tmp/$$.atcf . ;\
        rm -f /tmp/$$.atcf)
done


