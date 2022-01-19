#!/bin/bash
set -e

# notes:
# - the WebRTC code MUST be checked out in a directory called `src`,
#   gclient takes care of that for us

# This script and fork of the libwebrtc library is based
# on the fork of libwebrtc and packaging strategy used by
# the mediasoup project for their Android app

# This doesn't run on all platforms, only tested on amd64 (x86_64)

# Setup:
# 1. install Debian 11 (bullseye)
# 2. install Git:  sudo apt install git
# 3. download the JDK 1.8 and unpack it in /opt (configured below)
# 4. run this script
# 5. look for contrib/libwebrtc/src/*.aar

# Ideally, we would build the libwebrtc library for
# platforms other than Android, such as
# Linux, *BSD, Windows, Mac and iOS.  Both
# reSIProcate and libwebrtc support each of
# those platforms but additional effort is needed
# to enable and test them in this build script.

# References:
# https://medium.com/@silvestr1994/webrtc-on-android-part-1-building-b6982aad4b49
# https://webrtc.googlesource.com/src
# https://webrtc.googlesource.com/src/+/main/docs/native-code/development/index.md
# http://dev.chromium.org/developers/how-tos/install-depot-tools
# mediasoup:
# https://github.com/haiyangwu/webrtc-android-build
# https://github.com/haiyangwu/webrtc-mirror
# https://mediasoup.org/
# https://github.com/haiyangwu/mediasoup-client-android/blob/dev/mediasoup-client/build.gradle
# https://github.com/haiyangwu/mediasoup-demo-android


# Must download and unpack Oracle JDK 8 before running this script
# https://www.oracle.com/java/technologies/javase/javase8-archive-downloads.html
export JDK_HOME=/opt/jdk1.8.0_202
if [ ! -e ${JDK_HOME} ];
then
  echo "Please unpack JDK 1.8 at ${JDK_HOME}"
  exit 1
fi
export PATH=${JDK_HOME}/bin:$PATH

WORK_DIR=`readlink -f contrib`/libwebrtc
if [ -e ${WORK_DIR} ];
then
  echo "Aborting, already exists: ${WORK_DIR}"
  exit 1
fi
mkdir -p ${WORK_DIR}
cd ${WORK_DIR}

DEPOT_TOOLS_GIT=https://chromium.googlesource.com/chromium/tools/depot_tools.git
git clone ${DEPOT_TOOLS_GIT}
DEPOT_TOOLS_HOME=`readlink -f depot_tools`
if [ ! -e ${DEPOT_TOOLS_HOME} ];
then
  echo "Please clone ${DEPOT_TOOLS_GIT} at ${DEPOT_TOOLS_HOME}"
  exit 1
fi

export PATH=${DEPOT_TOOLS_HOME}:$PATH

gclient root
gclient config --spec 'solutions = [
  {
    "url": "https://github.com/dpocock/webrtc-mirror.git@50262226fcc07a2406fe226b2a8256ca9bcdb6c6",
    "managed": False,
    "name": "src",
    "deps_file": "DEPS",
    "custom_deps": {},
  },
]
target_os = ["android", "unix"]
'
gclient sync --nohooks --with_branch_heads

gclient sync

cd src

sudo apt install python2
# Debian 11 (bullseye) only has python2 and python3
# but many scripts still want to call `env python`
# so we put a symlink to python2 in the PATH
PY_BINDIR=`mktemp -d`
ln -s `which python2` ${PY_BINDIR}/python
export PATH=${PY_BINDIR}:$PATH

# optionally adjust scripts to use python2 or python3
#find . -name '*.py' -exec sed -i -e 's/^#.*env python$/#!\/usr\/bin\/env python3/g' {} \;

# for javap (JDK 11 doesn't work with M79)
#sudo apt install openjdk-11-jdk-headless

# install some prerequisites before we install legacy dependencies
sudo apt install libdbusmenu-glib-dev gcc-arm-linux-gnueabihf g++-10-arm-linux-gnueabihf

# fetch and install some legacy dependencies manually
ARCH=`dpkg --print-architecture`
wget http://deb.debian.org/debian/pool/main/libi/libindicator/libindicator3-7_0.5.0-4_${ARCH}.deb
wget http://deb.debian.org/debian/pool/main/liba/libappindicator/libappindicator3-1_0.4.92-7_${ARCH}.deb
wget http://deb.debian.org/debian/pool/main/liba/libappindicator/gir1.2-appindicator3-0.1_0.4.92-7_${ARCH}.deb
wget http://deb.debian.org/debian/pool/main/liba/libappindicator/libappindicator3-dev_0.4.92-7_${ARCH}.deb
sudo apt install ./libindicator3-7_0.5.0-4_${ARCH}.deb
sudo apt install ./libappindicator3-1_0.4.92-7_${ARCH}.deb
sudo apt install ./gir1.2-appindicator3-0.1_0.4.92-7_${ARCH}.deb
sudo apt install ./libappindicator3-dev_0.4.92-7_${ARCH}.deb

# FIXME - cache sudo perms before we begin or use fakeroot
echo "$0 running sudo to cache permissions - please type password to continue the script"
sudo true

sed \
  -e 's/python/python3/g' \
  -e 's/lib32gcc1/lib32gcc-s1/g' \
  -e 's/libapache2-mod-php5/libapache2-mod-php7.4/g' \
  -e 's/libbrlapi0.5/libbrlapi0.8/g' \
  -e 's/libffi6/libffi7/g' \
  -e 's/libgnome-keyring0//g' \
  -e 's/libgnome-keyring-dev//g' \
  -e 's/php5-cgi/php7.4-cgi/g' \
  -e 's/python3-crypto/python3-pycryptodome/g' \
  build/install-build-deps.sh > build/install-build-deps-2.sh
chmod a+x build/install-build-deps-2.sh

# install-chromeos-fonts.py is run using sudo so it
# does not inherit the PATH with custom python, we have
# to force it to use either python2 or python3 here
sed -i \
  -e 's/env python$/env python2/g' \
  -e 's/0755/0o755/g' \
  -e 's/0644/0o644/g' \
  build/linux/install-chromeos-fonts.py

build/install-build-deps-2.sh

# the next command line comes from
#  https://github.com/dpocock/webrtc-android-build/blob/master/README.md

# if the build fails, it is possible to run this command again

./tools_webrtc/android/build_aar.py \
  --extra-gn-args 'is_debug=false is_component_build=false is_clang=true rtc_include_tests=false rtc_use_h264=true rtc_enable_protobuf=false use_rtti=true use_custom_libcxx=false' \
  --build-dir ./out/release-build/

