# $Id$

EXTRA_DIST = DayOfWeekHash.gperf
EXTRA_DIST += Doxyfile
EXTRA_DIST += FAQ
EXTRA_DIST += gperfNotes.txt
EXTRA_DIST += gperf_w32.bat
EXTRA_DIST += groups.doc
EXTRA_DIST += HeaderHash.gperf
EXTRA_DIST += mainpage.doc
EXTRA_DIST += MethodHash.gperf
EXTRA_DIST += MonthHash.gperf
EXTRA_DIST += ParameterHash.gperf
EXTRA_DIST += parametersA.gperf
EXTRA_DIST += Readme-Compliance.txt
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += *.vcproj
EXTRA_DIST += ssl/WinSecurity.cxx

SUBDIRS = . test

AM_CXXFLAGS = -I $(top_srcdir)

# 
# make chokes on the filenames under doc/ because some of them have spaces
# in them.  The content needs to be reviewed and renamed appropriately
# before it can be distributed by `make dist'
#
#SUBDIRS += doc

#AM_CXXFLAGS = -I../../contrib/ares -DUSE_ARES

BUILT_SOURCES = \
	gen/DayOfWeekHash.cxx \
	gen/HeaderHash.cxx \
	gen/MethodHash.cxx \
	gen/MonthHash.cxx \
	gen/ParameterHash.cxx

lib_LTLIBRARIES = libresip.la

libresip_la_LIBADD = ../../rutil/librutil.la
libresip_la_LIBADD += @LIBSSL_LIBADD@
libresip_la_LIBADD += @LIBSTL_LIBADD@
libresip_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic

libresip_la_SOURCES = \
	Auth.cxx \
	CSeqCategory.cxx \
	CallId.cxx \
	Cookie.cxx \
	DateCategory.cxx \
	gen/DayOfWeekHash.cxx \
	DtmfPayloadContents.cxx \
	ExpiresCategory.cxx \
	GenericUri.cxx \
	IntegerCategory.cxx \
	InteropHelper.cxx \
	UInt32Category.cxx \
	Mime.cxx \
	NameAddr.cxx \
	ParserCategories.cxx \
	RequestLine.cxx \
	StatusLine.cxx \
	StringCategory.cxx \
	Token.cxx \
	TokenOrQuotedStringCategory.cxx \
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
	Embedded.cxx \
	ExtensionParameter.cxx \
	ExtensionHeader.cxx \
	ExistsOrDataParameter.cxx \
	ExistsParameter.cxx \
	ExternalBodyContents.cxx \
	QValue.cxx \
	QValueParameter.cxx \
	GenericContents.cxx \
	GenericPidfContents.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	gen/HeaderHash.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	UInt32Parameter.cxx \
	InternalTransport.cxx \
	LazyParser.cxx \
	Message.cxx \
	MessageWaitingContents.cxx \
	gen/MethodHash.cxx \
	MethodTypes.cxx \
	gen/MonthHash.cxx \
	MsgHeaderScanner.cxx \
	MultipartAlternativeContents.cxx \
	MultipartMixedContents.cxx \
	MultipartRelatedContents.cxx \
	MultipartSignedContents.cxx \
	NonceHelper.cxx \
	OctetContents.cxx \
	Parameter.cxx \
	gen/ParameterHash.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
	ParserContainerBase.cxx \
	Pidf.cxx \
	Pkcs7Contents.cxx \
	Pkcs8Contents.cxx \
	PlainContents.cxx \
	PrivacyCategory.cxx \
	QuotedDataParameter.cxx \
	RAckCategory.cxx \
	Rlmi.cxx \
	RportParameter.cxx \
	SERNonceHelper.cxx \
	SdpContents.cxx \
	SecurityAttributes.cxx \
	Compression.cxx \
	SipFrag.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
	InterruptableStackThread.cxx \
	EventStackThread.cxx \
	StatisticsHandler.cxx \
	StatisticsManager.cxx \
	StatisticsMessage.cxx \
	Symbols.cxx \
	TcpBaseTransport.cxx \
	TcpConnection.cxx \
	TcpConnectState.cxx \
	TcpTransport.cxx \
	TimeAccumulate.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	Tuple.cxx \
	TupleMarkManager.cxx \
	TransactionController.cxx \
	MessageFilterRule.cxx \
	TransactionUser.cxx \
	TransactionUserMessage.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportThread.cxx \
	TransportFailure.cxx \
	TransportSelector.cxx \
	TuIM.cxx \
	TuSelector.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \
	X509Contents.cxx \
	KeepAliveMessage.cxx \
	StatelessHandler.cxx \
	InvalidContents.cxx \
	WsBaseTransport.cxx \
	WsFrameExtractor.cxx \
	WsTransport.cxx \
	WsConnection.cxx \
	WsConnectionBase.cxx \
	WsCookieContext.cxx \
	WsDecorator.cxx

if USE_SSL
libresip_la_SOURCES += \
	ssl/DtlsTransport.cxx \
	ssl/Security.cxx \
	ssl/TlsBaseTransport.cxx \
	ssl/TlsConnection.cxx \
	ssl/TlsTransport.cxx \
	ssl/WssTransport.cxx \
   ssl/WssConnection.cxx
endif

SUFFIXES = .gperf .cxx
GPERFOPTS = -C -D -E -L C++ -t --key-positions='*' --compare-strncmp
#GPERFVER="GNU gperf 2.7.2"

# rule for case sensitive sorts of hash
gen/MethodHash.cxx: MethodHash.gperf
	mkdir -p $(abs_srcdir)/gen
	gperf $(GPERFOPTS) -Z MethodHash $< > $@

# note: the Date header field is case sensitive (RFC 3261 s20.17)
gen/DayOfWeekHash.cxx: DayOfWeekHash.gperf
	mkdir -p $(abs_srcdir)/gen
	gperf $(GPERFOPTS) -Z DayOfWeekHash $< > $@
gen/MonthHash.cxx: MonthHash.gperf
	mkdir -p $(abs_srcdir)/gen
	gperf $(GPERFOPTS) -Z MonthHash $< > $@

# rule for insensitive clods
#${SRC}: ${@:%.cxx=%.gperf} -- more portable?
gen/%.cxx: %.gperf
	mkdir -p $(abs_srcdir)/gen
	gperf $(GPERFOPTS) --ignore-case -Z $* $< > $@


resipincludedir = $(includedir)/resip/stack
nobase_resipinclude_HEADERS = AbandonServerTransaction.hxx \
	AddTransport.hxx \
	Aor.hxx \
	ApiCheck.hxx \
	ApiCheckList.hxx \
	ApplicationMessage.hxx \
	ApplicationSip.hxx \
	Auth.hxx \
	BasicNonceHelper.hxx \
	BranchParameter.hxx \
	CallId.hxx \
	Cookie.hxx \
	CancelableTimerQueue.hxx \
	CancelClientInviteTransaction.hxx \
	Compression.hxx \
	ConnectionBase.hxx \
	Connection.hxx \
	ConnectionManager.hxx \
	ConnectionTerminated.hxx \
	ContentsFactoryBase.hxx \
	ContentsFactory.hxx \
	Contents.hxx \
	CpimContents.hxx \
	CSeqCategory.hxx \
	DataParameter.hxx \
	DateCategory.hxx \
	DeprecatedDialog.hxx \
	DnsInterface.hxx \
	DnsResult.hxx \
	DnsResultMessage.hxx \
	DtlsMessage.hxx \
	DtmfPayloadContents.hxx \
	Embedded.hxx \
	EnableFlowTimer.hxx \
	EventStackThread.hxx \
	ExistsOrDataParameter.hxx \
	ExistsParameter.hxx \
	ExpiresCategory.hxx \
	ExtensionHeader.hxx \
	ExtensionParameter.hxx \
	ExternalBodyContents.hxx \
	FloatParameter.hxx \
	GenericContents.hxx \
	GenericPidfContents.hxx \
	GenericUri.hxx \
	HeaderFieldValue.hxx \
	HeaderFieldValueList.hxx \
	HeaderHash.hxx \
	Headers.hxx \
	HeaderTypes.hxx \
	Helper.hxx \
	IntegerCategory.hxx \
	IntegerParameter.hxx \
	InternalTransport.hxx \
	InteropHelper.hxx \
	InterruptableStackThread.hxx \
	InvalidContents.hxx \
	KeepAliveMessage.hxx \
	KeepAlivePong.hxx \
	LazyParser.hxx \
	MarkListener.hxx \
	MessageDecorator.hxx \
	MessageFilterRule.hxx \
	Message.hxx \
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
	NonceHelper.hxx \
	OctetContents.hxx \
	ParameterHash.hxx \
	Parameter.hxx \
	ParameterTypeEnums.hxx \
	ParameterTypes.hxx \
	ParserCategories.hxx \
	ParserCategory.hxx \
	ParserContainerBase.hxx \
	ParserContainer.hxx \
	Pidf.hxx \
	Pkcs7Contents.hxx \
	Pkcs8Contents.hxx \
	PlainContents.hxx \
	PollStatistics.hxx \
	PrivacyCategory.hxx \
	QuotedDataParameter.hxx \
	QValue.hxx \
	QValueParameter.hxx \
	RAckCategory.hxx \
	RemoveTransport.hxx \
	RequestLine.hxx \
	Rlmi.hxx \
	RportParameter.hxx \
	SdpContents.hxx \
	SecurityAttributes.hxx \
	SecurityTypes.hxx \
	SendData.hxx \
	SERNonceHelper.hxx \
	ShutdownMessage.hxx \
	SipFrag.hxx \
	SipMessage.hxx \
	SipStack.hxx \
	ssl/DtlsTransport.hxx \
	ssl/MacSecurity.hxx \
	ssl/Security.hxx \
	ssl/TlsBaseTransport.hxx \
	ssl/TlsConnection.hxx \
	ssl/TlsTransport.hxx \
	ssl/WinSecurity.hxx \
	ssl/WssTransport.hxx \
	ssl/WssConnection.hxx \
	StackThread.hxx \
	StartLine.hxx \
	StatelessHandler.hxx \
	StatisticsHandler.hxx \
	StatisticsManager.hxx \
	StatisticsMessage.hxx \
	StatusLine.hxx \
	StringCategory.hxx \
	Symbols.hxx \
	TcpBaseTransport.hxx \
	TcpConnection.hxx \
	TcpConnectState.hxx \
	TcpTransport.hxx \
	TerminateFlow.hxx \
	TimeAccumulate.hxx \
	TimerMessage.hxx \
	TimerQueue.hxx \
	Token.hxx \
	TokenOrQuotedStringCategory.hxx \
	TransactionController.hxx \
	TransactionControllerThread.hxx \
	TransactionMap.hxx \
	TransactionMessage.hxx \
	TransportSelectorThread.hxx \
	TransactionState.hxx \
	TransactionTerminated.hxx \
	TransactionUser.hxx \
	TransactionUserMessage.hxx \
	TransportFailure.hxx \
	Transport.hxx \
	TransportSelector.hxx \
	TransportThread.hxx \
	TuIM.hxx \
	Tuple.hxx \
	TupleMarkManager.hxx \
	TuSelector.hxx \
	UdpTransport.hxx \
	UInt32Category.hxx \
	UInt32Parameter.hxx \
	UnknownHeaderType.hxx \
	UnknownParameter.hxx \
	UnknownParameterType.hxx \
	Uri.hxx \
	ValueFifo.hxx \
	Via.hxx \
	WarningCategory.hxx \
	WsBaseTransport.hxx \
	WsDecorator.hxx \
	WsFrameExtractor.hxx \
	WsTransport.hxx \
	WsConnection.hxx \
	WsConnectionBase.hxx \
	WsConnectionValidator.hxx \
	WsCookieContext.hxx \
	WsCookieContextFactory.hxx \
	X509Contents.hxx \
	ZeroOutStatistics.hxx

##############################################################################
# 
# The Vovida Software License, Version 1.0 
# Copyright (c) 2000-2007 Vovida Networks, Inc.  All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 
# 3. The names "VOCAL", "Vovida Open Communication Application Library",
#    and "Vovida Open Communication Application Library (VOCAL)" must
#    not be used to endorse or promote products derived from this
#    software without prior written permission. For written
#    permission, please contact vocal@vovida.org.
# 
# 4. Products derived from this software may not be called "VOCAL", nor
#    may "VOCAL" appear in their name, without prior written
#    permission of Vovida Networks, Inc.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
# NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
# IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
# 
# ====================================================================
# 
# This software consists of voluntary contributions made by Vovida
# Networks, Inc. and many individuals on behalf of Vovida Networks,
# Inc.  For more information on Vovida Networks, Inc., please see
# <http://www.vovida.org/>.
# 
##############################################################################
