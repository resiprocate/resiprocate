# $Id: Makefile,v 1.70 2004/05/12 01:14:30 jason Exp $

BUILD = ../../../build
CONTRIB = ../../../contrib

include $(BUILD)/Makefile.pre

ifeq ($(USE_DTLS), yes)
PACKAGES += DTLS_SHIM 
TESTPROGRAMS += testshim.cxx
endif

PACKAGES += RESIP RUTIL ARES OPENSSL PTHREAD POPT OPENSIGCOMP

ifeq ($(USE_SSL), yes)
TESTPROGRAMS += makeSelfCert.cxx \
		testSecurity.cxx \
		testTls.cxx 
endif


TESTPROGRAMS += \
testIdentity.cxx \
testEmptyHfv.cxx \
RFC4475TortureTests.cxx \
limpc.cxx \
test503Generator.cxx \
testAppTimer.cxx \
testApplicationSip.cxx \
testClient.cxx \
testConnectionBase.cxx \
testCorruption.cxx \
testDigestAuthentication.cxx \
testDtlsTransport.cxx \
testDns.cxx \
testEmbedded.cxx \
testEmptyHeader.cxx \
testExternalLogger.cxx \
testFlowId.cxx \
testIM.cxx \
testLockStep.cxx \
testMessageWaiting.cxx \
testMultipartMixedContents.cxx \
testMultipartRelated.cxx \
testParserCategories.cxx \
testPidf.cxx \
testPidf.cxx \
testPksc7.cxx \
testPlainContents.cxx \
testRSP-2.cxx \
testResponses.cxx \
testRlmi.cxx \
testSdp.cxx \
testSelect.cxx \
testSelectInterruptor.cxx \
testServer.cxx \
testSipFrag.cxx \
testSipMessage.cxx \
testSipMessageMemory.cxx \
testSipStack1.cxx \
testStack.cxx \
testTcp.cxx \
testTime.cxx \
testTimer.cxx \
testTransactionFSM.cxx \
testTuple.cxx \
testTypedef.cxx \
testUdp.cxx \
testUri.cxx \
testXMLCursor.cxx 

#testTransportSelector.cxx


SRC = 		TestSupport.cxx

dumpasn1: dumpasn1.c
	gcc -o dumpasn1 dumpasn1.c

include $(BUILD)/Makefile.post

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
