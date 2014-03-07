#include <signal.h>

#include "AppSubsystem.hxx"
#include "Server.hxx"

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Time.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/BaseException.hxx>
#include <resip/stack/NameAddr.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace clicktocall;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::CLICKTOCALL

static bool finished = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down..." << endl;
   finished = true;
}

int 
main (int argc, char** argv)
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#else
#if defined(_DEBUG) && defined(LEAK_CHECK)
   resip::FindMemoryLeaks fml;
#endif
#endif

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }

   initNetwork();

   //////////////////////////////////////////////////////////////////////////////
   // Create Server
   //////////////////////////////////////////////////////////////////////////////
   {
      Server server(argc, argv);

      //////////////////////////////////////////////////////////////////////////////
      // Startup and run...
      //////////////////////////////////////////////////////////////////////////////

      server.startup();

      while(true)
      {
         server.process(50);
         if(finished) break;
      }

      server.shutdown();
   }

   InfoLog(<< "ClickToCall server is shutdown.");
   sleepSeconds(2);
}

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

