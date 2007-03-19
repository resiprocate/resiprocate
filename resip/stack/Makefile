# $Id: Makefile,v 1.144 2004/05/18 01:40:48 jason Exp $

BUILD = ../../build
include $(BUILD)/Makefile.pre

PACKAGES += RESIP RUTIL ARES PTHREAD OPENSSL OPENSIGCOMP

CXXFLAGS += -I../..
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
	Compression.cxx \
	SelectInterruptor.cxx \
	SipFrag.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
    InterruptableStackThread.cxx \
  StatisticsHandler.cxx \
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
	TupleMarkManager.cxx \
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

code: .firstrun

.firstrun:
	touch .firstrun MethodHash.cxx *Hash.cxx

# rule for case sensitive sorts of hash
MethodHash.cxx: MethodHash.gperf
	gperf -v > /dev/null 2>&1 && gperf $(GPERFOPTS) -Z `echo MethodHash | sed -e 's/.*\///'` $< >  $@

# rule for insensitive clods
#${SRC}: ${@:%.cxx=%.gperf} -- more portable?
%.cxx: %.gperf
	gperf -v > /dev/null 2>&1 && gperf $(GPERFOPTS) -Z `echo $* | sed -e 's/.*\///'` $< | \
	sed -e 's/str\[\([0-9][0-9]*\)\]/tolower(str[\1])/g' | \
	sed -e 's/^\([	]*\)if *(\*\([a-z][a-z]*\) *== *\*\([a-z][a-z]*\) *\&\& *!strncmp *(\([^)]*\)).*/\1if (tolower(*\2) == *\3 \&\& !strncasecmp( \4 ))/g' | \
	sed -e 's/\*str ==/tolower(*str) ==/' | \
	sed -e 's/\!strncmp/\!strncasecmp/'  > $@

include $(BUILD)/Makefile.post
INSTALL_INCDIR := $(DESTDIR)$(INSTALL_PREFIX)/include/resip/stack


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
