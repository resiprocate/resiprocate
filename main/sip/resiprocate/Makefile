# $Id: Makefile,v 1.127 2003/09/14 01:41:13 davidb Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE  ARES OPENSSL PTHREAD 

CODE_SUBDIRS = os
TARGET_LIBRARY = libresiprocate
TESTPROGRAMS =  

CXXFLAGS += -I/usr/local/ssl/include
LDFLAGS  += -L/usr/local/ssl/lib

#	XPidf.cxx \

SRC = \
	libSipImp.cxx \
	os/BaseException.cxx \
	os/Coders.cxx \
	os/Condition.cxx \
	os/CountStream.cxx \
	os/Data.cxx \
	os/DataStream.cxx \
	os/DnsUtil.cxx \
	os/Lock.cxx \
	os/Log.cxx \
	os/Logger.cxx \
	os/MD5Stream.cxx \
	os/Mutex.cxx \
	os/RecursiveMutex.cxx \
	os/ParseBuffer.cxx \
	os/RWMutex.cxx \
	os/Random.cxx \
	os/Socket.cxx \
	os/Subsystem.cxx \
	os/ThreadIf.cxx \
	os/Timer.cxx \
	os/vmd5.cxx \
	X_msMsgsInvite.cxx \
	GenericContents.cxx \
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
	DnsInterface.cxx \
	DnsResult.cxx \
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
	OctetContents.cxx \
	Parameter.cxx \
	ParameterTypes.cxx \
	ParserCategories.cxx \
	ParserCategory.cxx \
	Pkcs7Contents.cxx \
	PlainContents.cxx \
	Preparse.cxx \
	MsgHeaderScanner.cxx \
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
	StatelessHandler.cxx \
	Symbols.cxx \
	TcpTransport.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TlsTransport.cxx \
	TransactionController.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportSelector.cxx \
	TuIM.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \
	ParseUtil.cxx \
	HeaderHash.cxx \
	ParameterHash.cxx \
	MethodHash.cxx

GPERFOPTS=-D --enum -E -L C++ -t -k '*' --compare-strncmp
GPERFVER="GNU gperf 2.7.2"

%-raw.cxx: %.gperf
	@[ "$$(gperf -v )" == $(GPERFVER) ] || \
		(echo Bogus gperf need:;\
		 echo $(GPERFVER), have: ; gperf -v ; false)
	gperf $(GPERFOPTS) -Z $(*:%-raw=%) $< > $@

# Exceptions (case sensitive)
MethodHash.cxx: MethodHash-raw.cxx
	os/fixupGperf $< -o $@
	\rm MethodHash-raw.cxx

# The rest of the hashes.
%.cxx: %-raw.cxx
	os/fixupGperf $< -o $@ --ignorecase

include $(BUILD)/Makefile.post
