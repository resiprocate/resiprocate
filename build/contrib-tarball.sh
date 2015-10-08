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
  echo "Please run this script from a git workspace"
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

CONTRIB_PREFIX=resiprocate-contrib-${RESIP_VERSION}
CONTRIB_TARBALL=${CONTRIB_PREFIX}.tar.gz
CONTRIB_ZIP=${CONTRIB_PREFIX}.zip

echo "Building tarball ${CONTRIB_TARBALL}..."

tar czf "${CONTRIB_TARBALL}" contrib
zip -r ${CONTRIB_ZIP} contrib

