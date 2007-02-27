BUILD := ../../build
include $(BUILD)/Makefile.pre

PACKAGES += DUM RESIP RUTIL ARES OPENSSL PTHREAD POPT DB4CXX
TARGET_LIBRARY = libsipdial
TARGET_BINARY = sipdialer
CXXFLAGS += -I./include/sipdial

SRC += \
	DialInstance.cpp \
	MyInviteSessionHandler.cpp \
	DialerConfiguration.cpp

include $(BUILD)/Makefile.post
