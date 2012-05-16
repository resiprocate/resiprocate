#!/bin/bash

# this script makes the contrib tarball

# it must be run from the top level of the source tree, the same
# directory where configure is located

if [ ! -x configure ];
then
  echo "ERROR:"
  echo "Can\'t find configure, or it is not executable"
  echo "Please make sure that:"
  echo " a) you are using the source tree from the repository"
  echo " b) you are in the top level directory of the reSIProcate source"
  echo " c) you have already run"
  echo ""
  echo "       autoreconf --install"
  echo ""
  echo "    to create configure"
  echo "Operation failed."
  exit 1
fi

if [ ! -d .git ];
then
  echo "ERROR:"
  echo "Please run this script from a git-svn workspace"
  echo "Using git-svn simplifies this process because SVN creates many"
  echo ".svn metadata directories throughout the source tree."
  echo "Operation failed."
  exit 1
fi

if [ ! -d contrib ];
then
  echo "ERROR:"
  echo "contrib/ directory doesn\'t exist"
  echo "Operation failed."
  exit 1
fi

LOCAL_CHANGES=`git status --untracked-files=normal --porcelain contrib | wc -l`
  
if [ "${LOCAL_CHANGES}" -gt 0 ];
then
  echo "ERROR:"
  echo "There are local changes or untracked files in contrib"
  echo "Please remove them with"
  echo ""
  echo "  git clean"
  echo ""
  echo "stash them with"
  echo ""
  echo "  git stash"
  echo ""
  echo "or run this script from another workspace that is clean."
  echo "Operation failed."
  exit 1
fi

RESIP_VERSION=`./configure -V | grep '^resiprocate configure' | cut -f3 -d' '`

CONTRIB_TARBALL=resiprocate-contrib-${RESIP_VERSION}.tar.gz

echo "Building tarball ${CONTRIB_TARBALL}..."

tar czf "${CONTRIB_TARBALL}" contrib

