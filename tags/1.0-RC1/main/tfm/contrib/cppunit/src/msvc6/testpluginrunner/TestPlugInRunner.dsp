# Microsoft Developer Studio Project File - Name="TestPlugInRunner" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=TestPlugInRunner - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TestPlugInRunner.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TestPlugInRunner.mak" CFG="TestPlugInRunner - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TestPlugInRunner - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "TestPlugInRunner - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TestPlugInRunner - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "../../include" /I "../TestRunner" /I "..\..\..\include" /I "..\..\..\include\msvc6" /I "..\TestRunner" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /D "CPPUNIT_SUBCLASSING_TESTRUNNERDLG_BUILD" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ../../../lib/cppunit.lib ../../../lib/testrunner.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "TestPlugInRunner - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\..\..\include" /I "..\..\..\include\msvc6" /I "..\TestRunner" /D "_DEBUG" /D "CPPUNIT_TESTPLUGINRUNNER_BUILD" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /D "CPPUNIT_SUBCLASSING_TESTRUNNERDLG_BUILD" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ../../../lib/cppunitd.lib ../../../lib/testrunnerd.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TestPlugInRunner - Win32 Release"
# Name "TestPlugInRunner - Win32 Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=..\testrunner\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=..\testrunner\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=..\testrunner\res\idr_test.ico
# End Source File
# Begin Source File

SOURCE=..\testrunner\res\test_type.bmp
# End Source File
# Begin Source File

SOURCE=.\res\TestPlugInRunner.ico
# End Source File
# Begin Source File

SOURCE=.\res\TestPlugInRunner.rc2
# End Source File
# Begin Source File

SOURCE=..\testrunner\res\tfwkui_r.bmp
# End Source File
# End Group
# Begin Group "Gui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TestPlugInRunner.rc
# End Source File
# Begin Source File

SOURCE=.\TestPlugInRunnerApp.cpp
# End Source File
# Begin Source File

SOURCE=.\TestPlugInRunnerApp.h
# End Source File
# Begin Source File

SOURCE=.\TestPlugInRunnerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\TestPlugInRunnerDlg.h
# End Source File
# End Group
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\include\msvc6\testrunner\TestPlugInInterface.h
# End Source File
# End Group
# Begin Group "Models"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\TestPlugIn.cpp
# End Source File
# Begin Source File

SOURCE=.\TestPlugIn.h
# End Source File
# Begin Source File

SOURCE=.\TestPlugInException.cpp
# End Source File
# Begin Source File

SOURCE=.\TestPlugInException.h
# End Source File
# Begin Source File

SOURCE=.\TestPlugInRunnerModel.cpp
# End Source File
# Begin Source File

SOURCE=.\TestPlugInRunnerModel.h
# End Source File
# End Group
# Begin Group "DLL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\lib\testrunner.dll

!IF  "$(CFG)" == "TestPlugInRunner - Win32 Release"

# Begin Custom Build - Updating DLL: $(InputPath)
IntDir=.\Release
InputPath=..\..\..\lib\testrunner.dll

"$(IntDir)\testrunner.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) $(IntDir)\testrunner.dll

# End Custom Build

!ELSEIF  "$(CFG)" == "TestPlugInRunner - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\lib\testrunnercd.dll
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\testrunnerd.dll

!IF  "$(CFG)" == "TestPlugInRunner - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "TestPlugInRunner - Win32 Debug"

# Begin Custom Build - Updating DLL: $(InputPath)
IntDir=.\Debug
InputPath=..\..\..\lib\testrunnerd.dll

"$(IntDir)\testrunnerd.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) $(IntDir)\testrunnerd.dll

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
