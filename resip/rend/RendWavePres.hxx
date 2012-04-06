#if !defined(REND_WAVEPRES_HXX)
#define REND_WAVEPRES_HXX
/**
**/

#include "RendAcct.hxx"

namespace resip
{
   class SipStack;
}

class RendWaveAbortException : public std::exception 
{
public:
   RendWaveAbortException(const std::string what) throw() 
   {
      mWhat = what;
   }
   virtual ~RendWaveAbortException() throw();
   // const char *name() const { return "RendWaveAbortException"; }
   virtual const char *what() const throw() { return mWhat.c_str(); }

   std::string mWhat;
};

class RendWavePresIf 
{
public:
   RendWavePresIf() {};
   virtual ~RendWavePresIf();

   virtual void pause(int ms) = 0;
   virtual void makePubVector(int numAccts) = 0;
   virtual void makeSubMatrix(int numAccts, int numRep=1, int numStride=1, int maxPerUser=0) = 0;

   virtual void runPubWave(const char *lbl, int expireSecs, int f=0) = 0;
   virtual void runSubWave(const char *lbl, int expireSecs, int f=0) = 0;
   virtual void runCloseAll(const char *lbl, int f=0) = 0;
};

class RendTu;

class RendWavePresOpts : public RendOptsBase 
{
public:
   RendWavePresOpts()
      : mMaxInProgress(40), mMaxInProgressSub(0), mMaxInProgressPub(0), 
      mMaxFailPct(0), mPidfCheckMode(1),
      mSubExtraHdr(NULL),
      mProxy(NULL),
      mLocalTransportUri(NULL),
      mLocalNumPorts(1), mLocalBind(1),
      mKeepAliveIvalSecs(20),
      mMaxRetryReqCnt(0)
   { }

   int mMaxInProgress;
   int mMaxInProgressSub;
   int mMaxInProgressPub;
   int mMaxFailPct;
   int mPidfCheckMode;
   char* mSubExtraHdr;
   char* mProxy;
   char* mLocalTransportUri;
   int mLocalNumPorts;
   int mLocalBind; // 1 to bind
   int mKeepAliveIvalSecs;
   int mMaxRetryReqCnt;

   virtual struct poptOption* getPoptTbl();
   virtual const char* getPoptDesc() { return "Wave presence options"; }

   RendWavePresIf* createWavePres(RendTu *tu);
};

extern RendWavePresOpts TheRendWavePresOpts;

// class resip::SipStack;
class RendPresEng;
RendTu* RendTuCreate(resip::SipStack& stack, RendAcctMgrIf& acctMgr);
void RendTuDestroy(RendTu *tu);
//extern RendPresEng& RendPresEngCreate(RendTu *tu);
//extern RendWavePresIf& RendWavePresCreate(RendPresEng *presEng);
//RendWavePresIf* RendWavePresCreate(RendTu *tu);

RendWavePresIf* RendStartPresStack();
void RendStopPresStack();

#include "popt.h"
extern poptContext
RendParseOpts(const char *appName, int argc, char *argv[], RendOptsBase *testOpts);

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
