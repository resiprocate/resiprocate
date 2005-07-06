SUBDIRS = .  dum test
CXX = ${DISTCC} @CXX@
dnl INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I/usr/kerberos/include
INCLUDES = -I$(top_srcdir) -I$(top_builddir)
# EXTRA_DIST = resip.spec.in

LIBTOOL = @LIBTOOL@ --tag=CXX

lib_LTLIBRARIES = libresiprocate.la
libresiprocate_la_SOURCES = \
	os/AbstractFifo.cxx \
	os/BaseException.cxx \
	os/Coders.cxx \
	os/Condition.cxx \
	os/CountStream.cxx \
	os/Data.cxx \
	os/DataStream.cxx \
	os/DnsUtil.cxx \
	os/Id.cxx \
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
	os/SysLogBuf.cxx \
	os/SysLogStream.cxx \
	os/ThreadIf.cxx \
	os/Timer.cxx \
	os/Tuple.cxx \
	os/vmd5.cxx \
	os/WinCompat.cxx \
	AresDns.cxx \
	ApiCheck.cxx \
	BranchParameter.cxx \
	Connection.cxx \
	ConnectionManager.cxx \
	DataParameter.cxx \
	Dialog.cxx \
	DnsInterface.cxx \
	DnsResult.cxx \
	Embedded.cxx \
	ExistsParameter.cxx \
	ExternalDnsFactory.cxx \
	FloatParameter.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	HeaderHash.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	LazyParser.cxx \
	Message.cxx \
	MethodHash.cxx \
	MethodTypes.cxx \
	MsgHeaderScanner.cxx \
	Parameter.cxx \
	ParameterHash.cxx \
	ParameterTypes.cxx \
	ParserCategories.cxx \
	\
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
	ParserCategory.cxx \
	Pidf.cxx \
	ParseUtil.cxx \
	Rlmi.cxx \
	QopParameter.cxx \
	QuotedDataParameter.cxx \
	Registration.cxx \
	RportParameter.cxx \
	Security.cxx \
	SecurityAttributes.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
	StatisticsMessage.cxx \
	StatisticsManager.cxx \
	StatelessHandler.cxx \
	Subscription.cxx \
	Symbols.cxx \
	TcpBaseTransport.cxx \
	TcpConnection.cxx \
	TcpTransport.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TlsConnection.cxx \
	TlsTransport.cxx \
	TransactionController.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportMessage.cxx \
	TransportSelector.cxx \
	UdpTransport.cxx \
	UnknownHeaderType.cxx \
	UnknownParameter.cxx \
	UnknownParameterType.cxx \
	Uri.cxx \
	XMLCursor.cxx 

# These are sources that we might conditionally include
EXTRA_libresiprocate_la_SOURCES = \
	ApplicationSip.cxx \
	CpimContents.cxx \
	GenericContents.cxx \
	MessageWaitingContents.cxx \
	MultipartMixedContents.cxx \
	MultipartSignedContents.cxx \
	OctetContents.cxx \
	Pkcs7Contents.cxx \
	PlainContents.cxx \
	SdpContents.cxx \
	SipFrag.cxx \
	TuIM.cxx


libresiprocate_la_LIBADD = @CONTENTS_OBJS@

libresiprocate_la_DEPENDENCIES = @CONTENTS_OBJS@

libresiprocate_la_LDFLAGS = -version-info $(LIBRESIP_SO_VERSION)

resiprocate_includedir=$(includedir)/resiprocate
resiprocate_include_HEADERS = \
    ApplicationSip.hxx \
	AresDns.hxx \
	Auth.hxx \
	BranchParameter.hxx \
	CSeqCategory.hxx \
	CallId.hxx \
	Connection.hxx \
	ConnectionManager.hxx \
	Contents.hxx \
	CpimContents.hxx \
	DataParameter.hxx \
	DateCategory.hxx \
	Dialog.hxx \
	DnsInterface.hxx \
	DnsResult.hxx \
	DnsHandler.hxx \
	Embedded.hxx \
	ExistsParameter.hxx \
	ExpiresCategory.hxx \
	ExternalDnsFactory.hxx \
	FloatParameter.hxx \
	GenericContents.hxx \
	GenericUri.hxx \
	HeaderFieldValue.hxx \
	HeaderFieldValueList.hxx \
	HeaderHash.hxx \
	HeaderTypes.hxx \
	Headers.hxx \
	Helper.hxx \
	IntegerCategory.hxx \
	IntegerParameter.hxx \
	LazyParser.hxx \
	Message.hxx \
	MessageWaitingContents.hxx \
	MethodHash.hxx \
	MethodTypes.hxx \
	Mime.hxx \
	MsgHeaderScanner.hxx \
	MultipartMixedContents.hxx \
	MultipartSignedContents.hxx \
	NameAddr.hxx \
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
	ParseUtil.hxx \
	Pidf.hxx \
	Pkcs7Contents.hxx \
	PlainContents.hxx \
	QopParameter.hxx \
	QuotedDataParameter.hxx \
	RAckCategory.hxx \
	Registration.hxx \
	RequestLine.hxx \
	Rlmi.hxx \
	RportParameter.hxx \
	SdpContents.hxx \
	Security.hxx \
	SecurityAttributes.hxx \
	SecurityTypes.hxx \
	ShutdownMessage.hxx \
	SipFrag.hxx \
	SipMessage.hxx \
	SipMessageExplicit.hxx \
	SipStack.hxx \
	StackThread.hxx \
	StatelessHandler.hxx \
	StatisticsManager.hxx \
	StatisticsMessage.hxx \
	StatusLine.hxx \
	StringCategory.hxx \
	Subscription.hxx \
	Symbols.hxx \
	TcpTransport.hxx \
	TcpBaseTransport.hxx \
	TcpConnection.hxx \
	TimerMessage.hxx \
	TimerQueue.hxx \
	TlsConnection.hxx \
	TlsTransport.hxx \
	Token.hxx \
	TransactionController.hxx \
	TransactionMap.hxx \
	TransportMessage.hxx \
	TransactionMessage.hxx \
	TransactionState.hxx \
	TransactionTerminated.hxx \
	Transport.hxx \
	TransportMessage.hxx \
	TransportSelector.hxx \
	TuIM.hxx \
	UdpTransport.hxx \
	UnknownHeaderType.hxx \
	UnknownParameter.hxx \
	UnknownParameterType.hxx \
	Uri.hxx \
	Via.hxx \
	WarningCategory.hxx \
	XMLCursor.hxx \
	X_msMsgsInvite.hxx \
	config.hxx

resiprocate_os_includedir=$(includedir)/resiprocate/os
resiprocate_os_include_HEADERS = \
	os/BaseException.hxx \
	os/CircularBuffer.hxx \
	os/Coders.hxx \
	os/Condition.hxx \
	os/CountStream.hxx \
	os/Data.hxx \
	os/DataStream.hxx \
	os/DnsUtil.hxx \
	os/Fifo.hxx \
	os/HashMap.hxx \
	os/Id.hxx \
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
	os/RecursiveMutex.hxx \
	os/Socket.hxx \
	os/Subsystem.hxx \
	os/SysLogBuf.hxx \
	os/SysLogStream.hxx \
	os/ThreadIf.hxx \
	os/Timer.hxx \
	os/TransportType.hxx \
	os/Tuple.hxx \
	os/compat.hxx \
	os/vmd5.hxx \
	os/vthread.hxx \
	os/WinCompat.hxx 



#SUFFIXES = .gperf .cxx
#GPERFFLAGS = -D --enum -E -L C++ -t -k '*' --compare-strncmp
#.gperf.cxx:
#	gperf $(GPERFFLAGS) -Z `echo $* | sed -e 's/.*\///'` $< | \
#	sed -e 's/str\[\([0-9][0-9]*\)\]/tolower(str[\1])/g' | \
#	sed -e 's/^\([	]*\)if *(\*\([a-z][a-z]*\) *== *\*\([a-z][a-z]*\) *\&\& *!strncmp *(\([^)]*\)).*/\1if (tolower(*\2) == *\3 \&\& !strncasecmp( \4 ))/g' > $@

