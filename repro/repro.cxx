
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <signal.h>
#include "repro/ReproRunner.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace repro;
using namespace resip;
using namespace std;

static bool finished = false;
static bool receivedHUP = false;

static void
signalHandler(int signo)
{
#ifndef _WIN32
   if(signo == SIGHUP)
   {
      InfoLog(<<"Received HUP signal, logger reset");
      Log::reset();
      receivedHUP = true;
      return;
   }
#endif
   std::cerr << "Shutting down" << endl;
   finished = true;
}

/*
   Extending Repro by adding custom processors to the chain is as easy as overriding one of the 
   ReproRunner class virtual methods:
   virtual void addProcessor(repro::ProcessorChain& chain, std::auto_ptr<repro::Processor> processor);
   virtual void makeRequestProcessorChain(repro::ProcessorChain& chain);
   virtual void makeResponseProcessorChain(repro::ProcessorChain& chain);
   virtual void makeTargetProcessorChain(repro::ProcessorChain& chain);

   Override the makeXXXProcessorChain methods to add processors to the beginning or end of any chain,
   or override the addProcessor method, and you can examine the name of the processor being 
   added and add your own process either before or after the correct processor.  

   WARNING: Be careful when checking for names of optional processors.  Depending on
            the configuration some processors may not be enabled.

   Create an instance of your overridden ReproRunner class and call run to start everything 
   up.

   Example:

   class MyCustomProcessor : public Processor
   {
      public:
         MyCustomProcessor(ProxyConfig& config) : Processor("MyCustomProcessor") {}
         virtual ~MyCustomProcessor() {}
      
         virtual processor_action_t process(RequestContext &context)
         {  
            DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << context);
            
            ...Do something interesting here....

            return Processor::Continue;
         }
      }
   };

   class MyReproRunner : public ReproRunner
   {
   public:
      MyReproRunner() {}
      virtual ~MyReproRunner() {}
   
   protected:
      virtual void addProcessor(repro::ProcessorChain& chain, std::auto_ptr<repro::Processor> processor)
      {
         if(processor->getName() == "LocationServer")
         {
            // Add MyCustomProcessor before LocationServer
            addProcessor(chain, std::auto_ptr<Processor>(new MyCustomProcessor(*mProxyConfig)));
         }
         ReproRunner::addProcessor(chain, processor);  // call base class implementation
      }
   };

   There is also generic storage available for custom Processor at three different
   scope levels via a KeyValueStore:
     -Global Proxy Scope - Proxy::getKeyValueStore
     -Request Scope - RequestContext::getKeyValueStore
     -Target Scope - Target::getKeyValueStore
   Before this storage can be used you must statically allocate a storage key.
   See mFromTrustedNodeKey use in the IsTrustedNode class for an example.

*/

int
main(int argc, char** argv)
{
   // Install signal handlers
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
   if ( signal( SIGHUP, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGHUP" << endl;
      exit( -1 );
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

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   { FindMemoryLeaks fml;
#endif

   ReproRunner repro;
   if(!repro.run(argc, argv))
   {
      cerr << "Failed to start repro, exiting..." << endl;
      exit(-1);
   }

   // Main program thread, just waits here for a signal to shutdown
   while (!finished)
   {
      sleepMs(1000);
      if(receivedHUP)
      {
         repro.onHUP();
         receivedHUP = false;
      }
   }

   repro.shutdown();

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }
#endif
   return 0;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
/*
 * vi: set shiftwidth=3 expandtab:
 */
