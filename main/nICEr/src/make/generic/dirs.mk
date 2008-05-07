ICE_ICE_SRCDIR = $(ICE_ROOT)ice/
ICE_NET_SRCDIR = $(ICE_ROOT)net/
ICE_STUN_SRCDIR= $(ICE_ROOT)stun/
ICE_TESTUA_SRCDIR = $(ICE_ROOT)testua/
ICE_UTIL_SRCDIR = $(ICE_ROOT)util/
ICE_CRYPTO_SRCDIR = $(ICE_ROOT)crypto/

include $(ICE_ICE_SRCDIR)targets.mk
include $(ICE_NET_SRCDIR)targets.mk
include $(ICE_STUN_SRCDIR)targets.mk
include $(ICE_UTIL_SRCDIR)targets.mk
ifdef RESIPROCATE_SRCDIR
include $(ICE_TESTUA_SRCDIR)targets.mk
endif
include $(ICE_CRYPTO_SRCDIR)targets.mk

#ICE_ASYNC_SRCDIR = $(ICE_ROOT)async/
#ICE_INNER_SRCDIR = $(ICE_ROOT)inner/

#include $(ICE_ASYNC_SRCDIR)targets.mk
#include $(ICE_INNER_SRCDIR)targets.mk
