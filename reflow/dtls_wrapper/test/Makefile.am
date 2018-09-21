AM_CXXFLAGS = -I..

LDADD = ../../../rutil/librutil.la
LDADD += $(LIBSSL_LIBADD) @LIBSTL_LIBADD@ @LIBPTHREAD_LIBADD@
LDADD += -lsrtp

noinst_LTLIBRARIES = libdtlswrappertest.la

libdtlswrappertest_la_SOURCES = \
	../bf_dwrap.cxx \
	../DtlsFactory.cxx \
	../DtlsSocket.cxx \
	../DtlsTimer.cxx \
	\
	CreateCert.cxx \
	TestDtlsUdp.cxx \
	TestTimerContext.cxx

libdtlswrappertest_la_LIBADD = \
	$(LIBSSL_LIBADD)

LDADD += libdtlswrappertest.la

TESTS = \
	testDtlsWrapper

noinst_PROGRAMS = \
	$(TESTS) \
	testDtlsClient \
	testDtlsServer 

testDtlsWrapper_SOURCES = testDtlsWrapper.cxx
testDtlsClient_SOURCES = testDtlsClient.cxx
testDtlsServer_SOURCES = testDtlsServer.cxx

SRC = \
	CreateCert.cxx \
	TestDtlsUdp.cxx \
	TestTimerContext.cxx

####################################################################
#
# Copyright (c) 2007-2008, Plantronics, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are 
# met:
#
# 1. Redistributions of source code must retain the above copyright 
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution. 
#
# 3. Neither the name of Plantronics nor the names of its contributors 
#    may be used to endorse or promote products derived from this 
#    software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
####################################################################