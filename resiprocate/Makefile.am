SUBDIRS = test
CXX = ${DISTCC} @CXX@
LIBTOOL = @LIBTOOL@ --tag=CXX
INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I/usr/kerberos/include
EXTRA_DIST = PreparseInlines.cxx PreparseDiagnostic.cxx config.hxx
lib_LTLIBRARIES = libresiprocate.la
libresiprocate_la_SOURCES = \
	os/BaseException.cxx \
	os/Coders.cxx \
	os/Condition.cxx \
	os/CountStream.cxx \
	os/Data.cxx \
	os/DataStream.cxx \
	os/Lock.cxx \
	os/Log.cxx \
	os/Logger.cxx \
	os/MD5Stream.cxx \
	os/Mutex.cxx \
	os/ParseBuffer.cxx \
	os/RWMutex.cxx \
	os/Random.cxx \
	os/RecursiveMutex.cxx \
	os/Socket.cxx \
	os/Subsystem.cxx \
	os/ThreadIf.cxx \
	os/Timer.cxx \
	os/vmd5.cxx \
	ApplicationSip.cxx \
	BranchParameter.cxx \
	Connection.cxx \
	ConnectionMap.cxx \
	Contents.cxx \
	DataParameter.cxx \
	Dialog.cxx \
	Dialog2.cxx \
	DialogSet.cxx \
	DnsResolver.cxx \
	Embedded.cxx \
	Executive.cxx \
	ExistsParameter.cxx \
	FloatParameter.cxx \
	GenericContents.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	HeaderHash.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	LazyParser.cxx \
	Message.cxx \
	MessageWaitingContents.cxx \
	MethodHash.cxx \
	MethodTypes.cxx \
	MultipartMixedContents.cxx \
	MultipartSignedContents.cxx \
	OctetContents.cxx \
	Parameter.cxx \
	ParameterHash.cxx \
	ParameterTypes.cxx \
	ParserCategories.cxx \
	ParserCategory.cxx \
	Pidf.cxx \
	Pkcs7Contents.cxx \
	PlainContents.cxx \
	Preparse.cxx \
	QopParameter.cxx \
	QuotedDataParameter.cxx \
	Registration.cxx \
	RportParameter.cxx \
	SdpContents.cxx \
	Security.cxx \
	SipFrag.cxx \
	SipMessage.cxx \
	SipSession.cxx	\
	SipStack.cxx \
	StatelessHandler.cxx \
	Subscription.cxx \
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
	UnknownHeaderType.cxx \
	UnknownParameter.cxx \
	UnknownParameterType.cxx \
	Uri.cxx \
	XMLCursor.cxx

resiprocate_includedir=$(includedir)/resiprocate
resiprocate_include_HEADERS = \
	ApplicationSip.hxx \
	BranchParameter.hxx \
	Connection.hxx \
	ConnectionMap.hxx \
	Contents.hxx \
	DataParameter.hxx \
	Dialog.hxx \
	Dialog2.hxx \
	DialogSet.hxx \
	DnsResolver.hxx \
	Embedded.hxx \
	Executive.hxx \
	ExistsParameter.hxx \
	FloatParameter.hxx \
	GenericContents.hxx \
	HeaderFieldValue.hxx \
	HeaderFieldValueList.hxx \
	HeaderHash.hxx \
	HeaderTypes.hxx \
	Headers.hxx \
	Helper.hxx \
	IntegerParameter.hxx \
	LazyParser.hxx \
	Message.hxx \
	MessageWaitingContents.hxx \
	MethodHash.hxx \
	MethodTypes.hxx \
	MultipartMixedContents.hxx \
	MultipartSignedContents.hxx \
	OctetContents.hxx \
	Parameter.hxx \
	ParameterHash.hxx \
	ParameterTypeEnums.hxx \
	ParameterTypes.hxx \
	ParseException.hxx \
	ParserCategories.hxx \
	ParserCategory.hxx \
	ParserContainer.hxx \
	ParserContainerBase.hxx \
	Pidf.hxx \
	Pkcs7Contents.hxx \
	PlainContents.hxx \
	Preparse.hxx \
	QopParameter.hxx \
	QuotedDataParameter.hxx \
	Registration.hxx \
	ReliabilityMessage.hxx \
	RportParameter.hxx \
	SdpContents.hxx \
	Security.hxx \
	SendingMessage.hxx \
	SipFrag.hxx \
	SipMessage.hxx \
	SipMessageExplicit.hxx \
	SipSession.hxx \
	SipStack.hxx \
	StatelessHandler.hxx \
	Subscription.hxx \
	Symbols.hxx \
	TcpTransport.hxx \
	TimerMessage.hxx \
	TimerQueue.hxx \
	TlsTransport.hxx \
	TransactionMap.hxx \
	TransactionState.hxx \
	TransactionTerminated.hxx \
	Transport.hxx \
	TransportMessage.hxx \
	TransportSelector.hxx \
	TuIM.hxx \
	TuShim.hxx \
	TuUa.hxx \
	UdpTransport.hxx \
	UnknownHeaderType.hxx \
	UnknownParameter.hxx \
	UnknownParameterType.hxx \
	Uri.hxx \
	XMLCursor.hxx \
	XPidf.hxx \
	supported.hxx

resiprocate_os_includedir=$(includedir)/resiprocate/os
resiprocate_os_include_HEADERS = \
	os/BaseException.hxx \
	os/CircularBuffer.hxx \
	os/Coders.hxx \
	os/Condition.hxx \
	os/CountStream.hxx \
	os/Data.hxx \
	os/DataStream.hxx \
	os/Fifo.hxx \
	os/HashMap.hxx \
	os/Inserter.hxx \
	os/Lock.hxx \
	os/Lockable.hxx \
	os/Log.hxx \
	os/Logger.hxx \
	os/MD5Stream.hxx \
	os/Mutex.hxx \
	os/ParseBuffer.hxx \
	os/RWMutex.hxx \
	os/Random.hxx \
	os/RandomHex.hxx \
	os/RecursiveMutex.hxx \
	os/Socket.hxx \
	os/Subsystem.hxx \
	os/SysLogBuf.hxx \
	os/SysLogStream.hxx \
	os/ThreadIf.hxx \
	os/Timer.hxx \
	os/compat.hxx \
	os/vmd5.hxx \
	os/vthread.hxx

SUFFIXES = .gperf .cxx
GPERFFLAGS = -D --enum -E -L C++ -t -k '*' --compare-strncmp
.gperf.cxx:
	gperf $(GPERFFLAGS) -Z `echo $* | sed -e 's/.*\///'` $< | \
	sed -e 's/str\[\([0-9][0-9]*\)\]/tolower(str[\1])/g' | \
	sed -e 's/^\([	]*\)if *(\*\([a-z][a-z]*\) *== *\*\([a-z][a-z]*\) *\&\& *!strncmp *(\([^)]*\)).*/\1if (tolower(*\2) == *\3 \&\& !strncasecmp( \4 ))/g' > $@
