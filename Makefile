# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIPROCATE ARES PTHREAD OPENSSL

CODE_SUBDIRS = os external
TARGET_LIBRARY = libresiprocate
TESTPROGRAMS =

SRC = \
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
	os/SelectInterruptor.cxx \
	os/SHA1Stream.cxx \
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
	RequestLine.cxx \
	StatusLine.cxx \
	StringCategory.cxx \
	Token.cxx \
	Via.cxx \
	WarningCategory.cxx \
	\
	Aor.cxx \
	ApiCheck.cxx \
	ApplicationSip.cxx \
	BranchParameter.cxx \
	Connection.cxx \
	ConnectionBase.cxx \
	ConnectionManager.cxx \
	Contents.cxx \
	CpimContents.cxx \
	DataParameter.cxx \
	DeprecatedDialog.cxx \
	DnsInterface.cxx \
	DnsResult.cxx \
	DtlsTransport.cxx \
	Embedded.cxx \
	ExtensionParameter.cxx \
	ExtensionHeader.cxx \
	ExistsParameter.cxx \
	ExternalBodyContents.cxx \
	ExternalDnsFactory.cxx \
	FloatParameter.cxx \
	GenericContents.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	HeaderHash.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	InternalTransport.cxx \
	LazyParser.cxx \
	LazyParser.cxx \
	Message.cxx \
	MessageWaitingContents.cxx \
	MethodHash.cxx \
	MethodTypes.cxx \
	MsgHeaderScanner.cxx \
	MultipartAlternativeContents.cxx \
	MultipartMixedContents.cxx \
	MultipartRelatedContents.cxx \
	MultipartSignedContents.cxx \
	OctetContents.cxx \
	Parameter.cxx \
	ParameterHash.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
	Pidf.cxx \
	Pkcs7Contents.cxx \
	Pkcs8Contents.cxx \
	PlainContents.cxx \
	QopParameter.cxx \
	QopParameter.cxx \
	QuotedDataParameter.cxx \
	QuotedDataParameter.cxx \
	RAckCategory.cxx \
	Registration.cxx \
	Rlmi.cxx \
	RportParameter.cxx \
	SdpContents.cxx \
	Security.cxx \
	SecurityAttributes.cxx \
	SipFrag.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
	StatelessHandler.cxx \
	StatisticsManager.cxx \
	StatisticsMessage.cxx \
	Symbols.cxx \
	TcpBaseTransport.cxx \
	TcpConnection.cxx \
	TcpTransport.cxx \
	TimeAccumulate.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TlsConnection.cxx \
	TlsTransport.cxx \
	TlsTransport.cxx \
	TransactionController.cxx \
	MessageFilterRule.cxx \
	TransactionUser.cxx \
	TransactionUserMessage.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportMessage.cxx \
	TransportSelector.cxx \
	TuIM.cxx \
	TuSelector.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \
	X509Contents.cxx \
	XMLCursor.cxx \
	KeepAliveMessage.cxx \
	\
	\
	external/HttpProvider.cxx \
	external/HttpGetMessage.cxx

SUFFIXES += .gperf .cxx
GPERFOPTS = -D --enum -E -L C++ -t -k '*' --compare-strncmp
#GPERFVER="GNU gperf 2.7.2"

# rule for case sensitive sorts of hash
MethodHash.cxx: MethodHash.gperf
	gperf $(GPERFOPTS) -Z `echo MethodHash | sed -e 's/.*\///'` $< >  $@

# rule for insensitive clods
#${SRC}: ${@:%.cxx=%.gperf} -- more portable?
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
