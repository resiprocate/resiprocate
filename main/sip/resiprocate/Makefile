# $Id: Makefile,v 1.62 2002/11/04 06:33:51 derekm Exp $

BUILD = ../../build

include $(BUILD)/Makefile.pre

USE_REPO=false
PACKAGES += UTIL2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TARGET_LIBRARY = libsip2

TESTPROGRAMS =  test2.cxx testSipStack1.cxx testSipMessage.cxx testParserCategories.cxx testNonInviteServerTx.cxx testNonInviteClientTx.cxx testDnsResolver.cxx
#testPreparse.cxx

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


SRC = \
	ConnectionMap.cxx \
	DataParameter.cxx \
	Dialog.cxx \
	Executive.cxx \
	ExistsParameter.cxx \
	FloatParameter.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	Message.cxx \
	MethodTypes.cxx \
	Parameter.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
	ParserCategories.cxx \
	Preparse.cxx \
	Resolver.cxx \
	SendingMessage.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	Symbols.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportSelector.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \

include $(BUILD)/Makefile.post
