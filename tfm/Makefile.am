

EXTRA_DIST = LICENSE
EXTRA_DIST += README
EXTRA_DIST += tfm.sln
EXTRA_DIST += *.vcxproj *.vcxproj.filters
EXTRA_DIST += *.vcproj

# autotools won't build the contrib stuff
# it is distributed in the contrib tarball as many platforms
# already provide things like cppunit
#EXTRA_DIST += contrib

SUBDIRS = .
SUBDIRS += repro
SUBDIRS += tfdum

AM_CXXFLAGS = -I $(top_srcdir)

lib_LTLIBRARIES = libtfm.la

libtfm_la_LDFLAGS = @LIBTOOL_VERSION_RELEASE@ -export-dynamic
libtfm_la_LIBADD = ../repro/librepro.la
libtfm_la_LIBADD += ../resip/dum/libdum.la
libtfm_la_LIBADD += ../resip/stack/libresip.la
libtfm_la_LIBADD += ../rutil/librutil.la
libtfm_la_LIBADD += @LIBSSL_LIBADD@ @LIBPTHREAD_LIBADD@ -ldb_cxx -lcppunit -lNetxx

libtfm_la_SOURCES = ActionBase.cxx \
        CppTestSelector.cxx \
        StunEndPoint.cxx \
        StunServer.cxx \
        EndPoint.cxx \
        TfmHelper.cxx \
        Expect.cxx \
        StunEvent.cxx \
        TfmDns.cxx \
        CPTestProgressListener.cxx \
        TestRtp.cxx \
        RtpEvent.cxx \
        CheckPrivacy.cxx \
        AsciiGraphic.cxx \
        Box.cxx \
        CheckContacts.cxx\
        CheckFetchedContacts.cxx \
        CountDown.cxx \
        DialogSet.cxx \
        DnsUtils.cxx \
        Event.cxx \
        ExpectAction.cxx \
        ExpectActionEvent.cxx \
        Renderer.cxx \
        Resolver.cxx \
        RouteGuard.cxx \
        Sequence.cxx \
        SequenceSet.cxx \
        SequenceSetThread.cxx \
        SipEvent.cxx \
        SipRawMessage.cxx \
        Source.cxx \
        TelnetClient.cxx \
        TestEndPoint.cxx \
        TestException.cxx \
        TestProxy.cxx \
        TestSipEndPoint.cxx \
        TestUser.cxx \
        TransportDriver.cxx \
        \
        predicates/ExpectUtils.cxx \
        predicates/SubscribeExpired.cxx \
        predicates/SubscribeExpires.cxx \
		predicates/NotifyContents.cxx

#libtfmincludedir = $(includedir)/repro
noinst_HEADERS =  ActionBase.hxx \
        AsciiGraphic.hxx \
        BindPointerAction.hxx \
        Box.hxx \
        CheckContacts.hxx \
        CheckFetchedContacts.hxx \
        CheckPrivacy.hxx \
        CommonAction.hxx \
        CountDown.hxx \
        CppTestSelector.hxx \
        CPTextTestProgressListener.hxx \
        DialogSet.hxx \
        DnsUtils.hxx \
        EndPoint.hxx \
        Event.hxx \
        ExpectActionEvent.hxx \
        ExpectAction.hxx \
        ExpectFunctorDefs.hxx \
        Expect.hxx \
        PortAllocator.hxx \
        predicates/ExpectUtils.hxx \
        predicates/NotifyContents.hxx \
        predicates/SubscribeExpired.hxx \
        predicates/SubscribeExpires.hxx \
        Renderer.hxx \
        Resolver.hxx \
        RouteGuard.hxx \
        RtpEvent.hxx \
        SeqDecls.hxx \
        SeqDefns.hxx \
        SequenceClassConstructorDecls.hxx \
        SequenceClassConstructorDefns.hxx \
        Sequence.hxx \
        SequenceSet.hxx \
        SequenceSetThread.hxx \
        SipEvent.hxx \
        SipRawMessage.hxx \
        Source.hxx \
        StunEndPoint.hxx \
        StunEvent.hxx \
        StunServer.hxx \
        SubDecls.hxx \
        SubDefns.hxx \
        TelnetClient.hxx \
        TestEndPoint.hxx \
        TestException.hxx \
        TestProxy.hxx \
        TestRtp.hxx \
        TestSipElement.hxx \
        TestSipEndPoint.hxx \
        TestUser.hxx \
        TfmDns.hxx \
        TfmHelper.hxx \
        TransportDriver.hxx \
        WrapperEvent.hxx

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

