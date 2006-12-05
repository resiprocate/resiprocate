#include "precompile.h"
#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <memory>

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/TlsTransport.hxx"
#include "resip/stack/TlsConnection.hxx"
#include "resip/stack/Security.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

TlsTransport::TlsTransport(Fifo<TransactionMessage>& fifo, 
                           int portNum, 
                           IpVersion version,
                           const Data& interfaceObj,
                           Security& security,
                           const Data& sipDomain, 
                           SecurityTypes::SSLType sslType,
                           Compression &compression):
   TcpBaseTransport(fifo, portNum, version, interfaceObj, compression ),
   mSecurity(&security),
   mSslType(sslType)
{
   setTlsDomain(sipDomain);   
   mTuple.setType(transport());

   InfoLog (<< "Creating TLS transport for domain " 
            << sipDomain << " interface=" << interfaceObj 
            << " port=" << portNum);
}


TlsTransport::~TlsTransport()
{
}
  

Connection* 
TlsTransport::createConnection(Tuple& who, Socket fd, bool server)
{
   assert(this);
   who.transport = this;
   assert(  who.transport );

   Connection* conn = new TlsConnection(who, fd, mSecurity, server,
                                        tlsDomain(), mSslType, mCompression );
   assert( conn->transport() );
   return conn;
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
