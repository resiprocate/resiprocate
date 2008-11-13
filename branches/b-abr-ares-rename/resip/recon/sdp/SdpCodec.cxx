#include "SdpCodec.hxx"

using namespace sdpcontainer;

SdpCodec::SdpCodec(unsigned int payloadType,
                   const char* mimeType,
                   const char* mimeSubtype,
                   unsigned int rate,
                   unsigned int packetTime,
                   unsigned int numChannels,
                   const char* formatParameters) :
   mPayloadType(payloadType),
   mMimeType(mimeType),
   mMimeSubtype(mimeSubtype),
   mRate(rate),
   mPacketTime(packetTime),
   mNumChannels(numChannels),
   mFormatParameters(formatParameters)
{
}

SdpCodec::SdpCodec(const SdpCodec& rSdpCodec)
{
   operator=(rSdpCodec); 
}

SdpCodec::~SdpCodec()
{
}

SdpCodec&
SdpCodec::operator=(const SdpCodec& rhs)
{
   if (this == &rhs)            
      return *this;

   mPayloadType = rhs.mPayloadType;
   mMimeType = rhs.mMimeType;
   mMimeSubtype = rhs.mMimeSubtype;
   mRate = rhs.mRate;
   mPacketTime = rhs.mPacketTime;
   mNumChannels = rhs.mNumChannels;
   mFormatParameters = rhs.mFormatParameters;

   return *this;
}

EncodeStream& 
sdpcontainer::operator<<( EncodeStream& strm, const SdpCodec& sdpCodec)
{
   strm << "SdpCodec: payloadId=" << sdpCodec.mPayloadType
        << ", mime=" << sdpCodec.mMimeType << "/" << sdpCodec.mMimeSubtype 
        << ", rate=" << sdpCodec.mRate
        << ", packetTime=" << sdpCodec.mPacketTime
        << ", numCh=" << sdpCodec.mNumChannels 
        << ", fmtParam=" << sdpCodec.mFormatParameters << std::endl;
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

