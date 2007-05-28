# Microsoft Developer Studio Project File - Name="Netxx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Netxx - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Netxx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Netxx.mak" CFG="Netxx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Netxx - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Netxx - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Netxx - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "NETXX_NO_INET6" /YX /FD /I../../include /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Netxx - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "NETXX_NO_INET6" /YX /FD /GZ /I../../include /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Netxx - Win32 Release"
# Name "Netxx - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\Accept.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Address.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Datagram.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\DatagramServer.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\inet6.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\OSError.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Peer.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Probe.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Probe_select.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\RecvFrom.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Resolve_gethostbyname.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Resolve_getservbyname.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\ServerBase.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\SockAddr.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Socket.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\SockOpt.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Stream.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\StreamBase.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\StreamServer.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\Netxx\Address.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\Datagram.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\DatagramServer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\Netbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\Peer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\Probe.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\ProbeInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\SockOpt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\Stream.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\StreamBase.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\StreamServer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\Timeout.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Netxx\Types.h
# End Source File
# End Group
# End Target
# End Project
