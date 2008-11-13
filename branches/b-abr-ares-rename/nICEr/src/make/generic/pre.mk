ICE_ROOT=$(PWD)/../../

GLOBAL_LIBNAME = libICE.a

include $(PWD)/../generic/rules.mk
include $(PWD)/../generic/dirs.mk
include $(PWD)/../generic/test-dirs.mk
-include $(PWD)/*.d

GLOBAL_CFLAGS += -g -Werror -Wall -Wno-parentheses -DHAVE_STRDUP
GLOBAL_CFLAGS += -D__UNUSED__="__attribute__((unused))" -Drestrict=__restrict__
GLOBAL_CFLAGS += -I$(NRAPPKIT_BUILD_DIR)
GLOBAL_CFLAGS += -I$(NRAPPKIT_SRCDIR)/src/util
GLOBAL_CFLAGS += -I$(NRAPPKIT_SRCDIR)/src/util/libekr
GLOBAL_CFLAGS += -I$(NRAPPKIT_SRCDIR)/src/port/$(PLATFORM)/include
GLOBAL_CFLAGS += -DOPENSSL -I$(OPENSSL_SRC_DIR)include

GLOBAL_BUILD += $(GLOBAL_LIBNAME)

GLOBAL_LDFLAGS += $(GLOBAL_LIBNAME) 
GLOBAL_LDFLAGS += -L$(NRAPPKIT_BUILD_DIR) -lnrappkit
GLOBAL_LDFLAGS += -L$(OPENSSL_SRC_DIR) -lssl -lcrypto 
