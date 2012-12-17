# Microsoft Developer Studio Project File - Name="libopensigcomp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libopensigcomp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libopensigcomp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libopensigcomp.mak" CFG="libopensigcomp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libopensigcomp - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libopensigcomp - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libopensigcomp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug/obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug/obj"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /I "../generated" /I "../../src" /I "hush" /Zi /D "DEBUG" /D "NO_TEMPLATES" /D "USE_WINDOWS_LOCKING" /EHsc /GZ /c /GX 
# ADD CPP /nologo /MT /I "../generated" /I "../../src" /I "hush" /Zi /D "DEBUG" /D "NO_TEMPLATES" /D "USE_WINDOWS_LOCKING" /EHsc /GZ /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo 
# ADD LIB32 /nologo 

!ELSEIF  "$(CFG)" == "libopensigcomp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release/obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release/obj"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /I "../generated" /I "../../src" /I "hush" /D "NO_TEMPLATES" /D "USE_WINDOWS_LOCKING" /EHsc /c /GX 
# ADD CPP /nologo /MT /I "../generated" /I "../../src" /I "hush" /D "NO_TEMPLATES" /D "USE_WINDOWS_LOCKING" /EHsc /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo 
# ADD LIB32 /nologo 

!ENDIF

# Begin Target

# Name "libopensigcomp - Win32 Release"
# Name "libopensigcomp - Win32 Debug"
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\BitBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Compartment.h
# End Source File
# Begin Source File

SOURCE=..\..\src\CompartmentMap.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Compressor.h
# End Source File
# Begin Source File

SOURCE=..\..\src\CompressorData.h
# End Source File
# Begin Source File

SOURCE=..\..\src\CrcComputer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DeflateCompressor.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DeflateData.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DeflateDictionary.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Libc.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MultiBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MutexLockable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\NackCodes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\NackMap.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ProfileStack.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ReadWriteLockable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Sha1Hasher.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SigcompMessage.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SipDictionary.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Stack.h
# End Source File
# Begin Source File

SOURCE=..\..\src\State.h
# End Source File
# Begin Source File

SOURCE=..\..\src\StateChanges.h
# End Source File
# Begin Source File

SOURCE=..\..\src\StateHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\src\StateList.h
# End Source File
# Begin Source File

SOURCE=..\..\src\StateMap.h
# End Source File
# Begin Source File

SOURCE=..\..\src\TcpStream.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Types.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Udvm.h
# End Source File
# Begin Source File

SOURCE=..\..\src\UdvmOpcodes.h
# End Source File
# End Group
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\BitBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Buffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Compartment.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\CompartmentMap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Compressor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\CompressorData.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\CrcComputer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\DeflateCompressor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\DeflateData.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\DeflateDictionary.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\MultiBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\NackCodes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\NackMap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Sha1Hasher.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\SigcompMessage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\SipDictionary.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Stack.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\State.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\StateChanges.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\StateHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\StateList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\StateMap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\TcpStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Udvm.cpp
# End Source File
# End Group
# Begin Group "Generated"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\generated\DeflateBytecodes.cpp
# End Source File
# Begin Source File

SOURCE=..\generated\DeflateBytecodes.h
# End Source File
# End Group
# End Target
# End Project

