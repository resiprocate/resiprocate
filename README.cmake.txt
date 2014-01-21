CMake Instructions
==================

This is currently an experiement to see how CMake could be used to build
reSIProcate.

Currently, only the core of rutil and its unit tests are built. This is a proof
of concept...

This was quickly tested on Mac OS, Linux (CentOS) and Windows 7...

To build an _out of tree_ build:

$ mkdir cmake_build # Or any other name
$ cd cmake_build
$ cmake ..
$ make

Once this is built, you can run the unit tests with:

$ ctest

or, for verbose output:

$ ctest -V

If you want to start fresh either delete the out of tree build directory or
delete the CMakeCache.txt file.

Configuration Flags
===================

* This is not fully implemented on all flags yet *

CMake "cached" variables are used to specify options such as whether c-ares
should be used or not. You can get the list and tweak them with:

$ cmake -i ..

You can also set them on the command line like:

$ cmake -DWITH_C_ARES=true ..

Future Considerations
=====================

Installation Packages
---------------------

CMake supports rpm/deb?/NSIS to build installable packages using CPack. If this
built-in support can not be used we could always specify internal build targets
that will run external commands to package the appropriate bundle depending on
the platform.

