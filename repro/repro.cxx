#include <signal.h>
#include "repro/ReproRunner.hxx"
#include "rutil/Socket.hxx"

#include "rutil/WinLeakCheck.hxx"

using namespace repro;
using namespace resip;
using namespace std;

static bool finished = false;

static void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   finished = true;
}

/*
   Extending Repro by adding custom processors to the chain is as easy as overriding the 
   ReproRunner class virtual methods makeRequestProcessorChain, makeResponseProcessorChain 
   and/or makeTargetProcessorChain and adding your Processor to the chain.  Create
   an instance of your overridden ReproRunner class and call run to start everything 
   up.

   Example:

   class MyCustomProcessor : public Processor
   {
      public:
         MyCustomProcessor(ProxyConfig& config) {}
         virtual ~MyCustomProcessor() {}
      
         virtual processor_action_t process(RequestContext &context)
         {  
            DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << context);
            
            ...Do something interesting here....

            return Processor::Continue;
         }
      }
       
      virtual void dump(EncodeStream &os) const
      {
         os << "MyCustomProcessor Monkey" << std::endl;
      }
   };

   class MyReproRunner : public ReproRunner
   {
   public:
      MyReproRunner() {}
      virtual ~MyReproRunner() {}
   
   protected:
      virtual void makeRequestProcessorChain(repro::ProcessorChain& chain)
      {
         ReproRunner::makeRequestProcessorChain(chain);
         chain.addProcessor(std::auto_ptr<Processor>(new MyCustomProcessor(*mProxyConfig))); 
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
#ifdef WIN32
   Sleep(1000);
#else
   usleep(100000);
#endif
   }

   repro.shutdown();

#if defined(WIN32) && defined(_DEBUG) && defined(LEAK_CHECK) 
   }
#endif
}

/*
 * vi: set shiftwidth=3 expandtab:
 */
