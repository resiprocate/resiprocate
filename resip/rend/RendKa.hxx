#if !defined(REND_KA_HXX)
#define REND_KA_HXX

#include "resip/stack/SipStack.hxx"
#include "RendMisc.hxx"

struct RendKaStats 
{
   RendKaStats() : mSentMsgCnt(0), mSessionCnt(0), 
      mNumCurConn(0), mNumCurSession(0) { }
   unsigned mSentMsgCnt;
   unsigned mSessionCnt;
   unsigned mNumCurConn; // requires calc
   unsigned mNumCurSession;
};

class RendKaMgrIf 
{
public:
   virtual ~RendKaMgrIf() { };

   static RendKaMgrIf* createMgr(resip::SipStack& stack, int ivalSecs);

   virtual RendLocalKey addConn(RendTimeUs now, const resip::Tuple& target) = 0;
   virtual void delConn(RendLocalKey key) = 0;
   virtual RendTimeUs processTimers(RendTimeUs now) = 0;
   virtual void getStats(RendKaStats& stats) = 0;
};

#endif

/* ====================================================================

 Copyright (c) 2011, Logitech, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Logitech nor the names of its contributors 
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
