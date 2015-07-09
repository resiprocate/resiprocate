# $Id$

EXTRA_DIST = Makefile.w32 make_w32.bat
EXTRA_DIST += runtests.sh
EXTRA_DIST += sweepRandom.sh
EXTRA_DIST += testConfigParse-1.config

#AM_CXXFLAGS = -DUSE_ARES
AM_CXXFLAGS = -I $(top_srcdir)

LDADD = ../librutil.la
#LDADD += ../../contrib/ares/libares.a
LDADD += $(LIBSSL_LIBADD) @LIBSTL_LIBADD@ @LIBPTHREAD_LIBADD@

TESTS = \
	testCompat \
	testCoders \
	testConfigParse \
	testCountStream \
	testData \
	testDataPerformance \
	testDataStream \
	testDnsUtil \
	testFifo \
	testFileSystem \
	testInserter \
	testIntrusiveList \
	testLogger \
	testMD5Stream \
	testNetNs \
	testParseBuffer \
	testRandomHex \
	testRandomThread \
	testThreadIf \
	testXMLCursor

check_PROGRAMS = \
	testCompat \
	testCoders \
	testConfigParse \
	testCountStream \
	testData \
	testDataPerformance \
	testDataStream \
	testDnsUtil \
	testFifo \
	testFileSystem \
	testInserter \
	testIntrusiveList \
	testLogger \
	testMD5Stream \
	testNetNs \
	testParseBuffer \
	testRandomHex \
	testRandomThread \
	testThreadIf \
	testXMLCursor

testCompat_SOURCES = testCompat.cxx
testCoders_SOURCES = testCoders.cxx
testConfigParse_SOURCES = testConfigParse.cxx
testCountStream_SOURCES = testCountStream.cxx
testData_SOURCES = testData.cxx
testDataPerformance_SOURCES = testDataPerformance.cxx
testDataStream_SOURCES = testDataStream.cxx
testDnsUtil_SOURCES = testDnsUtil.cxx
testFifo_SOURCES = testFifo.cxx
testFileSystem_SOURCES = testFileSystem.cxx
testInserter_SOURCES = testInserter.cxx
testIntrusiveList_SOURCES = testIntrusiveList.cxx
testLogger_SOURCES = testLogger.cxx TestSubsystemLogLevel.cxx
testMD5Stream_SOURCES = testMD5Stream.cxx
testNetNs_SOURCES = testNetNs.cxx
testParseBuffer_SOURCES = testParseBuffer.cxx
testRandomHex_SOURCES = testRandomHex.cxx
testRandomThread_SOURCES = testRandomThread.cxx
testThreadIf_SOURCES = testThreadIf.cxx
testXMLCursor_SOURCES = testXMLCursor.cxx

noinst_HEADERS = TestSubsystemLogLevel.hxx

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
