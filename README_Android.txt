
About
-----

Android is a Linux-like operating system.  It uses the Linux kernel
and the bionic C library rather than the more common glibc.

Android comes with an SDK for Java, the primary language used in
app development and a Native Development Kit (NDK) for low-level
languages such as C++.

History
-------

Android support was first added to the stack in 2013 by Daniel Pocock

The Android build was updated in 2021 for API level 21 with
support from Mobile Insight.

SDK and NDK evolution
---------------------

The original SDK was based on the Eclipse IDE.  Since 2014, the SDK
is Android Studio, based on JetBrains' IntelliJ.

In the early days of the NDK, developers figured out how to integrate
their build systems with the NDK toolchains manually using scripts.
This was the approach used in the original reSIProcate Android effort.
Specifically, the build/android-custom-ndk script was created as a
wrapper around autotools.  More recently, Google has included this
strategy in their documentation:
https://developer.android.com/ndk/guides/other_build_systems#autoconf

In 2016, Android Studio 2.2 introduced support for CMake with the NDK.
Using CMake is not mandatory.  The SDK/NDK bundles a specific version
of CMake and if reSIProcate changes to CMake, it will be helpful
to ensure the versions are compatible.
https://android-developers.googleblog.com/2016/11/make-and-ndk-build-support-in-android.html

The original NDK was using the GCC toolchain for cross-compiling.
In around 2016, GCC was deprecated in favor of the CLang toolchain.

The list of CPU architectures supported by Android has evolved over time:
https://developer.android.com/ndk/guides/abis

Building for Android
--------------------

On a Linux host, install the SDK and NDK using the usual procedure.

Clone the reSIProcate project.

The first time you build reSIProcate, you will also need to
download and build OpenSSL for Android.  Simply add the environment
variable BUILD_OPENSSL=1 to the front of your command line.

It is necessary to set the NDK_HOME environment variable through
the command line or your shell profile.

Here is an example:

BUILD_OPENSSL=1 \
NDK_HOME=${HOME}/Android/Sdk/ndk/23.1.7779620 \
  build/android-custom-ndk

The binaries from both OpenSSL and reSIProcate are placed
in the directory ${HOME}/ndk-prebuilt

When you configure the logger in your code, we recommended you
enable the AndroidLogger from rutil/AndroidLogger.hxx
and then all the reSIProcate log output will be passed through
the Android logging system.

See the android-demo-message application for an example showing
how to integrate the libraries into your own app.
https://github.com/resiprocate/android-demo-message

