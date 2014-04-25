#ifndef STUNTUPLE_HXX
#define STUNTUPLE_HXX

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <rutil/resipfaststreams.hxx>

namespace reTurn {

class StunTuple
{
public:
   typedef enum
   {
      None,
      UDP,
      TCP,
      TLS
   } TransportType;

   explicit StunTuple();
   explicit StunTuple(TransportType transport, const asio::ip::address& address, unsigned int port);

   bool operator==(const StunTuple& rhs) const;
   bool operator!=(const StunTuple& rhs) const;
   bool operator<(const StunTuple& rhs) const;

   TransportType getTransportType() const { return mTransport; }
   void setTransportType(TransportType transport) { mTransport = transport; }

   const asio::ip::address& getAddress() const { return mAddress; }
   void setAddress(const asio::ip::address& address) { mAddress = address; }

   unsigned int getPort() const { return mPort; }
   void setPort(unsigned int port) { mPort = port; }

private:
   TransportType mTransport;
   asio::ip::address mAddress;
   unsigned int mPort;

   friend EncodeStream& operator<<(EncodeStream& strm, const StunTuple& tuple);
};

EncodeStream& operator<<(EncodeStream& strm, const StunTuple& tuple);

} 

#endif


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
