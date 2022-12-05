@echo off

rem Choose one of the CMake Windows supported
rem target architectures:
rem set ARCH=x64
set ARCH=Win32

rem CMake doesn't have any feature to automatically
rem create a PATH containing the DLLs from NuGet
rem packages.

rem It is necessary to improvise a solution, for
rem example, building a PATH manually or copying
rem all the DLLs to a single location that is
rem already in PATH.

rem We manually add the NuGet DLLs to the path
rem so that the tests can run.
set PKGROOT=%cd%\Builds\packages
set PATH=%PATH%^
;%PKGROOT%\zeroc.openssl.v142\build\native\bin\%ARCH%\Debug^
;%PKGROOT%\berkeley.db.v140\build\native\bin\%ARCH%\Debug

echo %PATH%

cmake^
 --build Builds^
 --config Debug^
 --target RUN_TESTS
