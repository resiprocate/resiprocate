# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE ARES PTHREAD OPENSSL

CODE_SUBDIRS = os
TARGET_LIBRARY = libresiprocate
TESTPROGRAMS =

CXXFLAGS += -I/sw/include $(DTLS_INCLUDES)
LDFLAGS  += -L/sw/lib

SRC = \
	\
	os/HeapInstanceCounter.cxx \
	\
	os/AbstractFifo.cxx \
	os/BaseException.cxx \
	os/Coders.cxx \
	os/Condition.cxx \
	os/CountStream.cxx \
	os/Data.cxx \
	os/DataStream.cxx \
	os/DnsUtil.cxx \
	os/FileSystem.cxx \
	os/Lock.cxx \
	os/Log.cxx \
	os/Logger.cxx \
	os/MD5Stream.cxx \
	os/Mutex.cxx \
	os/Poll.cxx \
	os/RecursiveMutex.cxx \
	os/ParseBuffer.cxx \
	os/RWMutex.cxx \
	os/Random.cxx \
	os/Socket.cxx \
	os/Subsystem.cxx \
	os/SysLogBuf.cxx \
	os/SysLogStream.cxx \
	os/ThreadIf.cxx \
	os/Timer.cxx \
	os/Tuple.cxx \
	os/vmd5.cxx \
	os/WinCompat.cxx \
	\
	\
	AresDns.cxx \
	Auth.cxx \
	CSeqCategory.cxx \
	CallId.cxx \
	DateCategory.cxx \
	ExpiresCategory.cxx \
	GenericUri.cxx \
	IntegerCategory.cxx \
	Mime.cxx \
	NameAddr.cxx \
	RAckCategory.cxx \
	RequestLine.cxx \
	StatusLine.cxx \
	StringCategory.cxx \
	Token.cxx \
	Via.cxx \
	WarningCategory.cxx \
	\
	Rlmi.cxx \
	GenericContents.cxx \
	XMLCursor.cxx \
	UnknownHeaderType.cxx \
	UnknownParameterType.cxx \
	Embedded.cxx \
	Pidf.cxx \
	MultipartSignedContents.cxx \
	LazyParser.cxx \
	BranchParameter.cxx \
	Connection.cxx \
	Contents.cxx \
	DataParameter.cxx \
	Dialog.cxx \
	DnsInterface.cxx \
	DnsResult.cxx \
	DtlsTransport.cxx \
	ExistsParameter.cxx \
	ExternalBodyContents.cxx \
	ExternalDnsFactory.cxx \
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
	MultipartAlternativeContents.cxx \
	MultipartRelatedContents.cxx \
	OctetContents.cxx \
	Parameter.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
	Pkcs7Contents.cxx \
	Pkcs8Contents.cxx \
	X509Contents.cxx \
	PlainContents.cxx \
	CpimContents.cxx \
	MsgHeaderScanner.cxx \
	QopParameter.cxx \
	QopParameter.cxx \
	QuotedDataParameter.cxx \
	QuotedDataParameter.cxx \
	Registration.cxx \
	RportParameter.cxx \
	SdpContents.cxx \
	Security.cxx \
	SecurityAttributes.cxx \
	SipFrag.cxx \
	ApplicationSip.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
	StatelessHandler.cxx \
	StatisticsManager.cxx \
	StatisticsMessage.cxx \
	Symbols.cxx \
	ConnectionManager.cxx \
	TcpConnection.cxx \
	TlsConnection.cxx \
	TcpBaseTransport.cxx \
	TcpTransport.cxx \
	TimeAccumulate.cxx \
	TlsTransport.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TlsTransport.cxx \
	TransactionController.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportMessage.cxx \
	TransportSelector.cxx \
	TuIM.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Aor.cxx \
	Uri.cxx \
	HeaderHash.cxx \
	ParameterHash.cxx \
	ApiCheck.cxx \
	MethodHash.cxx

SUFFIXES += .gperf .cxx
GPERFOPTS = -D --enum -E -L C++ -t -k '*' --compare-strncmp
#GPERFVER="GNU gperf 2.7.2"

# rule for case sensitive sorts of hash
MethodHash.cxx: MethodHash.gperf
	gperf $(GPERFOPTS) -Z `echo MethodHash | sed -e 's/.*\///'` $< >  $@

# rule for insensitive clods
%.cxx: %.gperf
	gperf $(GPERFOPTS) -Z `echo $* | sed -e 's/.*\///'` $< | \
	sed -e 's/str\[\([0-9][0-9]*\)\]/tolower(str[\1])/g' | \
	sed -e 's/^\([	]*\)if *(\*\([a-z][a-z]*\) *== *\*\([a-z][a-z]*\) *\&\& *!strncmp *(\([^)]*\)).*/\1if (tolower(*\2) == *\3 \&\& !strncasecmp( \4 ))/g' | \
	sed -e 's/\*str ==/tolower(*str) ==/' | \
	sed -e 's/\!strncmp/\!strncasecmp/'  > $@

INSTALL_ROOT=/usr/local

install: all
	install -d --mode=755 $(INSTALL_ROOT)
	install -d --mode=755 $(INSTALL_ROOT)/lib
	-install --mode=755 lib.$(TARGET_NAME)/libresiprocate.so $(INSTALL_ROOT)/lib
	-install --mode=755 lib.$(TARGET_NAME)/libresiprocate.a $(INSTALL_ROOT)/lib
	install -d --mode=755 $(INSTALL_ROOT)/include
	-install --mode=755 resiprocate/*.h $(INSTALL_ROOT)/include
	install --mode=755 resiprocate/*.hxx $(INSTALL_ROOT)/include
	install -d --mode=755 $(INSTALL_ROOT)/include/os
	-install --mode=755 resiprocate/os/*.h $(INSTALL_ROOT)/include/os
	install --mode=755 resiprocate/os/*.hxx $(INSTALL_ROOT)/include/os

include $(BUILD)/Makefile.post
