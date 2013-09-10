#include "StunTuple.hxx"

using namespace std;

namespace reTurn {

// Default constructor
StunTuple::StunTuple() :
   mTransport(None),
   mPort(0)
{
}

StunTuple::StunTuple(TransportType transport, const asio::ip::address& address, unsigned int port) :
   mTransport(transport),
   mAddress(address),
   mPort(port)
{
}

bool 
StunTuple::operator==(const StunTuple& rhs) const
{
   return mTransport == rhs.mTransport && 
          mAddress == rhs.mAddress &&
          mPort == rhs.mPort;
}

bool 
StunTuple::operator!=(const StunTuple& rhs) const
{
   return mTransport != rhs.mTransport ||
          mAddress != rhs.mAddress ||
          mPort != rhs.mPort;
}

bool 
StunTuple::operator<(const StunTuple& rhs) const
{
   if (mTransport < rhs.mTransport)
   {
      return true;
   }
   if (mTransport > rhs.mTransport)
   {
      return false;
   }
   if(mAddress < rhs.mAddress)
   {
      return true;
   }
   if(mAddress == rhs.mAddress)
   {
      return mPort < rhs.mPort;
   }
   return false;
}

EncodeStream&
operator<<(EncodeStream& strm, const StunTuple& tuple)
{
   switch(tuple.mTransport)
   {
   case StunTuple::None:
      strm << "[None ";
      break;
   case StunTuple::UDP:
      strm << "[UDP ";
      break;
   case StunTuple::TCP:
      strm << "[TCP ";
      break;
   case StunTuple::TLS:
      strm << "[TLS ";
      break;
   }
   strm << tuple.mAddress.to_string() << ":" << tuple.mPort << "]";
   return strm;
}

} // namespace


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
