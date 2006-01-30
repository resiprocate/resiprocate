SUBDIRS = .  dum test
CXX = ${DISTCC} @CXX@
# INCLUDES = -I$(top_srcdir) -I$(top_builddir) -I/usr/kerberos/include
INCLUDES = -I$(top_srcdir) -I$(top_builddir)

EXTRA_DIST = 	resiprocate_7_0.vcproj \
		resiprocate_7_1.vcproj \
		resiprocate_8_0.vcproj \
		doc Doxyfile FAQ Readme-Compliance.txt \
	        os/WinCompat.cxx 

LIBTOOL = @LIBTOOL@ --tag=CXX

lib_LTLIBRARIES = libresiprocate.la

# note that several source files have been
# delegated to #EXTRA_..._SOURCES, especially
# the *Contents* family

libresiprocate_la_SOURCES = \
        dns/DnsAAAARecord.cxx \
        dns/DnsCnameRecord.cxx \
        dns/DnsHostRecord.cxx \
        dns/DnsNaptrRecord.cxx \
        dns/DnsSrvRecord.cxx \
        dns/DnsStub.cxx \
        dns/QueryTypes.cxx \
        dns/RRCache.cxx \
        dns/RRList.cxx \
        dns/RROverlay.cxx \
        dns/RRVip.cxx \
	external/HttpGetMessage.cxx \
	external/HttpProvider.cxx \
	os/AbstractFifo.cxx \
	os/BaseException.cxx \
	os/Coders.cxx \
	os/Condition.cxx \
	os/CountStream.cxx \
	os/Data.cxx \
	os/DataStream.cxx \
	os/DnsUtil.cxx \
	os/FileSystem.cxx \
        os/FlowId.cxx \
	os/HeapInstanceCounter.cxx \
	os/Id.cxx \
	os/Lock.cxx \
	os/Log.cxx \
	os/Logger.cxx \
	os/MD5Stream.cxx \
	os/Mutex.cxx \
	os/ParseBuffer.cxx \
	os/Poll.cxx \
	os/RWMutex.cxx \
	os/Random.cxx \
	os/RecursiveMutex.cxx \
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
	Aor.cxx \
	ApiCheck.cxx \
	AresDns.cxx \
	Auth.cxx \
	BranchParameter.cxx \
	CSeqCategory.cxx \
	CallId.cxx \
	Connection.cxx \
	ConnectionBase.cxx \
	ConnectionManager.cxx \
	Contents.cxx \
	ContentsFactoryBase.cxx \
	DataParameter.cxx \
	DateCategory.cxx \
	DeprecatedDialog.cxx \
	DnsInterface.cxx \
	DnsResult.cxx \
	DtlsMessage.cxx \
	DtlsTransport.cxx \
	Embedded.cxx \
	ExistsParameter.cxx \
	ExpiresCategory.cxx \
	ExtensionHeader.cxx \
	ExtensionParameter.cxx \
	ExternalDnsFactory.cxx \
	FloatParameter.cxx \
	GenericUri.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	HeaderHash.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	IntegerCategory.cxx \
	InternalTransport.cxx \
	KeepAliveMessage.cxx \
	LazyParser.cxx \
	Message.cxx \
	MessageFilterRule.cxx \
	MethodHash.cxx \
	MethodTypes.cxx \
	Mime.cxx \
	MsgHeaderScanner.cxx \
	NameAddr.cxx \
	Parameter.cxx \
	ParameterHash.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
        ParserContainerBase.cxx \
	Pidf.cxx \
	QopParameter.cxx \
	QuotedDataParameter.cxx \
	RAckCategory.cxx \
        RequestLine.cxx \
	Rlmi.cxx \
	RportParameter.cxx \
	Security.cxx \
	SecurityAttributes.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
	StatelessHandler.cxx \
	StatisticsManager.cxx \
	StatisticsMessage.cxx \
        StatusLine.cxx \
        StringCategory.cxx \
	Symbols.cxx \
	TcpBaseTransport.cxx \
	TcpConnection.cxx \
	TcpTransport.cxx \
	TimeAccumulate.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TlsConnection.cxx \
	TlsTransport.cxx \
        Token.cxx \
	TransactionController.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	TransactionUser.cxx \
	TransactionUserMessage.cxx \
	Transport.cxx \
	TransportFailure.cxx \
	TransportSelector.cxx \
	TuSelector.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \
        Via.cxx \
        WarningCategory.cxx \
	XMLCursor.cxx 

# RjS - work in progress - many of these are now duplicated above

# These are sources that we might conditionally include
EXTRA_libresiprocate_la_SOURCES = \
	ApplicationSip.cxx \
	CpimContents.cxx \
	ExternalBodyContents.cxx \
	GenericContents.cxx \
	MessageWaitingContents.cxx \
	MultipartAlternativeContents.cxx \
	MultipartMixedContents.cxx \
	MultipartRelatedContents.cxx \
	MultipartSignedContents.cxx \
	OctetContents.cxx \
	Pkcs7Contents.cxx \
	Pkcs8Contents.cxx \
	PlainContents.cxx \
	SdpContents.cxx \
	SipFrag.cxx \
	TuIM.cxx \
	X509Contents.cxx 


libresiprocate_la_LIBADD = @CONTENTS_OBJS@

libresiprocate_la_DEPENDENCIES = @CONTENTS_OBJS@

libresiprocate_la_LDFLAGS = -version-info $(LIBRESIP_SO_VERSION)

resiprocate_includedir=$(includedir)/resiprocate
resiprocate_include_HEADERS = \
	Aor.hxx \
	ApiCheck.hxx \
	ApiCheckList.hxx \
	ApplicationMessage.hxx \
	ApplicationSip.hxx \
	AresDns.hxx \
	Auth.hxx \
	BranchParameter.hxx \
	CSeqCategory.hxx \
	CallId.hxx \
	Connection.hxx \
	ConnectionBase.hxx \
	ConnectionManager.hxx \
	Contents.hxx \
	ContentsFactory.hxx \
	ContentsFactoryBase.hxx \
	CpimContents.hxx \
	DataParameter.hxx \
	DateCategory.hxx \
	DeprecatedDialog.hxx \
	DnsInterface.hxx \
	DnsResult.hxx \
	DnsHandler.hxx \
	DtlsMessage.hxx \
	DtlsTransport.hxx \
	Embedded.hxx \
	ExistsParameter.hxx \
	ExpiresCategory.hxx \
	ExtensionHeader.hxx \
	ExtensionParameter.hxx \
	ExternalBodyContents.hxx \
	ExternalDnsFactory.hxx \
	FloatParameter.hxx \
	GenericContents.hxx \
	GenericIPAddress.hxx \
	GenericUri.hxx \
	HeaderFieldValue.hxx \
	HeaderFieldValueList.hxx \
	HeaderHash.hxx \
	HeaderTypes.hxx \
	Headers.hxx \
	Helper.hxx \
	IntegerCategory.hxx \
	IntegerParameter.hxx \
	InternalTransport.hxx \
	KeepAliveMessage.hxx \
	LazyParser.hxx \
	Message.hxx \
	MessageFilterRule.hxx \
	MessageWaitingContents.hxx \
	MethodHash.hxx \
	MethodTypes.hxx \
	Mime.hxx \
	MsgHeaderScanner.hxx \
	MultipartAlternativeContents.hxx \
	MultipartMixedContents.hxx \
	MultipartRelatedContents.hxx \
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
	Pidf.hxx \
	Pkcs7Contents.hxx \
	Pkcs8Contents.hxx \
	PlainContents.hxx \
	QopParameter.hxx \
	QuotedDataParameter.hxx \
	RAckCategory.hxx \
	RequestLine.hxx \
	Rlmi.hxx \
	RportParameter.hxx \
	SdpContents.hxx \
	Security.hxx \
	SecurityAttributes.hxx \
	SecurityTypes.hxx \
	SendData.hxx \
	ShutdownMessage.hxx \
	SipFrag.hxx \
	SipMessage.hxx \
	SipStack.hxx \
	StackThread.hxx \
	StatelessHandler.hxx \
	StatisticsManager.hxx \
	StatisticsMessage.hxx \
	StatusLine.hxx \
	StringCategory.hxx \
	Symbols.hxx \
	TcpBaseTransport.hxx \
	TcpConnection.hxx \
	TcpTransport.hxx \
	TimeAccumulate.hxx \
	TimerMessage.hxx \
	TimerQueue.hxx \
	TlsConnection.hxx \
	TlsTransport.hxx \
	Token.hxx \
	TransactionController.hxx \
	TransactionMap.hxx \
	TransactionMessage.hxx \
	TransactionState.hxx \
	TransactionTerminated.hxx \
	TransactionUser.hxx \
	TransactionUserMessage.hxx \
	Transport.hxx \
	TransportFailure.hxx \
	TransportSelector.hxx \
	TuIM.hxx \
	TuSelector.hxx \
	UdpTransport.hxx \
	UnknownHeaderType.hxx \
	UnknownParameter.hxx \
	UnknownParameterType.hxx \
	Uri.hxx \
	Via.hxx \
	WarningCategory.hxx \
	X509Contents.hxx \
	XMLCursor.hxx \
	config.hxx

resiprocate_external_includedir=$(includedir)/resiprocate/external
resiprocate_external_include_HEADERS = \
	external/AsyncID.hxx \
	external/ExternalDns.hxx \
	external/ExternalTimer.hxx \
	external/HttpGetMessage.hxx \
	external/HttpProvider.hxx 

resiprocate_os_includedir=$(includedir)/resiprocate/os
resiprocate_os_include_HEADERS = \
	os/AbstractFifo.hxx \
	os/AsyncProcessHandler.hxx \
	os/BaseException.hxx \
	os/CircularBuffer.hxx \
	os/Coders.hxx \
	os/Condition.hxx \
	os/CountStream.hxx \
	os/Data.hxx \
	os/DataStream.hxx \
	os/DnsUtil.hxx \
	os/Fifo.hxx \
	os/FileSystem.hxx \
	os/FiniteFifo.hxx \
        os/FlowId.hxx \
	os/HashMap.hxx \
	os/HeapInstanceCounter.hxx \
	os/Id.hxx \
	os/Inserter.hxx \
	os/IntrusiveListElement.hxx \
	os/Lock.hxx \
	os/Lockable.hxx \
	os/Log.hxx \
	os/Logger.hxx \
	os/MD5Stream.hxx \
	os/Mutex.hxx \
	os/ParseBuffer.hxx \
	os/Poll.hxx \
	os/RWMutex.hxx \
	os/Random.hxx \
	os/RecursiveMutex.hxx \
	os/SHA1Stream.hxx \
	os/SelectInterruptor.hxx \
	os/SharedCount.hxx \
	os/SharedPtr.hxx \
	os/Socket.hxx \
	os/Subsystem.hxx \
	os/SysLogBuf.hxx \
	os/SysLogStream.hxx \
	os/ThreadIf.hxx \
	os/TimeLimitFifo.hxx \
	os/Timer.hxx \
	os/TransportType.hxx \
	os/Tuple.hxx \
	os/WinCompat.hxx \
	os/WinLeakCheck.hxx \
	os/compat.hxx \
	os/vmd5.hxx \
	os/vthread.hxx 

resiprocate_dns_includedir=$(includedir)/resiprocate/dns
resiprocate_dns_include_HEADERS = \
       dns/DnsAAAARecord.hxx \
       dns/DnsCnameRecord.hxx \
       dns/DnsHostRecord.hxx \
       dns/DnsNaptrRecord.hxx \
       dns/DnsResourceRecord.hxx \
       dns/DnsSrvRecord.hxx \
       dns/DnsStub.hxx \
       dns/QueryTypes.hxx \
       dns/RRCache.hxx \
       dns/RRFactory.hxx \
       dns/RRList.hxx \
       dns/RROverlay.hxx \
       dns/RRVip.hxx


#SUFFIXES = .gperf .cxx
#GPERFFLAGS = -D --enum -E -L C++ -t -k '*' --compare-strncmp
#.gperf.cxx:
#	gperf $(GPERFFLAGS) -Z `echo $* | sed -e 's/.*\///'` $< | \
#	sed -e 's/str\[\([0-9][0-9]*\)\]/tolower(str[\1])/g' | \
#	sed -e 's/^\([	]*\)if *(\*\([a-z][a-z]*\) *== *\*\([a-z][a-z]*\) *\&\& *!strncmp *(\([^)]*\)).*/\1if (tolower(*\2) == *\3 \&\& !strncasecmp( \4 ))/g' > $@

