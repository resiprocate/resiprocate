# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIP RUTIL ARES PTHREAD OPENSSL

CXXFLAGS += -I../../
CODE_SUBDIRS = 
TARGET_LIBRARY = libresip
TESTPROGRAMS =

SRC = \
	Auth.cxx \
	CSeqCategory.cxx \
	CallId.cxx \
	DateCategory.cxx \
	ExpiresCategory.cxx \
	GenericUri.cxx \
	IntegerCategory.cxx \
	UInt32Category.cxx \
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
	BasicNonceHelper.cxx \
	BranchParameter.cxx \
	Connection.cxx \
	ConnectionBase.cxx \
	ConnectionManager.cxx \
	Contents.cxx \
	ContentsFactoryBase.cxx \
	CpimContents.cxx \
	DataParameter.cxx \
	DeprecatedDialog.cxx \
	DnsInterface.cxx \
	DnsResult.cxx \
	DtlsMessage.cxx \
	DtlsTransport.cxx \
	Embedded.cxx \
	ExtensionParameter.cxx \
	ExtensionHeader.cxx \
	ExistsParameter.cxx \
	ExternalBodyContents.cxx \
	QValue.cxx \
	QValueParameter.cxx \
	FlowId.cxx \
	GenericContents.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	HeaderHash.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	UInt32Parameter.cxx \
	InternalTransport.cxx \
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
	NonceHelper.cxx \
	OctetContents.cxx \
	Parameter.cxx \
	ParameterHash.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
	ParserContainerBase.cxx \
	Pidf.cxx \
	Pkcs7Contents.cxx \
	Pkcs8Contents.cxx \
	PlainContents.cxx \
	QopParameter.cxx \
	QuotedDataParameter.cxx \
	RAckCategory.cxx \
	Rlmi.cxx \
	RportParameter.cxx \
	SERNonceHelper.cxx \
	SdpContents.cxx \
	Security.cxx \
	SecurityAttributes.cxx \
	SipFrag.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
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
	Tuple.cxx \
	TransactionController.cxx \
	MessageFilterRule.cxx \
	TransactionUser.cxx \
	TransactionUserMessage.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportFailure.cxx \
	TransportSelector.cxx \
	TuIM.cxx \
	TuSelector.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \
	X509Contents.cxx \
	XMLCursor.cxx \
	KeepAliveMessage.cxx \
	StatelessHandler.cxx \
	InvalidContents.cxx

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

include $(BUILD)/Makefile.post
INSTALL_INCDIR := $(INSTALL_PREFIX)/include/resip/stack

