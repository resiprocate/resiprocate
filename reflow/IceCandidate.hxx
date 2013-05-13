#if !defined(IceCandidate_hxx)
#define IceCandidate_hxx

#include "rutil/ParseBuffer.hxx"

namespace reTurn
{
class IceCandidate
{
public:
   enum CandidateType
   {
      CandidateType_Host,
      CandidateType_Srflx,
      CandidateType_Prflx,
      CandidateType_Relay,
      CandidateType_Unknown
   };

   IceCandidate() {}
   IceCandidate(
               const StunTuple& transportAddr, 
               CandidateType candidateType, 
               unsigned int priority, 
               const resip::Data& foundation, 
               unsigned int componentId, 
               const StunTuple& relatedAddr)
      : mTransportAddr(transportAddr),
        mCandidateType(candidateType),
        mPriority(priority),
        mFoundation(foundation),
        mComponentId(componentId),
        mRelatedAddr(relatedAddr)
   {
   }
   IceCandidate(const IceCandidate& rhs)
      : mTransportAddr(rhs.mTransportAddr),
        mCandidateType(rhs.mCandidateType),
        mPriority(rhs.mPriority),
        mFoundation(rhs.mFoundation),
        mComponentId(rhs.mComponentId),
        mRelatedAddr(rhs.mRelatedAddr)
   {
   }

   /*
   .jjg. untested, and currently unneeded
   IceCandidate(const resip::Data& sdpAttribute)
   {
      resip::ParseBuffer pb(sdpAttribute);

      pb.skipWhitespace();
      const char* start = pb.position();

      pb.skipNonWhitespace();
      pb.data(mFoundation, start);

      pb.skipWhitespace();
      resip::Data componentId;
      start = pb.position();
      pb.skipNonWhitespace();
      pb.data(componentId, start);
      mComponentId = (unsigned int)componentId.convertInt();

      pb.skipWhitespace();
      resip::Data transport;
      start = pb.position();
      pb.skipNonWhitespace();
      pb.data(transport, start);
      if (resip::isEqualNoCase(transport, "udp")) { mTransportAddr.setTransportType(StunTuple::UDP); }
      else if (resip::isEqualNoCase(transport, "tcp")) { mTransportAddr.setTransportType(StunTuple::TCP); }
      else { assert(0); }

      pb.skipWhitespace();
      resip::Data priority;
      start = pb.position();
      pb.skipNonWhitespace();
      pb.data(priority, start);
      mPriority = (unsigned int)priority.convertInt();

      pb.skipWhitespace();
      resip::Data connectionAddr;
      start = pb.position();
      pb.skipNonWhitespace();
      pb.data(connectionAddr, start);
      mTransportAddr.setAddress(asio::ip::address::from_string(connectionAddr.c_str()));

      pb.skipWhitespace();
      resip::Data port;
      start = pb.position();
      pb.skipNonWhitespace();
      pb.data(port, start);
      mTransportAddr.setPort(port.convertInt());

      pb.skipWhitespace();
      resip::Data candidateType;
      pb.skipChars("typ ");
      start = pb.position();
      pb.skipNonWhitespace();
      pb.data(candidateType, start);
      if (resip::isEqualNoCase(candidateType, "host")) { mCandidateType = CandidateType_Host; }
      else if (resip::isEqualNoCase(candidateType, "srflx")) { mCandidateType = CandidateType_Srflx; }
      else if (resip::isEqualNoCase(candidateType, "prflx")) { mCandidateType = CandidateType_Prflx; }
      else if (resip::isEqualNoCase(candidateType, "relay")) { mCandidateType = CandidateType_Relay; }
      else { mCandidateType = CandidateType_Unknown; }

      pb.skipWhitespace();
      if (!pb.eof())
      {
         resip::Data relatedAddr;
         start = pb.position();
         pb.skipNonWhitespace();
         pb.data(relatedAddr, start);
         mRelatedAddr.setTransportType(mTransportAddr.getTransportType());
         mRelatedAddr.setAddress(asio::ip::address::from_string(relatedAddr.c_str()));
      }

      pb.skipWhitespace();
      if (!pb.eof())
      {
         resip::Data relatedPort;
         start = pb.position();
         pb.skipNonWhitespace();
         pb.data(relatedPort, start);
         mRelatedAddr.setPort(relatedPort.convertInt());
      }
   }
   */

   virtual ~IceCandidate() {}

   const StunTuple& getTransportAddr() const { return mTransportAddr; }
   const resip::Data& getFoundation() const { return mFoundation; }
   unsigned int getComponentId() const { return mComponentId; }
   CandidateType getCandidateType() const { return mCandidateType; }

private:
   StunTuple mTransportAddr;
   CandidateType mCandidateType;
   unsigned int mPriority;
   resip::Data mFoundation;
   unsigned int mComponentId;
   StunTuple mRelatedAddr;
   
};
}

#endif // IceCandidate_hxx


/* ====================================================================

 Copyright (c) 2009, CounterPath, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of CounterPath nor the names of its contributors 
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
