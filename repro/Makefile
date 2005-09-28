# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

# repro is not GPL so if you want to link in MySQL, you needs a 
# a comercial license for MySQL that is not GPL 
USE_MYSQL = false

BUILD = ../build
include $(BUILD)/Makefile.pre

CXXFLAGS += -I../
PACKAGES += DUM RESIP RUTIL OPENSSL ARES DB4CXX PTHREAD POPT

ifeq ($(USE_MYSQL),yes)
PACKAGES += MYSQL
SRC += MySqlDb.cxx
endif

CODE_SUBDIRS = monkeys stateAgents
TARGET_LIBRARY = librepro


TESTPROGRAMS = \
	repro.cxx \
#	genUsers.cxx \
#	userAdmin.cxx 

SRC 	+=   \
	RouteStore.cxx \
	UserStore.cxx \
	ConfigStore.cxx \
	AclStore.cxx \
	Store.cxx \
	AbstractDb.cxx \
	BerkeleyDb.cxx \
	\
	CommandLineParser.cxx \
	HttpBase.cxx \
	HttpConnection.cxx \
	WebAdmin.cxx \
	WebAdminThread.cxx \
	\
	Proxy.cxx \
	Registrar.cxx \
	ReproServerAuthManager.cxx \
	RequestContext.cxx \
	ResponseContext.cxx \
        Processor.cxx \
        ProcessorChain.cxx \
	\
        monkeys/DigestAuthenticator.cxx \
	monkeys/StrictRouteFixup.cxx \
	monkeys/AmIResponsible.cxx \
	monkeys/IsTrustedNode.cxx \
	monkeys/ConstantLocationMonkey.cxx \
	monkeys/LocationServer.cxx \
	monkeys/RecursiveRedirect.cxx \
	monkeys/SimpleStaticRoute.cxx \
	monkeys/StaticRoute.cxx \
	monkeys/StrictRouteFixup.cxx \
	\


ifeq ($(USE_SSL), yes)
SRC +=	stateAgents/CertServer.cxx \
	stateAgents/CertPublicationHandler.cxx \
	stateAgents/CertSubscriptionHandler.cxx \
	stateAgents/PrivateKeyPublicationHandler.cxx \
	stateAgents/PrivateKeySubscriptionHandler.cxx
endif


include $(BUILD)/Makefile.post


