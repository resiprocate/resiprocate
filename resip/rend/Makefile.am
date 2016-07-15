# $Id$

EXTRA_DIST = readme.txt
EXTRA_DIST += design-notes.txt
EXTRA_DIST += flownotes.txt
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += *.vcproj

SUBDIRS = . 

AM_CXXFLAGS = -I $(top_srcdir)

lib_LTLIBRARIES = librend.la
librend_la_LIBADD = ../../resip/dum/libdum.la
librend_la_LIBADD += ../../resip/stack/libresip.la
librend_la_LIBADD += ../../rutil/librutil.la
librend_la_LIBADD += @LIBSSL_LIBADD@ @LIBPTHREAD_LIBADD@
librend_la_LIBADD += -lpopt
librend_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@
librend_la_LDFLAGS += -export-dynamic

bin_PROGRAMS = rendIt
rendIt_LDADD = librend.la
rendIt_LDADD += ../../resip/stack/libresip.la
rendIt_LDADD += ../../rutil/librutil.la
rendIt_LDADD += -lpopt

rendIt_SOURCES = rendIt.cxx

librend_la_SOURCES = \
	RendAcct.cxx \
	RendDlg.cxx \
	RendDlgAcct.cxx \
	RendKa.cxx \
	RendMisc.cxx \
	RendPres.cxx \
	RendReg.cxx \
	RendRunner.cxx \
	RendSketch.cxx \
	RendTroop.cxx

rendincludedir = $(includedir)/rend
nobase_rendinclude_HEADERS = \
	RendAcct.hxx \
	RendDlg.hxx \
	RendDlgAcct.hxx \
	RendKa.hxx \
	RendMisc.hxx \
	RendReport.hxx \
	RendRunner.hxx \
	RendSketch.hxx \
	RendTroop.hxx \
	RendWavePres.hxx

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
