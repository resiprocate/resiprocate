# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += DUM RESIPROCATE OPENSSL ARES DB4CXX PTHREAD POPT

CODE_SUBDIRS = monkeys stateAgents
TARGET_LIBRARY = librepro

ifeq ($(USE_SSL), true)

TESTPROGRAMS = \
	repro.cxx \
	userAdmin.cxx 

SRC =   \
	CommandLineParser.cxx \
	HttpBase.cxx \
	HttpConnection.cxx \
	\
	Proxy.cxx \
	Registrar.cxx \
	ReproServerAuthManager.cxx \
	RequestContext.cxx \
	ResponseContext.cxx \
	RouteDbMemory.cxx \
	UserAbstractDb.cxx \
	UserDb.cxx \
	WebAdmin.cxx \
	RouteAbstractDb.cxx \
	WebAdminThread.cxx \
        RequestProcessor.cxx \
        RequestProcessorChain.cxx \
	\
        monkeys/DigestAuthenticator.cxx \
	monkeys/RouteProcessor.cxx \
	monkeys/AmIResponsible.cxx \
	monkeys/LocationServer.cxx \
	monkeys/ConstantLocationMonkey.cxx \
	monkeys/RouteMonkey.cxx \

SRC +=	stateAgents/CertServer.cxx \
	stateAgents/CertPublicationHandler.cxx \
	stateAgents/CertSubscriptionHandler.cxx \
	stateAgents/PrivateKeyPublicationHandler.cxx \
	stateAgents/PrivateKeySubscriptionHandler.cxx
endif


include $(BUILD)/Makefile.post


