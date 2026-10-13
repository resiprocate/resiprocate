#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL 

#ifndef DTLS_Timer_hxx
#define DTLS_Timer_hxx

#include <memory>
#include <mutex>

namespace dtls
{

class DtlsTimer
{
   public:
      DtlsTimer(unsigned int seq); 
      virtual ~DtlsTimer();
      
      virtual void expired()=0;
      virtual void fire();
      unsigned int getSeq() { return mSeq; }
      //invalid could call though to context and call an external cancel
      void invalidate() { mValid = false; }

      // The lock the timer's owner holds while it uses what the timer acts on, and while it
      // invalidates the timer: fire() takes it too, so a timer firing on another thread neither
      // races the owner nor runs for an owner that has invalidated it meanwhile.  Shared, so it
      // outlives the owner while the timer is pending.
      void setLock(std::shared_ptr<std::mutex> lock) { mLock = std::move(lock); }
   private:
      unsigned int mSeq;
      bool mValid;
      std::shared_ptr<std::mutex> mLock;
};


class DtlsTimerContext
{     
   public:
      virtual ~DtlsTimerContext() {}
      virtual void addTimer(DtlsTimer* timer, unsigned int waitMs)=0;
      //could add cancel here
   protected:
      void fire(DtlsTimer *timer);
};
   

}

#endif

#endif
/* ====================================================================

 Copyright (c) 2007-2026, SIP Spectrum, Inc. http://sipspectrum.com
 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:
 
 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 
 
 3. None of the contributors names may be used to endorse or promote 
    products derived from this software without specific prior written 
    permission. 
 
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
