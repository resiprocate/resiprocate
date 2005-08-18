# Microsoft Developer Studio Project File - Name="cppunit_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cppunit_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cppunit_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cppunit_dll.mak" CFG="cppunit_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cppunit_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cppunit_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cppunit_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cppunit_dll___Win32_Release"
# PROP BASE Intermediate_Dir "cppunit_dll___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseDLL"
# PROP Intermediate_Dir "ReleaseDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_BUILD_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"..\..\lib\cppunit_dll.pdb" /machine:I386 /out:"..\..\lib\cppunit_dll.dll" /implib:"..\..\lib\cppunit_dll.lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "cppunit_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cppunit_dll___Win32_Debug"
# PROP BASE Intermediate_Dir "cppunit_dll___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugDLL"
# PROP Intermediate_Dir "DebugDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_BUILD_DLL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"..\..\lib\cppunitd_dll.pdb" /debug /machine:I386 /out:"..\..\lib\cppunitd_dll.dll" /implib:"..\..\lib\cppunitd_dll.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "cppunit_dll - Win32 Release"
# Name "cppunit_dll - Win32 Debug"
# Begin Group "extension"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\Orthodox.h
# End Source File
# Begin Source File

SOURCE=.\RepeatedTest.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\RepeatedTest.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestDecorator.h
# End Source File
# Begin Source File

SOURCE=.\TestSetUp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestSetUp.h
# End Source File
# End Group
# Begin Group "helper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\AutoRegisterSuite.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\HelperMacros.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestCaller.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestFactory.h
# End Source File
# Begin Source File

SOURCE=.\TestFactoryRegistry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestFactoryRegistry.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestSuiteBuilder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestSuiteFactory.h
# End Source File
# Begin Source File

SOURCE=.\TypeInfoHelper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TypeInfoHelper.h
# End Source File
# End Group
# Begin Group "core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Asserter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Asserter.h
# End Source File
# Begin Source File

SOURCE=.\Exception.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Exception.h
# End Source File
# Begin Source File

SOURCE=.\NotEqualException.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\NotEqualException.h
# End Source File
# Begin Source File

SOURCE=.\SourceLine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\SourceLine.h
# End Source File
# Begin Source File

SOURCE=.\SynchronizedObject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\SynchronizedObject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Test.h
# End Source File
# Begin Source File

SOURCE=.\TestAssert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestAssert.h
# End Source File
# Begin Source File

SOURCE=.\TestCase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestCase.h
# End Source File
# Begin Source File

SOURCE=.\TestFailure.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestFailure.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestFixture.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestListener.h
# End Source File
# Begin Source File

SOURCE=.\TestResult.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestResult.h
# End Source File
# Begin Source File

SOURCE=.\TestSuite.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestSuite.h
# End Source File
# End Group
# Begin Group "output"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CompilerOutputter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\CompilerOutputter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Outputter.h
# End Source File
# Begin Source File

SOURCE=.\TestResultCollector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestResultCollector.h
# End Source File
# Begin Source File

SOURCE=.\TestSucessListener.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestSucessListener.h
# End Source File
# Begin Source File

SOURCE=.\TextOutputter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextOutputter.h
# End Source File
# Begin Source File

SOURCE=.\TextTestProgressListener.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextTestProgressListener.h
# End Source File
# Begin Source File

SOURCE=.\TextTestResult.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextTestResult.h
# End Source File
# Begin Source File

SOURCE=.\XmlOutputter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\XmlOutputter.h
# End Source File
# End Group
# Begin Group "portability"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\include\cppunit\config-msvc6.h"
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Portability.h
# End Source File
# End Group
# Begin Group "textui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\TestRunner.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunitui\text\TestRunner.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextTestRunner.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\ChangeLog
# End Source File
# Begin Source File

SOURCE=.\DllMain.cpp
# End Source File
# Begin Source File

SOURCE="..\..\INSTALL-WIN32.txt"
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Makefile.am
# End Source File
# Begin Source File

SOURCE=.\Makefile.am
# End Source File
# Begin Source File

SOURCE=..\..\NEWS
# End Source File
# End Target
# End Project
