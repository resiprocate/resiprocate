# This should be the path to the build directory of resip (in the sip subdirectory)
BUILD = ../build

# Includes macros
include $(BUILD)/Makefile.pre

# All of these packages are prerequisites for resiprocate
PACKAGES += RESIP RUTIL OPENSSL ARES PTHREAD

# Add an entry to TESTPROGRAMS for each target that has a main in it
# On linux this will generate an executable in bin.debug.Linux.i686/main
TESTPROGRAMS += presSvr.cpp

# Add each of the C++ or C files that other than the main
# Each main target (from TESTPROGRAMS) will be linked with all of the files in SRC
SRC = 		PresConfig.cpp TuPresSvr.cpp ResourceMgr.cpp SubDialogMgr.cpp SubDialog.cpp Resource.cpp DialogSetMgr.cpp DialogState.cpp

# Includes macros
include $(BUILD)/Makefile.post

