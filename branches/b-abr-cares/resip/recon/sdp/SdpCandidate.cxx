#include "SdpCandidate.hxx"

using namespace sdpcontainer;

const char* SdpCandidate::SdpCandidateTransportTypeString[] =
{
   "NONE",
   "UDP",
   "TCP-SO",
   "TCP-ACT",
   "TCP-PASS",
   "TLS-SO",
   "TLS-ACT",
   "TLS-PASS"
};

const char* SdpCandidate::SdpCandidateTypeString[] =
{
   "NONE",
   "HOST",
   "SRFLX",
   "PRFLX",
   "RELAY"
};

// Constructor
SdpCandidate::SdpCandidate(const char * foundation,
                           unsigned int id,
                           SdpCandidateTransportType transport,
                           UInt64 priority,
                           const char * connectionAddress,
                           unsigned int port,
                           SdpCandidateType candidateType,
                           const char * relatedAddress,
                           unsigned int relatedPort, 
                           bool inUse) :
      mFoundation(foundation),
      mId(id),
      mTransport(transport),
      mPriority(priority),
      mConnectionAddress(connectionAddress),
      mPort(port),
      mCandidateType(candidateType),
      mRelatedAddress(relatedAddress),
      mRelatedPort(relatedPort),
      mInUse(inUse)
{
}

// Copy constructor
SdpCandidate::SdpCandidate(const SdpCandidate& rhs)
{
   operator=(rhs);  
}

// Destructor
SdpCandidate::~SdpCandidate()
{
}

SdpCandidate& 
SdpCandidate::operator=(const SdpCandidate& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   // Assign values
   mFoundation = rhs.mFoundation;
   mId = rhs.mId;
   mTransport = rhs.mTransport;
   mPriority = rhs.mPriority;
   mConnectionAddress = rhs.mConnectionAddress;
   mPort = rhs.mPort;
   mCandidateType = rhs.mCandidateType;
   mRelatedAddress = rhs.mRelatedAddress;
   mRelatedPort = rhs.mRelatedPort;
   mInUse = rhs.mInUse;
   mExtensionAttributes = rhs.mExtensionAttributes;

   return *this;
}

bool 
SdpCandidate::operator==(const SdpCandidate& rhs) const
{
   return mFoundation == rhs.mFoundation &&
          mId == rhs.mId &&
          mTransport == rhs.mTransport &&
          mPriority == rhs.mPriority &&
          mConnectionAddress == rhs.mConnectionAddress &&
          mPort == rhs.mPort &&
          mCandidateType == rhs.mCandidateType &&
          mRelatedAddress == rhs.mRelatedAddress &&
          mRelatedPort == rhs.mRelatedPort &&
          mExtensionAttributes == rhs.mExtensionAttributes &&
          mInUse == rhs.mInUse;
}

bool 
SdpCandidate::operator!=(const SdpCandidate& rhs) const
{
   return !operator==(rhs);
}

bool 
SdpCandidate::operator<(const SdpCandidate& rhs) const
{
   if(mPriority != rhs.mPriority)
   {
      return mPriority > rhs.mPriority;  // We want to order a list of these from highest priority to lowest - so condition is reversed
   }
   
   // Priority should be unique, so we shouldn't get here, but implementation is included for completeness
   if(mFoundation != rhs.mFoundation)
   {
      return mFoundation < rhs.mFoundation;
   }

   if(mId != rhs.mId)
   {
      return mId < rhs.mId;
   }

   if(mTransport != rhs.mTransport)
   {
      return mTransport < rhs.mTransport;
   }
   
   if(mConnectionAddress != rhs.mConnectionAddress)
   {
      return mConnectionAddress < rhs.mConnectionAddress;
   }

   if(mPort != rhs.mPort)
   {
      return mPort < rhs.mPort;
   }

   if(mCandidateType != rhs.mCandidateType)
   {
      return mCandidateType < rhs.mCandidateType;
   }

   if(mRelatedAddress != rhs.mRelatedAddress)
   {
      return mRelatedAddress < rhs.mRelatedAddress;
   }

   if(mRelatedPort != rhs.mRelatedPort)
   {
      return mRelatedPort < rhs.mRelatedPort;
   }

   return false;  // equal
}

SdpCandidate::SdpCandidateTransportType 
SdpCandidate::getCandidateTransportTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("udp", dataType))
   {
      return CANDIDATE_TRANSPORT_TYPE_UDP;
   }
   else if(resip::isEqualNoCase("tcp-so", dataType))
   {
      return CANDIDATE_TRANSPORT_TYPE_TCP_SO;
   }
   else if(resip::isEqualNoCase("tcp-act", dataType))
   {
      return CANDIDATE_TRANSPORT_TYPE_TCP_ACT;
   }
   else if(resip::isEqualNoCase("tcp-pass", dataType))
   {
      return CANDIDATE_TRANSPORT_TYPE_TCP_PASS;
   }
   else if(resip::isEqualNoCase("tls-so", dataType))
   {
      return CANDIDATE_TRANSPORT_TYPE_TLS_SO;
   }
   else if(resip::isEqualNoCase("tls-act", dataType))
   {
      return CANDIDATE_TRANSPORT_TYPE_TLS_ACT;
   }
   else if(resip::isEqualNoCase("tls-pass", dataType))
   {
      return CANDIDATE_TRANSPORT_TYPE_TLS_PASS;
   }
   else
   {
      return CANDIDATE_TRANSPORT_TYPE_NONE;
   }
}

SdpCandidate::SdpCandidateType 
SdpCandidate::getCandidateTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("host", dataType))
   {
      return CANDIDATE_TYPE_HOST;
   }
   else if(resip::isEqualNoCase("srflx", dataType))
   {
      return CANDIDATE_TYPE_SRFLX;
   }
   else if(resip::isEqualNoCase("prflx", dataType))
   {
      return CANDIDATE_TYPE_PRFLX;
   }
   else if(resip::isEqualNoCase("relay", dataType))
   {
      return CANDIDATE_TYPE_RELAY;
   }
   else
   {
      return CANDIDATE_TYPE_NONE;
   }
}

EncodeStream& 
sdpcontainer::operator<<( EncodeStream& strm, const SdpCandidate& sdpCandidate)
{
   strm << "SdpCandidate: foundation=" << sdpCandidate.mFoundation
        << ", id=" << sdpCandidate.mId
        << ", transport=" << SdpCandidate::SdpCandidateTransportTypeString[sdpCandidate.mTransport]
        << ", priority=" << sdpCandidate.mPriority 
        << ", addr=" << sdpCandidate.mConnectionAddress
        << ", port=" << sdpCandidate.mPort
        << ", type=" << SdpCandidate::SdpCandidateTypeString[sdpCandidate.mCandidateType]
        << ", relatedAddr=" << sdpCandidate.mRelatedAddress 
        << ", relatedPort=" << sdpCandidate.mRelatedPort
        << ", ";

   SdpCandidate::SdpCandidateExtensionAttributeList::const_iterator it = sdpCandidate.mExtensionAttributes.begin();
   for(;it != sdpCandidate.mExtensionAttributes.end(); it++)
   {
      strm << it->getName() << "=" << it->getValue() << ", ";
   }

   strm << "inUse=" << sdpCandidate.mInUse << std::endl;
   return strm;
}


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
