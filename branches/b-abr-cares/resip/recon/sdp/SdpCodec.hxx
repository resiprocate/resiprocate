#if !defined(SdpCodec_hxx)
#define SdpCodec_hxx

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"

namespace sdpcontainer
{

class SdpCodec
{
public:

   SdpCodec(unsigned int payloadType,
            const char* mimeType,
            const char* mimeSubType,
            unsigned int rate,
            unsigned int packetTime,
            unsigned int numChannels,
            const char* formatParameters);

   SdpCodec(const SdpCodec& rSdpCodec);

   virtual ~SdpCodec();

   SdpCodec& operator=(const SdpCodec& rhs);

   void setPayloadType(unsigned int payloadType) { mPayloadType = payloadType; }
   void setMimeType(const resip::Data& mimeType) { mMimeType = mimeType; }
   void setMimeSubtype(const resip::Data& mimeSubtype) { mMimeSubtype = mimeSubtype; }
   void setRate(unsigned int rate) { mRate = rate; }
   void setNumChannels(unsigned int numChannels) { mNumChannels = numChannels; }
   void setPacketTime(unsigned int packetTime) { mPacketTime = packetTime; }
   void setFormatParameters(const resip::Data& formatParameters) { mFormatParameters = formatParameters; }

   unsigned int getPayloadType() const { return mPayloadType; }
   const resip::Data& getMimeType() const { return mMimeType; }   
   const resip::Data& getMimeSubtype() const { return mMimeSubtype; }   
   unsigned int getRate() const { return mRate; }
   unsigned int getNumChannels() const { return mNumChannels; }
   unsigned int getPacketTime() const { return mPacketTime; }
   const resip::Data& getFormatParameters() const { return mFormatParameters; }   

   void toString(resip::Data& sdpCodecString) const;

private:
    unsigned int mPayloadType;      
    resip::Data mMimeType;        
    resip::Data mMimeSubtype;     
    unsigned int mRate;              
    unsigned int mPacketTime; // ptime
    unsigned int mNumChannels;
    resip::Data mFormatParameters; 

    friend EncodeStream& operator<<(EncodeStream& strm, const SdpCodec& );
};

EncodeStream& operator<<(EncodeStream& strm, const SdpCodec& );

} // namespace

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
