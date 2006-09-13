# $Id: Makefile,v 1.64 2005/06/16 17:19:26 satluri Exp $
BUILD = ../build

include $(BUILD)/Makefile.pre

CODE_SUBDIRS = predicates repro

PACKAGES += 	REPRO DUM RESIP RUTIL TFMLIBS OPENSSL ARES DB4CXX PTHREAD POPT 
ifeq ($(OSTYPE),MinGW)
PACKAGES += PCRE
endif

TESTPROGRAMS = 	sanityTests.cxx 
TARGET_LIBRARY = libtfm

CXXFLAGS += -I.. -Icontrib -I/usr/local/include/boost-1_33 -I/usr/local/include/boost-1_33_1

SRC = ActionBase.cxx \
	AsciiGraphic.cxx \
	Box.cxx \
	CommandLineParser.cxx \
	CheckContacts.cxx\
	CheckFetchedContacts.cxx \
	CountDown.cxx \
	DialogSet.cxx \
	DnsUtils.cxx \
	Event.cxx \
	ExpectAction.cxx \
	ExpectActionEvent.cxx \
	Fixture.cxx \
	Renderer.cxx \
	Resolver.cxx \
	RouteGuard.cxx \
	Sequence.cxx \
	SequenceSet.cxx \
	SequenceSetThread.cxx \
	SipEvent.cxx \
	SipRawMessage.cxx \
	Source.cxx \
	TelnetClient.cxx \
	TestEndPoint.cxx \
	TestException.cxx \
	TestProxy.cxx \
	TestSipEndPoint.cxx \
	TestUser.cxx \
	TransportDriver.cxx \
	\
	predicates/ExpectUtils.cxx \
	predicates/NotifyContents.cxx \
	predicates/SubscribeExpired.cxx \
	predicates/SubscribeExpires.cxx \
	\
	repro/TestRepro.cxx \
	repro/TestReproUser.cxx

include $(BUILD)/Makefile.post


