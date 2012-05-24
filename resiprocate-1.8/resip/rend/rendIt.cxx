/**
**/

#include <signal.h>

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#include "rutil/Logger.hxx"

#include "RendMisc.hxx"
#include "RendRunner.hxx"
#include "RendWavePres.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

static RendRunnerIf *TheRunner = NULL;
static int TheStopUrgency = 0;

static void signalHandler(int signo) 
{
   if ( TheRunner ) 
   {
      ++TheStopUrgency;
      TheRunner->signalStop(TheStopUrgency);
   }
}

int 
main(int argc, char *argv[]) 
{
   RendRunnerIf *runner = RendRunnerCreate();
   const char *lastArg = argc>=2 ? argv[argc-1] : NULL;
   runner->setSketch(lastArg && *lastArg!='-' ? lastArg : "help");
   runner->setArgs("rendIt", argc, argv);

#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      std::cerr << "Couldn't install signal handler for SIGPIPE" << std::endl;
      exit(-1);
   }
#endif

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      std::cerr << "Couldn't install signal handler for SIGINT" << std::endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      std::cerr << "Couldn't install signal handler for SIGTERM" << std::endl;
      exit( -1 );
   }

   TheRunner = runner;
   runner->start();
   InfoLog(<<"Starting main loop...");
   while ( ! runner->waitForFinish(REND_S2MS(5)) ) 
   {
      fprintf(stdout, "Main loop: ping.\n");
   }
   InfoLog(<<"Main loop complete.");
   runner->stop();
   TheRunner = NULL;
   delete runner;
   return 0;
}

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
