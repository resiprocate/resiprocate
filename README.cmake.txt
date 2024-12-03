CMake Instructions
==================

This is currently an experiement to see how CMake could be used to build
reSIProcate.

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

Windows:
>mkdir cmake_build # Or any other name
>cd cmake_build
>cmake ..
Open resiprocate.sln in visual studio 

If you want to the unit tests, then right click on the RUN_TESTS project and Build it.


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

