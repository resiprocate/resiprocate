# $Id$

EXTRA_DIST = clicktocall.config
EXTRA_DIST += doc
EXTRA_DIST += *.sln
EXTRA_DIST += *.vcproj

SUBDIRS = .
SUBDIRS += test

#AM_CXXFLAGS = -DUSE_ARES

sbin_PROGRAMS = clicktocall
clicktocall_LDADD = ../../resip/dum/libdum.la
clicktocall_LDADD += ../../resip/stack/libresip.la ../../rutil/librutil.la
clicktocall_LDADD += @LIBSSL_LIBADD@ -lpcre @LIBPTHREAD_LIBADD@

clicktocall_SOURCES = \
        AddressTranslator.cxx \
        AppSubsystem.cxx \
        B2BSession.cxx \
        clicktocall.cxx \
        ConfigParser.cxx \
        HttpBase.cxx \
        HttpConnection.cxx \
        Server.cxx \
        WebAdmin.cxx \
        WebAdminThread.cxx \
        XmlRpcConnection.cxx \
        XmlRpcServerBase.cxx \
        XmlRpcServer.cxx \
        XmlRpcServerThread.cxx


clicktocallincludedir = $(includedir)/clicktocall
nobase_clicktocallinclude_HEADERS = ConfigParser.hxx \
        Version.hxx \
        WebAdminThread.hxx \
        XmlRpcServerBase.hxx \
        HttpBase.hxx \
        XmlRpcConnection.hxx \
        AppSubsystem.hxx \
        XmlRpcServerThread.hxx \
        B2BSession.hxx \
        AddressTranslator.hxx \
        WebAdmin.hxx \
        XmlRpcServer.hxx \
        HttpConnection.hxx \
        Server.hxx \
        ClickToCallCmds.hxx


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
