# $Id$

EXTRA_DIST = README.txt

SUBDIRS = . 

#AM_CXXFLAGS = -DUSE_ARES
AM_CXXFLAGS = -I $(top_srcdir)

lib_LTLIBRARIES = libb2bua.la

libb2bua_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic
libb2bua_la_LIBADD = ../resip/dum/libdum.la
libb2bua_la_LIBADD += ../resip/stack/libresip.la ../rutil/librutil.la
libb2bua_la_LIBADD += @LIBSSL_LIBADD@ @LIBPTHREAD_LIBADD@

libb2bua_la_SOURCES = \
        AuthenticationManager.cxx \
        B2BCall.cxx \
        B2BCallManager.cxx \
        B2BUA.cxx \
        CallHandle.cxx \
        DailyCDRHandler.cxx \
        DefaultAuthorizationManager.cxx \
        DialogUsageManagerRecurringTask.cxx \
        DummyRegistrationPersistenceManager.cxx \
        DummyServerRegistrationHandler.cxx \
        MediaManager.cxx \
        MediaProxy.cxx \
        MyAppDialog.cxx \
        MyDialogSetHandler.cxx \
        MyInviteSessionHandler.cxx \
        RtpProxyRecurringTask.cxx \
        RtpProxyUtil.cxx \
        TaskManager.cxx

b2buaincludedir = $(includedir)/b2bua
nobase_b2buainclude_HEADERS = \
        AccountingManager.hxx \
        AuthenticationManager.hxx \
        AuthorizationManager.hxx \
        B2BCall.hxx \
        B2BCallManager.hxx \
        B2BUA.hxx \
        CallHandle.hxx \
        CDRHandler.hxx \
        DefaultAuthorizationManager.hxx \
        DialogUsageManagerRecurringTask.hxx \
        DummyRegistrationPersistenceManager.hxx \
        DummyServerRegistrationHandler.hxx \
        Logging.hxx \
        MediaManager.hxx \
        MediaProxy.hxx \
        MyAppDialog.hxx \
        MyDialogSetHandler.hxx \
        MyInviteSessionHandler.hxx \
        RtpProxyRecurringTask.hxx \
        RtpProxyUtil.hxx \
        TaskManager.hxx

##############################################################################
# 
# Copyright (c) 2012 Daniel Pocock.  All rights reserved.
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
##############################################################################
