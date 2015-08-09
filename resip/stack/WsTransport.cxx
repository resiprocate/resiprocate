#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <memory>
#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/WsTransport.hxx"
#include "resip/stack/WsConnection.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;

WsTransport::WsTransport(Fifo<TransactionMessage>& fifo, int portNum,
      IpVersion version, const Data& pinterface,
      AfterSocketCreationFuncPtr socketFunc,
      Compression &compression,
      unsigned transportFlags,
      SharedPtr<WsConnectionValidator> connectionValidator,
      SharedPtr<WsCookieContextFactory> cookieContextFactory)
: TcpBaseTransport(fifo, portNum, version, pinterface, socketFunc, compression, transportFlags),
  WsBaseTransport(connectionValidator, cookieContextFactory)
{
   mTuple.setType(WS);

   init();

   InfoLog (<< "Creating WS transport host=" << pinterface
         << " port=" << mTuple.getPort()
         << " ipv4=" << bool(version==V4) );

   mTxFifo.setDescription("WsTransport::mTxFifo");
}

WsTransport::~WsTransport()
{
}

Connection*
WsTransport::createConnection(const Tuple& who, Socket fd, bool server)
{
   resip_assert(this);
   Connection* conn = new WsConnection(this,who, fd, mCompression, mConnectionValidator);
   return conn;
}

/* ====================================================================
 *
 * Copyright 2012 Doubango Telecom.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

