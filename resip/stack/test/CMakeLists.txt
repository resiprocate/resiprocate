
########### next target ###############

SET(UAS_SRCS
   UAS.cxx
)

add_executable(UAS ${UAS_SRCS})

target_link_libraries(UAS resip)


########### next target ###############

SET(testEmptyHfv_SRCS
   testEmptyHfv.cxx
)

add_executable(testEmptyHfv ${testEmptyHfv_SRCS})

target_link_libraries(testEmptyHfv resip)


########### next target ###############

SET(RFC4475TortureTests_SRCS
   RFC4475TortureTests.cxx
)

add_executable(RFC4475TortureTests ${RFC4475TortureTests_SRCS})

target_link_libraries(RFC4475TortureTests resip)


########### next target ###############

SET(limpc_SRCS
   limpc.cxx
)

add_executable(limpc ${limpc_SRCS})

target_link_libraries(limpc resip)


########### next target ###############

SET(test503Generator_SRCS
   test503Generator.cxx
)

add_executable(test503Generator ${test503Generator_SRCS})

target_link_libraries(test503Generator resip)


########### next target ###############

SET(testAppTimer_SRCS
   testAppTimer.cxx
)

add_executable(testAppTimer ${testAppTimer_SRCS})

target_link_libraries(testAppTimer resip)


########### next target ###############

SET(testApplicationSip_SRCS
   testApplicationSip.cxx
   TestSupport.cxx
)

add_executable(testApplicationSip ${testApplicationSip_SRCS})

target_link_libraries(testApplicationSip resip)


########### next target ###############

SET(testClient_SRCS
   testClient.cxx
)

add_executable(testClient ${testClient_SRCS})

target_link_libraries(testClient resip)


########### next target ###############

SET(testConnectionBase_SRCS
   testConnectionBase.cxx
   TestSupport.cxx
)

add_executable(testConnectionBase ${testConnectionBase_SRCS})

target_link_libraries(testConnectionBase resip)


########### next target ###############

SET(testCorruption_SRCS
   testCorruption.cxx
)

add_executable(testCorruption ${testCorruption_SRCS})

target_link_libraries(testCorruption resip)


########### next target ###############

SET(testDigestAuthentication_SRCS
   testDigestAuthentication.cxx
   TestSupport.cxx
)

add_executable(testDigestAuthentication ${testDigestAuthentication_SRCS})

target_link_libraries(testDigestAuthentication resip)


########### next target ###############

SET(testDtlsTransport_SRCS
   testDtlsTransport.cxx
)

add_executable(testDtlsTransport ${testDtlsTransport_SRCS})

target_link_libraries(testDtlsTransport resip)


########### next target ###############

SET(testDns_SRCS
   testDns.cxx
)

add_executable(testDns ${testDns_SRCS})

target_link_libraries(testDns resip)


########### next target ###############

SET(testEmbedded_SRCS
   testEmbedded.cxx
)

add_executable(testEmbedded ${testEmbedded_SRCS})

target_link_libraries(testEmbedded resip)


########### next target ###############

SET(testEmptyHeader_SRCS
   testEmptyHeader.cxx
   TestSupport.cxx
)

add_executable(testEmptyHeader ${testEmptyHeader_SRCS})

target_link_libraries(testEmptyHeader resip)


########### next target ###############

SET(testExternalLogger_SRCS
   testExternalLogger.cxx
)

add_executable(testExternalLogger ${testExternalLogger_SRCS})

target_link_libraries(testExternalLogger resip)


########### next target ###############

SET(testIM_SRCS
   testIM.cxx
)

add_executable(testIM ${testIM_SRCS})

target_link_libraries(testIM resip)


########### next target ###############

SET(testLockStep_SRCS
   testLockStep.cxx
)

add_executable(testLockStep ${testLockStep_SRCS})

target_link_libraries(testLockStep resip)


########### next target ###############

SET(testMessageWaiting_SRCS
   testMessageWaiting.cxx
)

add_executable(testMessageWaiting ${testMessageWaiting_SRCS})

target_link_libraries(testMessageWaiting resip)


########### next target ###############

SET(testMultipartMixedContents_SRCS
   testMultipartMixedContents.cxx
   TestSupport.cxx
)

add_executable(testMultipartMixedContents ${testMultipartMixedContents_SRCS})

target_link_libraries(testMultipartMixedContents resip)


########### next target ###############

SET(testMultipartRelated_SRCS
   testMultipartRelated.cxx
   TestSupport.cxx
)

add_executable(testMultipartRelated ${testMultipartRelated_SRCS})

target_link_libraries(testMultipartRelated resip)


########### next target ###############

SET(testParserCategories_SRCS
   testParserCategories.cxx
)

add_executable(testParserCategories ${testParserCategories_SRCS})

target_link_libraries(testParserCategories resip)


########### next target ###############

SET(testPidf_SRCS
   testPidf.cxx
)

add_executable(testPidf ${testPidf_SRCS})

target_link_libraries(testPidf resip)


########### next target ###############

SET(testPksc7_SRCS
   testPksc7.cxx
   TestSupport.cxx
)

add_executable(testPksc7 ${testPksc7_SRCS})

target_link_libraries(testPksc7 resip)


########### next target ###############

SET(testPlainContents_SRCS
   testPlainContents.cxx
)

add_executable(testPlainContents ${testPlainContents_SRCS})

target_link_libraries(testPlainContents resip)


########### next target ###############

SET(testResponses_SRCS
   testResponses.cxx
)

add_executable(testResponses ${testResponses_SRCS})

target_link_libraries(testResponses resip)


########### next target ###############

SET(testRlmi_SRCS
   testRlmi.cxx
   TestSupport.cxx
)

add_executable(testRlmi ${testRlmi_SRCS})

target_link_libraries(testRlmi resip)


########### next target ###############

SET(testSdp_SRCS
   testSdp.cxx
   TestSupport.cxx
)

add_executable(testSdp ${testSdp_SRCS})

target_link_libraries(testSdp resip)


########### next target ###############

SET(testSelect_SRCS
   testSelect.cxx
)

add_executable(testSelect ${testSelect_SRCS})

target_link_libraries(testSelect resip)


########### next target ###############

SET(testSelectInterruptor_SRCS
   testSelectInterruptor.cxx
)

add_executable(testSelectInterruptor ${testSelectInterruptor_SRCS})

target_link_libraries(testSelectInterruptor resip)


########### next target ###############

SET(testServer_SRCS
   testServer.cxx
)

add_executable(testServer ${testServer_SRCS})

target_link_libraries(testServer resip)


########### next target ###############

SET(testSipFrag_SRCS
   testSipFrag.cxx
   TestSupport.cxx
)

add_executable(testSipFrag ${testSipFrag_SRCS})

target_link_libraries(testSipFrag resip)


########### next target ###############

SET(testSipMessage_SRCS
   testSipMessage.cxx
   TestSupport.cxx
)

add_executable(testSipMessage ${testSipMessage_SRCS})

target_link_libraries(testSipMessage resip)


########### next target ###############

SET(testSipMessageEncode_SRCS
   testSipMessageEncode.cxx
)

add_executable(testSipMessageEncode ${testSipMessageEncode_SRCS})

target_link_libraries(testSipMessageEncode resip)


########### next target ###############

SET(testSipMessageMemory_SRCS
   testSipMessageMemory.cxx
   TestSupport.cxx
)

add_executable(testSipMessageMemory ${testSipMessageMemory_SRCS})

target_link_libraries(testSipMessageMemory resip)


########### next target ###############

SET(testSipStack1_SRCS
   testSipStack1.cxx
)

add_executable(testSipStack1 ${testSipStack1_SRCS})

target_link_libraries(testSipStack1 resip)


########### next target ###############

SET(testStack_SRCS
   testStack.cxx
)

add_executable(testStack ${testStack_SRCS})

target_link_libraries(testStack resip)


########### next target ###############

SET(testTcp_SRCS
   testTcp.cxx
)

add_executable(testTcp ${testTcp_SRCS})

target_link_libraries(testTcp resip)


########### next target ###############

SET(testTime_SRCS
   testTime.cxx
)

add_executable(testTime ${testTime_SRCS})

target_link_libraries(testTime resip)


########### next target ###############

SET(testTimer_SRCS
   testTimer.cxx
)

add_executable(testTimer ${testTimer_SRCS})

target_link_libraries(testTimer resip)


########### next target ###############

SET(testTransactionFSM_SRCS
   testTransactionFSM.cxx
   TestSupport.cxx
)

add_executable(testTransactionFSM ${testTransactionFSM_SRCS})

target_link_libraries(testTransactionFSM resip)


########### next target ###############

SET(testTuple_SRCS
   testTuple.cxx
)

add_executable(testTuple ${testTuple_SRCS})

target_link_libraries(testTuple resip)


########### next target ###############

SET(testTypedef_SRCS
   testTypedef.cxx
)

add_executable(testTypedef ${testTypedef_SRCS})

target_link_libraries(testTypedef resip)


########### next target ###############

SET(testUdp_SRCS
   testUdp.cxx
)

add_executable(testUdp ${testUdp_SRCS})

target_link_libraries(testUdp resip)


########### next target ###############

SET(testUri_SRCS
   testUri.cxx
   TestSupport.cxx
)

add_executable(testUri ${testUri_SRCS})

target_link_libraries(testUri resip)


########### next target ###############

SET(testWsCookieContext_SRCS
   testWsCookieContext.cxx
)

add_executable(testWsCookieContext ${testWsCookieContext_SRCS})

target_link_libraries(testWsCookieContext resip)

########### next target ###############

if (USE_SSL)
SET(testSocketFunc_SRCS
  testSocketFunc.cxx
)

add_executable(testSocketFunc ${testSocketFunc_SRCS})

target_link_libraries(testSocketFunc resip)

endif (USE_SSL)


########### install files ###############




#original Makefile.am contents follow:

## $Id$
#
#EXTRA_DIST = *.dat
#EXTRA_DIST += dumpasn1.cfg dumpasn1.c
#EXTRA_DIST += host.dns
#EXTRA_DIST += *.sh
#EXTRA_DIST += Mac
#EXTRA_DIST += Makefile-nossl.w32
#EXTRA_DIST += Makefile.w32
#EXTRA_DIST += make-nossl_w32.bat
#EXTRA_DIST += make_w32.bat
#EXTRA_DIST += msgHeaderDiagram-4858.dot
#EXTRA_DIST += openldap.srv
#EXTRA_DIST += raw-tests.txt
#EXTRA_DIST += RSP-2 RSP-3
#EXTRA_DIST += runtests.sh
#EXTRA_DIST += test_10_0.vcxproj test_10_0.vcxproj.filters
#EXTRA_DIST += *.vcproj
#EXTRA_DIST += test.rc
#EXTRA_DIST += *.py
#EXTRA_DIST += torture-test.txt
#EXTRA_DIST += yahoo.dns
#EXTRA_DIST += testTFSM-clientinvite
#EXTRA_DIST += testTFSM-clientnoninvite
#EXTRA_DIST += testTFSM-serverinvite
#EXTRA_DIST += testTFSM-servernoninvite
#
##AM_CXXFLAGS = -DUSE_ARES
#AM_CXXFLAGS = -I $(top_srcdir)
#
#LDADD = ../libresip.la
#LDADD += ../../../rutil/librutil.la
##LDADD += ../../../contrib/ares/libares.a
#LDADD += $(LIBSSL_LIBADD) $(LIBPOPT_LIBADD) @LIBSTL_LIBADD@ @LIBPTHREAD_LIBADD@
#
#TESTS = \
#	testAppTimer \
#	testApplicationSip \
#	testConnectionBase \
#	testCorruption \
#	testDigestAuthentication \
#	testEmbedded \
#	testEmptyHeader \
#	testExternalLogger \
#	testIM \
#	testMessageWaiting \
#	testMultipartMixedContents \
#	testMultipartRelated \
#	testParserCategories \
#	testPidf \
#	testPksc7 \
#	testPlainContents \
#	testRlmi \
#	testSdp \
#	testSelectInterruptor \
#	testSipFrag \
#	testSipMessage \
#	testSipMessageMemory \
#	testStack \
#	testTcp \
#	testTime \
#	testTimer \
#	testTuple \
#	testUri \
#	testWsCookieContext
#
#check_PROGRAMS = \
#	UAS \
#	testEmptyHfv \
#	RFC4475TortureTests \
#	limpc \
#	test503Generator \
#	testAppTimer \
#	testApplicationSip \
#	testClient \
#	testConnectionBase \
#	testCorruption \
#	testDigestAuthentication \
#	testDtlsTransport \
#	testDns \
#	testEmbedded \
#	testEmptyHeader \
#	testExternalLogger \
#	testIM \
#	testLockStep \
#	testMessageWaiting \
#	testMultipartMixedContents \
#	testMultipartRelated \
#	testParserCategories \
#	testPidf \
#	testPksc7 \
#	testPlainContents \
#	testResponses \
#	testRlmi \
#	testSdp \
#	testSelect \
#	testSelectInterruptor \
#	testServer \
#	testSipFrag \
#	testSipMessage \
#	testSipMessageEncode \
#	testSipMessageMemory \
#	testSipStack1 \
#	testStack \
#	testTcp \
#	testTime \
#	testTimer \
#	testTransactionFSM \
#	testTuple \
#	testTypedef \
#	testUdp \
#	testUri \
#	testWsCookieContext
#
#if USE_SSL
#TESTS += testSocketFunc
#check_PROGRAMS += testSocketFunc
#endif
#
#UAS_SOURCES = UAS.cxx
#testEmptyHfv_SOURCES = testEmptyHfv.cxx
#RFC4475TortureTests_SOURCES = RFC4475TortureTests.cxx
#limpc_SOURCES = limpc.cxx
#test503Generator_SOURCES = test503Generator.cxx
#testAppTimer_SOURCES = testAppTimer.cxx
#testApplicationSip_SOURCES = testApplicationSip.cxx TestSupport.cxx
#testClient_SOURCES = testClient.cxx
#testConnectionBase_SOURCES = testConnectionBase.cxx TestSupport.cxx
#testCorruption_SOURCES = testCorruption.cxx
#testDigestAuthentication_SOURCES = testDigestAuthentication.cxx TestSupport.cxx
#testDtlsTransport_SOURCES = testDtlsTransport.cxx
#testDns_SOURCES = testDns.cxx
#testEmbedded_SOURCES = testEmbedded.cxx
#testEmptyHeader_SOURCES = testEmptyHeader.cxx TestSupport.cxx
#testExternalLogger_SOURCES = testExternalLogger.cxx
#testIM_SOURCES = testIM.cxx
#testLockStep_SOURCES = testLockStep.cxx
#testMessageWaiting_SOURCES = testMessageWaiting.cxx
#testMultipartMixedContents_SOURCES = testMultipartMixedContents.cxx TestSupport.cxx
#testMultipartRelated_SOURCES = testMultipartRelated.cxx TestSupport.cxx
#testParserCategories_SOURCES = testParserCategories.cxx
#testPidf_SOURCES = testPidf.cxx
#testPksc7_SOURCES = testPksc7.cxx TestSupport.cxx
#testPlainContents_SOURCES = testPlainContents.cxx
#testResponses_SOURCES = testResponses.cxx
#testRlmi_SOURCES = testRlmi.cxx TestSupport.cxx
#testSdp_SOURCES = testSdp.cxx TestSupport.cxx
#testSelect_SOURCES = testSelect.cxx
#testSelectInterruptor_SOURCES = testSelectInterruptor.cxx
#testServer_SOURCES = testServer.cxx
#testSipFrag_SOURCES = testSipFrag.cxx TestSupport.cxx
#testSipMessage_SOURCES = testSipMessage.cxx TestSupport.cxx
#testSipMessageEncode_SOURCES = testSipMessageEncode.cxx
#testSipMessageMemory_SOURCES = testSipMessageMemory.cxx TestSupport.cxx
#testSipStack1_SOURCES = testSipStack1.cxx
#testSocketFunc_SOURCES = testSocketFunc.cxx
#testStack_SOURCES = testStack.cxx
#testTcp_SOURCES = testTcp.cxx
#testTime_SOURCES = testTime.cxx
#testTimer_SOURCES = testTimer.cxx
#testTransactionFSM_SOURCES = testTransactionFSM.cxx TestSupport.cxx
#testTuple_SOURCES = testTuple.cxx
#testTypedef_SOURCES = testTypedef.cxx
#testUdp_SOURCES = testUdp.cxx
#testUri_SOURCES = testUri.cxx TestSupport.cxx
#testWsCookieContext_SOURCES = testWsCookieContext.cxx
#
#noinst_HEADERS = digcalc.hxx \
#	InviteClient.hxx \
#	InviteServer.hxx \
#	md5.hxx \
#	Register.hxx \
#	Registrar.hxx \
#	Resolver.hxx \
#	testIM.hxx \
#	TestSupport.hxx \
#	Transceiver.hxx \
#	tassert.h
#
###############################################################################
## 
## The Vovida Software License, Version 1.0 
## Copyright (c) 2000-2007 Vovida Networks, Inc.  All rights reserved.
## 
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
## 
## 1. Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
## 
## 2. Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in
##    the documentation and/or other materials provided with the
##    distribution.
## 
## 3. The names "VOCAL", "Vovida Open Communication Application Library",
##    and "Vovida Open Communication Application Library (VOCAL)" must
##    not be used to endorse or promote products derived from this
##    software without prior written permission. For written
##    permission, please contact vocal@vovida.org.
## 
## 4. Products derived from this software may not be called "VOCAL", nor
##    may "VOCAL" appear in their name, without prior written
##    permission of Vovida Networks, Inc.
## 
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
## WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
## OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
## NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
## NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
## IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
## USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
## DAMAGE.
## 
## ====================================================================
## 
## This software consists of voluntary contributions made by Vovida
## Networks, Inc. and many individuals on behalf of Vovida Networks,
## Inc.  For more information on Vovida Networks, Inc., please see
## <http://www.vovida.org/>.
## 
###############################################################################
#
