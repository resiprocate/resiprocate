#if !defined(MediaRelayPort_hxx)
#define MediaRelayPort_hxx 

#include <map>
#include <memory>
#include <rutil/Data.hxx>
#include <rutil/Random.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>
#include <resip/stack/Tuple.hxx>

namespace gateway
{

class MediaEndpoint
{
public:
   MediaEndpoint() : mSsrc(resip::Random::getRandom()), mRelayDatagramLen(0), mSendTimeMs(0), mRecvTimeMs(0), mKeepaliveMode(false) {}

   resip::Tuple mTuple;
   unsigned int mSsrc;
   std::auto_ptr<char> mRelayDatagram;
   int mRelayDatagramLen;
   UInt64 mSendTimeMs;
   UInt64 mRecvTimeMs;
   bool mKeepaliveMode;

   void reset() 
   { 
      mTuple = resip::Tuple();
      mRelayDatagram.release();
      mRelayDatagramLen = 0; 
      mKeepaliveMode = false;
   }
};

class MediaRelayPort
{
public:
   MediaRelayPort();
   MediaRelayPort(resip::Socket& v4fd, resip::Tuple& v4tuple, resip::Socket& v6fd, resip::Tuple& v6tuple);
   MediaRelayPort(resip::Socket& v4fd, resip::Tuple& v4tuple);
   ~MediaRelayPort(); 

   // V4 and V6 fd's and tuples
   resip::Socket mV4Fd;
   resip::Tuple mLocalV4Tuple;

   resip::Socket mV6Fd;
   resip::Tuple mLocalV6Tuple;

   // Sender data
   MediaEndpoint mFirstEndpoint;
   MediaEndpoint mSecondEndpoint;
};

}

#endif  

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

