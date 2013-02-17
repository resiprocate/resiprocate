# Microsoft Developer Studio Project File - Name="unittest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=unittest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "unittest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "unittest.mak" CFG="unittest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unittest - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "unittest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug/obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug/obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /GX /Zi /I "../../src" /I "hush" /D "DEBUG" /D "NO_TEMPLATES" /D "USE_WINDOWS_LOCKING" /Zm200 /GZ /c
# ADD CPP /nologo /MT /GX /Zi /I "../../src" /I "hush" /D "DEBUG" /D "NO_TEMPLATES" /D "USE_WINDOWS_LOCKING" /Zm200 /GZ /c
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Debug/libopensigcomp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:IX86 /pdbtype:sept
# ADD LINK32 Debug/libopensigcomp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:IX86 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "unittest - Win32 Debug"
# Begin Source File

SOURCE=..\..\src\test\inflate.cpp
DEP_CPP_INFLA=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\test\inflate.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\inflate.h
# End Source File
# Begin Source File

SOURCE=..\..\src\test\main.cpp
DEP_CPP_MAIN_=\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\osc_generators.cpp
DEP_CPP_OSC_G=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\State.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\osc_generators.h"\
	
NODEP_CPP_OSC_G=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\osc_generators.h
# End Source File
# Begin Source File

SOURCE=..\..\src\test\sha1_hash_vector.cpp
DEP_CPP_SHA1_=\
	"..\..\src\test\sha1_hash_vector.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\sha1_hash_vector.h
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_BitBuffer.cpp
DEP_CPP_TEST_=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_Buffer.cpp
DEP_CPP_TEST_O=\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_Compartment.cpp
DEP_CPP_TEST_OS=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\CompartmentMap.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\Libc.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\NackMap.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\State.h"\
	"..\..\src\StateHandler.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\osc_generators.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OS=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_CompartmentMap.cpp
DEP_CPP_TEST_OSC=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\CompartmentMap.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\State.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_Compressor.cpp
DEP_CPP_TEST_OSC_=\
	"..\..\src\Compressor.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_CompressorData.cpp
DEP_CPP_TEST_OSC_C=\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_CrcComputer.cpp
DEP_CPP_TEST_OSC_CR=\
	"..\..\src\CrcComputer.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_DeflateCompressor.cpp
DEP_CPP_TEST_OSC_D=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\CompartmentMap.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\Libc.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\NackMap.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\State.h"\
	"..\..\src\StateChanges.h"\
	"..\..\src\StateHandler.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\inflate.h"\
	"..\..\src\test\TestList.h"\
	"..\..\src\Udvm.h"\
	
NODEP_CPP_TEST_OSC_D=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_DeflateData.cpp
DEP_CPP_TEST_OSC_DE=\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\DeflateData.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_DeflateDictionary.cpp
DEP_CPP_TEST_OSC_DEF=\
	"..\..\src\DeflateDictionary.h"\
	"..\..\src\Libc.h"\
	"..\..\src\MultiBuffer.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_MultiBuffer.cpp
DEP_CPP_TEST_OSC_M=\
	"..\..\src\Libc.h"\
	"..\..\src\MultiBuffer.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_MutexLockable.cpp
DEP_CPP_TEST_OSC_MU=\
	"..\..\src\MutexLockable.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_NackMap.cpp
DEP_CPP_TEST_OSC_N=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\CompartmentMap.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\Libc.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\NackMap.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\State.h"\
	"..\..\src\StateHandler.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\osc_generators.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_N=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_ReadWriteLockable.cpp
DEP_CPP_TEST_OSC_R=\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_Sha1Hasher.cpp
DEP_CPP_TEST_OSC_S=\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\test\sha1_hash_vector.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_S=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_SigcompMessage.cpp
DEP_CPP_TEST_OSC_SI=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_SI=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_SipDictionary.cpp
DEP_CPP_TEST_OSC_SIP=\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SipDictionary.h"\
	"..\..\src\State.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_SIP=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_Stack.cpp
DEP_CPP_TEST_OSC_ST=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\CompartmentMap.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\Libc.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\NackMap.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\SipDictionary.h"\
	"..\..\src\Stack.h"\
	"..\..\src\State.h"\
	"..\..\src\StateChanges.h"\
	"..\..\src\StateHandler.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\TcpStream.h"\
	"..\..\src\test\TestList.h"\
	"..\..\src\test\torture_tests.h"\
	"..\..\src\Udvm.h"\
	
NODEP_CPP_TEST_OSC_ST=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_State.cpp
DEP_CPP_TEST_OSC_STA=\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\State.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_STA=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_StateChanges.cpp
DEP_CPP_TEST_OSC_STAT=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\State.h"\
	"..\..\src\StateChanges.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\osc_generators.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_STAT=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_StateHandler.cpp
DEP_CPP_TEST_OSC_STATE=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\CompartmentMap.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\Libc.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\NackMap.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\State.h"\
	"..\..\src\StateHandler.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\osc_generators.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_STATE=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_StateList.cpp
DEP_CPP_TEST_OSC_STATEL=\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\State.h"\
	"..\..\src\StateList.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_STATEL=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_StateMap.cpp
DEP_CPP_TEST_OSC_STATEM=\
	"..\..\src\NackCodes.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\State.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\test\TestList.h"\
	
NODEP_CPP_TEST_OSC_STATEM=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_TcpStream.cpp
DEP_CPP_TEST_OSC_T=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\TcpStream.h"\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\test_osc_Udvm.cpp
DEP_CPP_TEST_OSC_U=\
	"..\..\src\BitBuffer.h"\
	"..\..\src\Compartment.h"\
	"..\..\src\CompartmentMap.h"\
	"..\..\src\Compressor.h"\
	"..\..\src\CompressorData.h"\
	"..\..\src\CrcComputer.h"\
	"..\..\src\DeflateCompressor.h"\
	"..\..\src\Libc.h"\
	"..\..\src\NackCodes.h"\
	"..\..\src\NackMap.h"\
	"..\..\src\ReadWriteLockable.h"\
	"..\..\src\Sha1Hasher.h"\
	"..\..\src\SigcompMessage.h"\
	"..\..\src\SipDictionary.h"\
	"..\..\src\State.h"\
	"..\..\src\StateChanges.h"\
	"..\..\src\StateHandler.h"\
	"..\..\src\StateList.h"\
	"..\..\src\StateMap.h"\
	"..\..\src\TcpStream.h"\
	"..\..\src\test\TestList.h"\
	"..\..\src\test\torture_tests.h"\
	"..\..\src\Udvm.h"\
	"..\..\src\UdvmOpcodes.h"\
	
NODEP_CPP_TEST_OSC_U=\
	"..\..\src\openssl\sha.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\TestList.cpp
DEP_CPP_TESTL=\
	"..\..\src\test\TestList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\TestList.h
# End Source File
# Begin Source File

SOURCE=..\..\src\test\torture_tests.cpp
DEP_CPP_TORTU=\
	"..\..\src\NackCodes.h"\
	"..\..\src\test\torture_tests.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\src\test\torture_tests.h
# End Source File
# End Target
# End Project
