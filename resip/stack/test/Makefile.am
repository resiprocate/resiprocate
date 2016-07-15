# $Id$

EXTRA_DIST = *.dat
EXTRA_DIST += dumpasn1.cfg dumpasn1.c
EXTRA_DIST += host.dns
EXTRA_DIST += *.sh
EXTRA_DIST += Mac
EXTRA_DIST += Makefile-nossl.w32
EXTRA_DIST += Makefile.w32
EXTRA_DIST += make-nossl_w32.bat
EXTRA_DIST += make_w32.bat
EXTRA_DIST += msgHeaderDiagram-4858.dot
EXTRA_DIST += openldap.srv
EXTRA_DIST += raw-tests.txt
EXTRA_DIST += RSP-2 RSP-3
EXTRA_DIST += runtests.sh
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += *.vcproj
EXTRA_DIST += test.rc
EXTRA_DIST += *.py
EXTRA_DIST += torture-test.txt
EXTRA_DIST += yahoo.dns
EXTRA_DIST += testTFSM-clientinvite
EXTRA_DIST += testTFSM-clientnoninvite
EXTRA_DIST += testTFSM-serverinvite
EXTRA_DIST += testTFSM-servernoninvite

#AM_CXXFLAGS = -DUSE_ARES
AM_CXXFLAGS = -I $(top_srcdir)

LDADD = ../libresip.la
LDADD += ../../../rutil/librutil.la
#LDADD += ../../../contrib/ares/libares.a
LDADD += $(LIBSSL_LIBADD) $(LIBPOPT_LIBADD) @LIBSTL_LIBADD@ @LIBPTHREAD_LIBADD@

TESTS = \
	testAppTimer \
	testApplicationSip \
	testConnectionBase \
	testCorruption \
	testDigestAuthentication \
	testEmbedded \
	testEmptyHeader \
	testExternalLogger \
    testGenericPidfContents \
	testIM \
	testMessageWaiting \
	testMultipartMixedContents \
	testMultipartRelated \
	testParserCategories \
	testPidf \
	testPksc7 \
	testPlainContents \
	testRlmi \
	testDtmfPayload \
	testSdp \
	testSelectInterruptor \
	testSipFrag \
	testSipMessage \
	testSipMessageMemory \
	testStack \
	testTcp \
	testTime \
	testTimer \
	testTuple \
	testUri \
	testWsCookieContext

check_PROGRAMS = \
	UAS \
	testEmptyHfv \
	RFC4475TortureTests \
	limpc \
	test503Generator \
	testAppTimer \
	testApplicationSip \
	testClient \
	testConnectionBase \
	testCorruption \
	testDigestAuthentication \
	testDtlsTransport \
	testDns \
	testEmbedded \
	testEmptyHeader \
	testExternalLogger \
    testGenericPidfContents \
	testIM \
	testLockStep \
	testMessageWaiting \
	testMultipartMixedContents \
	testMultipartRelated \
	testParserCategories \
	testPidf \
	testPksc7 \
	testPlainContents \
	testResponses \
	testRlmi \
	testDtmfPayload \
	testSdp \
	testSelect \
	testSelectInterruptor \
	testServer \
	testSipFrag \
	testSipMessage \
	testSipMessageEncode \
	testSipMessageMemory \
	testSipStack1 \
	testSipStackNetNs \
	testStack \
	testTcp \
	testTime \
	testTimer \
	testTransactionFSM \
	testTuple \
	testTypedef \
	testUdp \
	testUri \
	testWsCookieContext

if USE_SSL
TESTS += testSocketFunc \
	testSecurity
check_PROGRAMS += testSocketFunc \
	testSecurity
endif

UAS_SOURCES = UAS.cxx
testEmptyHfv_SOURCES = testEmptyHfv.cxx
RFC4475TortureTests_SOURCES = RFC4475TortureTests.cxx
limpc_SOURCES = limpc.cxx
test503Generator_SOURCES = test503Generator.cxx
testAppTimer_SOURCES = testAppTimer.cxx
testApplicationSip_SOURCES = testApplicationSip.cxx TestSupport.cxx
testClient_SOURCES = testClient.cxx
testConnectionBase_SOURCES = testConnectionBase.cxx TestSupport.cxx
testCorruption_SOURCES = testCorruption.cxx
testDigestAuthentication_SOURCES = testDigestAuthentication.cxx TestSupport.cxx
testDtlsTransport_SOURCES = testDtlsTransport.cxx
testDtmfPayload_SOURCES = testDtmfPayload.cxx
testDns_SOURCES = testDns.cxx
testEmbedded_SOURCES = testEmbedded.cxx
testEmptyHeader_SOURCES = testEmptyHeader.cxx TestSupport.cxx
testExternalLogger_SOURCES = testExternalLogger.cxx
testGenericPidfContents_SOURCES = testGenericPidfContents.cxx TestSupport.cxx
testIM_SOURCES = testIM.cxx
testLockStep_SOURCES = testLockStep.cxx
testMessageWaiting_SOURCES = testMessageWaiting.cxx
testMultipartMixedContents_SOURCES = testMultipartMixedContents.cxx TestSupport.cxx
testMultipartRelated_SOURCES = testMultipartRelated.cxx TestSupport.cxx
testParserCategories_SOURCES = testParserCategories.cxx
testPidf_SOURCES = testPidf.cxx
testPksc7_SOURCES = testPksc7.cxx TestSupport.cxx
testPlainContents_SOURCES = testPlainContents.cxx
testResponses_SOURCES = testResponses.cxx
testRlmi_SOURCES = testRlmi.cxx TestSupport.cxx
testSdp_SOURCES = testSdp.cxx TestSupport.cxx
testSecurity_SOURCES = testSecurity.cxx
testSelect_SOURCES = testSelect.cxx
testSelectInterruptor_SOURCES = testSelectInterruptor.cxx
testServer_SOURCES = testServer.cxx
testSipFrag_SOURCES = testSipFrag.cxx TestSupport.cxx
testSipMessage_SOURCES = testSipMessage.cxx TestSupport.cxx
testSipMessageEncode_SOURCES = testSipMessageEncode.cxx
testSipMessageMemory_SOURCES = testSipMessageMemory.cxx TestSupport.cxx
testSipStack1_SOURCES = testSipStack1.cxx
testSipStackNetNs_SOURCES = testSipStackNetNs.cxx
testSocketFunc_SOURCES = testSocketFunc.cxx
testStack_SOURCES = testStack.cxx
testTcp_SOURCES = testTcp.cxx
testTime_SOURCES = testTime.cxx
testTimer_SOURCES = testTimer.cxx
testTransactionFSM_SOURCES = testTransactionFSM.cxx TestSupport.cxx
testTuple_SOURCES = testTuple.cxx
testTypedef_SOURCES = testTypedef.cxx
testUdp_SOURCES = testUdp.cxx
testUri_SOURCES = testUri.cxx TestSupport.cxx
testWsCookieContext_SOURCES = testWsCookieContext.cxx

noinst_HEADERS = digcalc.hxx \
	InviteClient.hxx \
	InviteServer.hxx \
	md5.hxx \
	Register.hxx \
	Registrar.hxx \
	Resolver.hxx \
	testIM.hxx \
	TestSupport.hxx \
	Transceiver.hxx \
	tassert.h

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

