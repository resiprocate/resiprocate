#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include "rutil/SelectInterruptor.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/ThreadIf.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class FakeApp : public ThreadIf
{
   public: 
      FakeApp(SelectInterruptor& s);
      ~FakeApp() {};
      
      void thread();
   private:
      SelectInterruptor& mSi;
};

FakeApp::FakeApp(SelectInterruptor& s) : mSi(s)
{}

void FakeApp::thread()
{
    static unsigned wakeups[6] = { 3, 1, 0, 2, 5, 1 };

    for (unsigned long n = 0; n < sizeof(wakeups)/sizeof(long); n++)
    {
       InfoLog( << "Wakeup in: " << wakeups[n] << ", " << n+1 << " of " 
                << sizeof(wakeups)/sizeof(long));
#ifdef WIN32
	   Sleep(wakeups[n]*1000);
#else
	   sleep(wakeups[n]);
#endif
       InfoLog(<< "Waking up select");
       mSi.interrupt();
    }
    shutdown();
}


int
main(int argc, char* argv[])
{
#ifdef WIN32
	initNetwork();
#endif

   Log::initialize(Log::Cout, Log::Debug, argv[0]);
   
   SelectInterruptor si;
   FakeApp app(si);

   InfoLog(<< "Starting FakeApp");
   app.run();

   int numWakeups = 0;
   while(!app.isShutdown())
   {
      FdSet fdset;
      si.buildFdSet(fdset);
      
      int ret = fdset.selectMilliSeconds(10000);
      
      if (ret > 0)
      {
         InfoLog(<< "Select detected: " << ret << " ready descriptors");
         si.process(fdset);
         numWakeups++;
      }
      else
      {
         InfoLog(<< "Select detected no ready descriptors, test failed");
         break;
      }
   }

   if (numWakeups == 6)
   {
      InfoLog(<< "Finished, test passed");
   }
   app.shutdown();
   app.join();
}
         
   
            


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
