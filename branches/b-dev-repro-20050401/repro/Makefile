# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += DUM RESIPROCATE OPENSSL ARES PTHREAD

CODE_SUBDIRS = monkeys
TARGET_LIBRARY = librepro


TESTPROGRAMS = \
	repro.cxx \
	userAdmin.cxx 

CXXFLAGS += -I/sw/include
LDFLAGS  += -L/sw/lib 
LDLIBS   += -ldb-4.2 -lpopt


SRC =   \
	WebAdmin.cxx \
	HttpBase.cxx \
	HttpConnection.cxx \
        monkeys/DigestAuthenticator.cxx \
	monkeys/RouteProcessor.cxx \
	monkeys/AmIResponsible.cxx \
	monkeys/LocationServer.cxx \
	monkeys/ConstantLocationMonkey.cxx \
	CommandLineParser.cxx \
	ReproServerAuthManager.cxx \
	Proxy.cxx \
	Registrar.cxx \
	RequestContext.cxx \
	ResponseContext.cxx \
	UserDb.cxx \
	UserAbstractDb.cxx \
	WebAdminThread.cxx \
        RequestProcessor.cxx \
        RequestProcessorChain.cxx



include $(BUILD)/Makefile.post


