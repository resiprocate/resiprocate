# Microsoft Developer Studio Project File - Name="SIPSTACK" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=SIPSTACK - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SIPSTACK.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SIPSTACK.mak" CFG="SIPSTACK - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SIPSTACK - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "SIPSTACK - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SIPSTACK - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "SIPSTACK - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SIPSTACK - Win32 Release"
# Name "SIPSTACK - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\convertStringToInt.cxx
# End Source File
# Begin Source File

SOURCE=.\Data.cxx
# End Source File
# Begin Source File

SOURCE=.\Executive.cxx
# End Source File
# Begin Source File

SOURCE=.\FloatSubComponent.cxx
# End Source File
# Begin Source File

SOURCE=.\HeaderFieldValue.cxx
# End Source File
# Begin Source File

SOURCE=.\HeaderFieldValueList.cxx
# End Source File
# Begin Source File

SOURCE=.\HeaderTypes.cxx
# End Source File
# Begin Source File

SOURCE=.\IntSubComponent.cxx
# End Source File
# Begin Source File

SOURCE=.\Lock.cxx
# End Source File
# Begin Source File

SOURCE=.\Log.cxx
# End Source File
# Begin Source File

SOURCE=.\Logger.cxx
# End Source File
# Begin Source File

SOURCE=.\Message.cxx
# End Source File
# Begin Source File

SOURCE=.\MethodTypes.cxx
# End Source File
# Begin Source File

SOURCE=.\Mutex.cxx
# End Source File
# Begin Source File

SOURCE=.\ParserCategories.cxx
# End Source File
# Begin Source File

SOURCE=.\ParserCategory.cxx
# End Source File
# Begin Source File

SOURCE=.\Preparse.cxx
# End Source File
# Begin Source File

SOURCE=.\SipMessage.cxx
# End Source File
# Begin Source File

SOURCE=.\SipStack.cxx
# End Source File
# Begin Source File

SOURCE=.\StringSubComponent.cxx
# End Source File
# Begin Source File

SOURCE=.\SubComponent.cxx
# End Source File
# Begin Source File

SOURCE=.\SubComponentList.cxx
# End Source File
# Begin Source File

SOURCE=.\Subsystem.cxx
# End Source File
# Begin Source File

SOURCE=.\Symbols.cxx
# End Source File
# Begin Source File

SOURCE=.\testSipStack1.cxx
# End Source File
# Begin Source File

SOURCE=.\Timer.cxx
# End Source File
# Begin Source File

SOURCE=.\TimerMessage.cxx
# End Source File
# Begin Source File

SOURCE=.\TimerQueue.cxx
# End Source File
# Begin Source File

SOURCE=.\TransactionMap.cxx
# End Source File
# Begin Source File

SOURCE=.\TransactionState.cxx
# End Source File
# Begin Source File

SOURCE=.\Transport.cxx
# End Source File
# Begin Source File

SOURCE=.\TransportSelector.cxx
# End Source File
# Begin Source File

SOURCE=.\UdpTransport.cxx
# End Source File
# Begin Source File

SOURCE=.\UnknownSubComponent.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Condition.hxx
# End Source File
# Begin Source File

SOURCE=.\Data.hxx
# End Source File
# Begin Source File

SOURCE=.\Dialog.hxx
# End Source File
# Begin Source File

SOURCE=.\Executive.hxx
# End Source File
# Begin Source File

SOURCE=.\Fifo.hxx
# End Source File
# Begin Source File

SOURCE=.\FloatSubComponent.hxx
# End Source File
# Begin Source File

SOURCE=.\HeaderFieldValue.hxx
# End Source File
# Begin Source File

SOURCE=.\HeaderFieldValueList.hxx
# End Source File
# Begin Source File

SOURCE=.\HeaderTypes.hxx
# End Source File
# Begin Source File

SOURCE=.\Helper.hxx
# End Source File
# Begin Source File

SOURCE=.\HostSpecification.hxx
# End Source File
# Begin Source File

SOURCE=.\IntSubComponent.hxx
# End Source File
# Begin Source File

SOURCE=.\Lock.hxx
# End Source File
# Begin Source File

SOURCE=.\Lockable.hxx
# End Source File
# Begin Source File

SOURCE=.\Log.hxx
# End Source File
# Begin Source File

SOURCE=.\Logger.hxx
# End Source File
# Begin Source File

SOURCE=.\Message.hxx
# End Source File
# Begin Source File

SOURCE=.\MethodTypes.hxx
# End Source File
# Begin Source File

SOURCE=.\Mutex.hxx
# End Source File
# Begin Source File

SOURCE=.\ParseException.hxx
# End Source File
# Begin Source File

SOURCE=.\ParserCategories.hxx
# End Source File
# Begin Source File

SOURCE=.\ParserCategory.hxx
# End Source File
# Begin Source File

SOURCE=.\ParserContainer.hxx
# End Source File
# Begin Source File

SOURCE=.\ParserContainerBase.hxx
# End Source File
# Begin Source File

SOURCE=.\Preparse.hxx
# End Source File
# Begin Source File

SOURCE=.\SipMessage.hxx
# End Source File
# Begin Source File

SOURCE=.\SipStack.hxx
# End Source File
# Begin Source File

SOURCE=.\StringSubComponent.hxx
# End Source File
# Begin Source File

SOURCE=.\SubComponent.hxx
# End Source File
# Begin Source File

SOURCE=.\SubComponentList.hxx
# End Source File
# Begin Source File

SOURCE=.\Subsystem.hxx
# End Source File
# Begin Source File

SOURCE=.\supported.hxx
# End Source File
# Begin Source File

SOURCE=.\Symbols.hxx
# End Source File
# Begin Source File

SOURCE=.\SysLogBuf.hxx
# End Source File
# Begin Source File

SOURCE=.\SysLogStream.hxx
# End Source File
# Begin Source File

SOURCE=.\Timer.hxx
# End Source File
# Begin Source File

SOURCE=.\TimerMessage.hxx
# End Source File
# Begin Source File

SOURCE=.\TimerQueue.hxx
# End Source File
# Begin Source File

SOURCE=.\TransactionMap.hxx
# End Source File
# Begin Source File

SOURCE=.\TransactionState.hxx
# End Source File
# Begin Source File

SOURCE=.\Transport.hxx
# End Source File
# Begin Source File

SOURCE=.\TransportSelector.hxx
# End Source File
# Begin Source File

SOURCE=.\UdpTransport.hxx
# End Source File
# Begin Source File

SOURCE=.\UnknownSubComponent.hxx
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
