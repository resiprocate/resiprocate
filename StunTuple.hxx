#ifndef STUNTUPLE_HXX
#define STUNTUPLE_HXX

#include <asio.hpp>

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

   friend std::ostream& operator<<(std::ostream& strm, const StunTuple& tuple);
};

std::ostream& operator<<(std::ostream& strm, const StunTuple& tuple);

} 

#endif


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */

