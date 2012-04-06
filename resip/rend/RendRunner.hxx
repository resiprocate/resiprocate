#if !defined(REND_RUNNER_HXX)
#define REND_RUNNER_HXX 1
/**
  * REND Runner
  *
  * Object which owns the stack, Tu, engine, waves, etc. and
  * joins them all together.
  * 
  * This header is processed by swig to create python interface,
  * so keep it simple. Avoid function overloading.
  *
  * Written by Kennard White (Logitech, Inc.) over 2010..2011
**/

#include "RendReport.hxx"

class RendRunnerIf 
{
public:
   RendRunnerIf() { };
   virtual ~RendRunnerIf();

   virtual void setSketch(const char *sketchName) = 0;
   virtual void setArgs(const char *appName, int argc, char **argv) = 0;
   virtual void start() = 0;
   virtual void stop() = 0;

   // called from within signal handler to request sketch to stop
   virtual void signalStop(int urgency) = 0;

   /*
    Waits at most {waitMs} for underlying sketch to finish. May
    return sooner than {waitMs} if already finished, or finishes
    while waiting. Returns true iff sketch is finished.
    NOTE: a negative value of {waitMs} is current treated as 0 (no wait).
   */
   virtual bool waitForFinish(int waitMs) = 0;

   virtual int getTroopReport(RendTroopReport& rpt) = 0;
   virtual int setWorkVolume(RendWorkVolume& vol) = 0;

   virtual int doSimpleAction(RendSimpleAction act) = 0;

};

extern RendRunnerIf* RendRunnerCreate();

#endif // end-of-header

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
