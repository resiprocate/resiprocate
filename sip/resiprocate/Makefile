# $Id: Makefile,v 1.111 2003/03/17 21:30:58 davidb Exp $

BUILD = ../../build

USE_SSL = 1
VOCAL_USE_DEPRECATED=0

include $(BUILD)/Makefile.pre
-include ../Makefile.opt

USE_REPO=false

PACKAGES += SIP2 UTIL2 ARES PTHREAD

ifeq ($(USE_SSL),1)
PACKAGES += OPENSSL
CFLAGS += -DUSE_SSL
CXXFLAGS += -DUSE_SSL
LDFLAGS += -lssl -lcrypto
endif

#CXXFLAGS += -DVOCAL2_AOR_HAS_SCHEME

TARGET_LIBRARY = libsip2
TESTPROGRAMS =  

parsetest: bin.debug.Linux.i686/testParserCategories
	./bin.debug.Linux.i686/testParserCategories

#	TuShim.cxx \

SRC = \
	SipSession.cxx	\
	Registration.cxx \
	Subscription.cxx \
	DialogSet.cxx \
	Dialog2.cxx \
	XMLCursor.cxx \
	UnknownHeaderType.cxx \
	UnknownParameterType.cxx \
	Embedded.cxx \
	Pidf.cxx \
	MultipartSignedContents.cxx \
	LazyParser.cxx \
	BranchParameter.cxx \
	Connection.cxx \
	ConnectionMap.cxx \
	Contents.cxx \
	DataParameter.cxx \
	Dialog.cxx \
	DnsResolver.cxx \
	Executive.cxx \
	ExistsParameter.cxx \
	FloatParameter.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	LazyParser.cxx \
	Message.cxx \
	MessageWaitingContents.cxx \
	MethodTypes.cxx \
	MultipartMixedContents.cxx \
	Parameter.cxx \
	ParameterTypes.cxx \
	ParserCategories.cxx \
	ParserCategory.cxx \
	Pkcs7Contents.cxx \
	PlainContents.cxx \
	Preparse.cxx \
	QopParameter.cxx \
	QopParameter.cxx \
	QuotedDataParameter.cxx \
	QuotedDataParameter.cxx \
	RportParameter.cxx \
	SdpContents.cxx \
	Security.cxx \
	SipFrag.cxx \
	ApplicationSip.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	Symbols.cxx \
	TcpTransport.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TlsTransport.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportSelector.cxx \
	TuIM.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \
	HeaderHash.cxx \
	ParameterHash.cxx \
	MethodHash.cxx 

ifeq ($(HASHES),1)
GPERFOPTS=-D --enum -E -L C++ -t -k '*' --compare-strncmp
GPERFVER="GNU gperf 2.7.2"

%-raw.cxx: %.gperf
	@[ "$$(gperf -v )" == $(GPERFVER) ] || \
		(echo Bogus gperf need:;\
		 echo $(GPERFVER), have: ; gperf -v ; false)
	gperf $(GPERFOPTS) -Z $(*:%-raw=%) $< > $@

# Exceptions (case sensitive)
MethodHash.cxx: MethodHash-raw.cxx
	../util/fixupGperf $< -o $@
	\rm MethodHash-raw.cxx

# The rest of the hashes.
%.cxx: %-raw.cxx
	../util/fixupGperf $< -o $@ --ignorecase
endif

include $(BUILD)/Makefile.post
