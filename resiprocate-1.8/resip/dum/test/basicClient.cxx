#if defined(HAVE_CONFIG_HXX)
#include "resip/stack/config.hxx"
#endif

#include <cstring>
#include <cassert>

#ifndef __APPLE__
bool TRUE=true;
bool FALSE=false;
#endif

#include "basicClientUserAgent.hxx"
#include <resip/stack/SipStack.hxx>
#include <resip/stack/Uri.hxx>
#include <rutil/Logger.hxx>

#ifdef USE_SSL
#include <resip/stack/ssl/Security.hxx>
#endif

#include <signal.h>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

static bool exitSignalDetected = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   exitSignalDetected = true;
}

int
main(int argc, char* argv[])
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
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

   // Initialize network
   initNetwork();

   try
   {
      BasicClientUserAgent ua(argc, argv);
      ua.startup();
      
      InfoLog(<< argv[0] << " starting");

      while(ua.process(1000))  // Look for exit key every 1 second
      {
         if(exitSignalDetected)
         {
            ua.shutdown();
            exitSignalDetected = false;
         }
      }
   }
#ifdef USE_SSL
   catch (BaseSecurity::Exception& e)
   {
      WarningLog (<< "Couldn't set up security object: " << e);
      exit(-1);
   }
#endif
   catch (BaseException& e)
   {
      ErrLog (<< "Caught: " << e);
      exit(-1);
   }
   catch( ... )
   {
      ErrLog( << "Caught non-resip exception" );
      exit(-1);
   }

   return 0;
}

/* ====================================================================

 Copyright (c) 2011, SIP Spectrum, Inc.
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

