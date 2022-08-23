#!/bin/bash
set -e
set -x

# This is a script for comparing the output of the Autotools
# and CMake build systems.
# Copyright (C) 2022 Daniel Pocock https://danielpocock.com

# Autotools is the system that is currently supported for non-Windows
# builds.

# CMake is proposed as a replacement.  It is discussed here:
# https://github.com/resiprocate/resiprocate/pull/107

# Pending improvements to this comparison script:
# - adapt build.sh on the branch, use it from here (FIXME below)
# - adapt release-tarball.sh on the branch, use it from here (FIXME below)
# - do the `make install` and `make dist`
#   in different clones of the repository
# - run dpkg-buildpackage
# - scp the tarball to a Fedora host and run rpmbuild
# - automatically check the diff reports

# Configuration for this script:

GIT_REPO=https://github.com/resiprocate/resiprocate
CMAKE_COMMIT=b-dpocock-cmake
AUTOTOOLS_BRANCH=master

# End of configuration

WORK_DIR=`mktemp -d`
cd ${WORK_DIR}

AUTOTOOLS_BUILD=${WORK_DIR}/autotools-build
CMAKE_BUILD=${WORK_DIR}/cmake-build
PARALLEL="-j`grep -c ^processor /proc/cpuinfo`"

AUTOTOOLS_BUILD_TARBALL=${WORK_DIR}/autotools-build-tarball
CMAKE_BUILD_TARBALL=${WORK_DIR}/cmake-build-tarball

AUTOTOOLS_SOURCE_LIST=${WORK_DIR}/autotools-source-list.txt
AUTOTOOLS_OBJECT_LIST=${WORK_DIR}/autotools-object-list.txt
AUTOTOOLS_UNBUILT_SOURCES=${WORK_DIR}/autotools-unbuilt-sources.txt
AUTOTOOLS_TARBALL_CONTENTS=${WORK_DIR}/autotools-tarball-contents.txt
AUTOTOOLS_DESTDIR=${WORK_DIR}/autotools-install
AUTOTOOLS_DESTDIR_LIST=${WORK_DIR}/autotools-install-list.txt
AUTOTOOLS_DESTDIR2=${WORK_DIR}/autotools-install2
AUTOTOOLS_DESTDIR2_LIST=${WORK_DIR}/autotools-install2-list.txt

CMAKE_SOURCE_LIST=${WORK_DIR}/cmake-source-list.txt
CMAKE_OBJECT_LIST=${WORK_DIR}/cmake-object-list.txt
CMAKE_UNBUILT_SOURCES=${WORK_DIR}/cmake-unbuilt-sources.txt
CMAKE_TARBALL_CONTENTS=${WORK_DIR}/cmake-tarball-contents.txt
CMAKE_DESTDIR=${WORK_DIR}/cmake-install
CMAKE_DESTDIR_LIST=${WORK_DIR}/cmake-install-list.txt
CMAKE_DESTDIR2=${WORK_DIR}/cmake-install2
CMAKE_DESTDIR2_LIST=${WORK_DIR}/cmake-install2-list.txt

AUTOTOOLS_CMAKE_SOURCES_DIFF=${WORK_DIR}/autotools-cmake-sources-diff.txt
AUTOTOOLS_CMAKE_OBJECTS_DIFF=${WORK_DIR}/autotools-cmake-objects-diff.txt
AUTOTOOLS_CMAKE_TARBALL_DIFF=${WORK_DIR}/autotools-cmake-tarball-diff.txt
AUTOTOOLS_CMAKE_DESTDIR_DIFF=${WORK_DIR}/autotools-cmake-install-diff.txt
AUTOTOOLS_CMAKE_DESTDIR2_DIFF=${WORK_DIR}/autotools-cmake-install2-diff.txt

###############################################################
# Clone and checkout the branches

git clone ${GIT_REPO} ${CMAKE_BUILD}
cd ${CMAKE_BUILD}
git checkout ${CMAKE_COMMIT}

# This should be the last common commit between master and
# the CMake branch:
AUTOTOOLS_COMMIT=`git merge-base ${AUTOTOOLS_BRANCH} ${CMAKE_COMMIT}`

git clone ${GIT_REPO} ${AUTOTOOLS_BUILD}
cd ${AUTOTOOLS_BUILD}
git checkout ${AUTOTOOLS_COMMIT}

###############################################################
# Autotools tests:
# - captures the current output of the build scripts
# - checks for faults in the current build process, e.g. uncompiled sources

cd ${AUTOTOOLS_BUILD}
build/debian.sh
find . -type f -name '*.cxx' | sort | sed -e 's/.cxx$//' > ${AUTOTOOLS_SOURCE_LIST}
make ${PARALLEL} check
find . -type f -name '*.o' | sort | sed -e 's/.o$//' > ${AUTOTOOLS_OBJECT_LIST}
diff -u ${AUTOTOOLS_SOURCE_LIST} ${AUTOTOOLS_OBJECT_LIST} > \
  ${AUTOTOOLS_UNBUILT_SOURCES} \
  || if [ $? -gt 1 ] ; then exit 1 ; fi

mkdir -p ${AUTOTOOLS_DESTDIR} && make ${PARALLEL} DESTDIR=${AUTOTOOLS_DESTDIR} install
find ${AUTOTOOLS_DESTDIR} -type f | sort > ${AUTOTOOLS_DESTDIR_LIST}

make clean
build/release-tarball.sh
tar tzf *.tar.gz | sort -u > ${AUTOTOOLS_TARBALL_CONTENTS}
OUTPUT_TARBALL=`readlink -f *.tar.gz`
mkdir -p ${AUTOTOOLS_BUILD_TARBALL} && cd ${AUTOTOOLS_BUILD_TARBALL}
tar xzf ${OUTPUT_TARBALL} && cd *
build/debian.sh
make ${PARALLEL} check
mkdir -p ${AUTOTOOLS_DESTDIR2} && make ${PARALLEL} DESTDIR=${AUTOTOOLS_DESTDIR2} install
find ${AUTOTOOLS_DESTDIR2} -type f | sort > ${AUTOTOOLS_DESTDIR2_LIST}

###############################################################
# CMake tests:
# - captures the current output of the build scripts

cd ${CMAKE_BUILD}
# FIXME: uncomment when complete
#build/debian.sh
# FIXME: These flags were copied from the Github issue
# The desired flags need to be placed in the build/debian.sh script
# and other scripts used for packaging and CI builds
CMAKE_FLAGS=" \
  -DWITH_SSL=ON \
  -DWITH_C_ARES=ON \
  "
cmake ${CMAKE_FLAGS} .
find . -type f -name '*.cxx' | sort | sed -e 's/.cxx$//' > ${CMAKE_SOURCE_LIST}
make ${PARALLEL}
make ${PARALLEL} test
find . -type f -name '*.o' | sort | sed -e 's/.o$//' > ${CMAKE_OBJECT_LIST}
diff -u ${CMAKE_SOURCE_LIST} ${CMAKE_OBJECT_LIST} > \
  ${CMAKE_UNBUILT_SOURCES} \
  || if [ $? -gt 1 ] ; then exit 1 ; fi

mkdir -p ${CMAKE_DESTDIR} && make ${PARALLEL} DESTDIR=${CMAKE_DESTDIR} install
find ${CMAKE_DESTDIR} -type f | sort > ${CMAKE_DESTDIR_LIST}

make clean
# FIXME: uncomment when complete
#build/release-tarball.sh
make DESTDIR=${CMAKE_DESTDIR} dist
tar tzf *.tar.gz | sort -u > ${CMAKE_TARBALL_CONTENTS}
OUTPUT_TARBALL=`readlink -f *.tar.gz`
mkdir -p ${CMAKE_BUILD_TARBALL} && cd ${CMAKE_BUILD_TARBALL}
tar xzf ${OUTPUT_TARBALL} && cd *
# FIXME: uncomment when complete
#build/debian.sh
cmake ${CMAKE_FLAGS} .
make ${PARALLEL}
make ${PARALLEL} test
mkdir -p ${CMAKE_DESTDIR2} && make ${PARALLEL} DESTDIR=${CMAKE_DESTDIR2} install
find ${CMAKE_DESTDIR2} -type f | sort > ${CMAKE_DESTDIR2_LIST}

###############################################################
# Compare the reports from the Autotools and CMake builds

diff -u ${AUTOTOOLS_SOURCE_LIST} ${CMAKE_SOURCE_LIST} > \
  ${AUTOTOOLS_CMAKE_SOURCES_DIFF} \
  || if [ $? -gt 1 ] ; then exit 1 ; fi

diff -u ${AUTOTOOLS_OBJECT_LIST} ${CMAKE_OBJECT_LIST} > \
  ${AUTOTOOLS_CMAKE_OBJECTS_DIFF} \
  || if [ $? -gt 1 ] ; then exit 1 ; fi

diff -u ${AUTOTOOLS_TARBALL_CONTENTS} ${CMAKE_TARBALL_CONTENTS} > \
  ${AUTOTOOLS_CMAKE_TARBALL_DIFF} \
  || if [ $? -gt 1 ] ; then exit 1 ; fi

diff -u ${AUTOTOOLS_DESTDIR_LIST} ${CMAKE_DESTDIR_LIST} > \
  ${AUTOTOOLS_CMAKE_DESTDIR_DIFF} \
  || if [ $? -gt 1 ] ; then exit 1 ; fi

diff -u ${AUTOTOOLS_DESTDIR2_LIST} ${CMAKE_DESTDIR2_LIST} > \
  ${AUTOTOOLS_CMAKE_DESTDIR2_DIFF} \
  || if [ $? -gt 1 ] ; then exit 1 ; fi

