# $Id: Makefile,v 1.69 2002/11/05 21:35:21 jason Exp $

BUILD = ../../build

include $(BUILD)/Makefile.pre

USE_REPO=false
PACKAGES += UTIL2 PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
endif

TARGET_LIBRARY = libsip2
TESTPROGRAMS =  SipTortureTests.cxx test2.cxx testSipStack1.cxx testSipMessage.cxx testParserCategories.cxx testNonInviteServerTx.cxx testNonInviteClientTx.cxx testDnsResolver.cxx testPreparse.cxx



ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif


SRC = \
	BranchParameter.cxx \
	TestTransport.cxx \
	ConnectionMap.cxx \
	DataParameter.cxx \
	Dialog.cxx \
	DnsMessage.cxx \
	DnsResolver.cxx \
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
	DnsResolver.cxx \
	SendingMessage.cxx \
	SipMessage.cxx \
	DnsMessage.cxx \
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
