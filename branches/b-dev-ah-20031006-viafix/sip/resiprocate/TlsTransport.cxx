#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <memory>

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TlsTransport.hxx"
#include "resiprocate/TlsConnection.hxx"
#include "resiprocate/Security.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

TlsTransport::TlsTransport(Fifo<Message>& fifo, 
                           const Data& sipDomain, 
                           const Data& interfaceObj, 
                           int portNum, 
                           const Data& keyDir, const Data& privateKeyPassPhrase,
                           bool ipv4) : 
   TcpBaseTransport(fifo, portNum, interfaceObj, ipv4),
   mDomain(sipDomain),
   mSecurity(new Security(true, true))
{
   InfoLog (<< "Creating TLS transport for domain " 
            << sipDomain << " interface=" << interfaceObj 
            << " port=" << portNum);
   
   bool ok = true;
   ok = mSecurity->loadRootCerts(  mSecurity->getPath( keyDir, "root.pem")) ? ok : false;
   ok = mSecurity->loadMyPublicCert( mSecurity->getPath( keyDir , mDomain + "_cert.pem")) ? ok : false;
   ok = mSecurity->loadMyPrivateKey( privateKeyPassPhrase, mSecurity->getPath( keyDir , mDomain + "_key.pem")) ? ok : false;
   
   InfoLog( << "Listening for TLS connections on port " << portNum );
}


TlsTransport::~TlsTransport()
{
}

Connection* 
TlsTransport::createConnection(Tuple& who, Socket fd, bool server)
{
   who.transport = this;
   return new TlsConnection(who, fd, mSecurity, server);
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
