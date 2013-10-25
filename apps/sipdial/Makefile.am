# $Id$

man_MANS = sipdialer.1

EXTRA_DIST = README.txt
EXTRA_DIST += test.html
EXTRA_DIST += sipdial.cfg
EXTRA_DIST += todo.txt
EXTRA_DIST += MacOS
EXTRA_DIST += $(man_MANS)

SUBDIRS = .

#AM_CXXFLAGS = -DUSE_ARES

lib_LTLIBRARIES = libsipdial.la
libsipdial_la_LIBADD = ../../resip/dum/libdum.la
libsipdial_la_LIBADD += ../../resip/stack/libresip.la
libsipdial_la_LIBADD += ../../rutil/librutil.la
libsipdial_la_LIBADD += @LIBSSL_LIBADD@ @LIBPTHREAD_LIBADD@
libsipdial_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@
libsipdial_la_LDFLAGS += -export-dynamic

bin_PROGRAMS = sipdialer
sipdialer_LDADD = libsipdial.la
sipdialer_LDADD += ../../resip/dum/libdum.la
sipdialer_LDADD += ../../resip/stack/libresip.la
sipdialer_LDADD += ../../rutil/librutil.la

libsipdial_la_SOURCES = \
        DialInstance.cpp \
        MyInviteSessionHandler.cpp \
        DialerConfiguration.cpp

sipdialer_SOURCES = sipdialer.cpp

sipdialincludedir = $(includedir)/sipdial
nobase_sipdialinclude_HEADERS = DialerConfiguration.hxx \
	DialInstance.hxx \
	MyInviteSessionHandler.hxx


##############################################################################
# 
# Copyright 2012 Daniel Pocock.  All rights reserved.
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
# 3. Neither the name of the author(s) nor the names of any contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# 
# ====================================================================
# 
# 
##############################################################################
