#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Security.hxx"

#include "repro/Proxy.hxx"
#include "repro/monkeys/RouteProcessor.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace repro;
using namespace resip;
using namespace std;

int
main(int argc, char** argv)
{

/* Initialize a stack */
   Log::initialize(Log::Cout, Log::Info, argv[0]);
   Security security;
   SipStack stack(&security);
   stack.addTransport(UDP,5060);
   stack.addTransport(TCP,5060);
   stack.addTransport(TLS,5061);
   StackThread stackThread(stack);

   InMemoryRegistrationDatabase regData;

   /* Initialize a proxy */
   RequestProcessorChain requestProcessors;

   RouteProcessor rp;
   requestProcessors.addProcessor(rp);

   DigestAuthenticator da;
   requestProcessors.addProcessor(rp); 
   
   LocationServer ls(regData);
   requestProcessors.addProcessor(rp);

   Proxy proxy(stack, requestProcessors);

   /* Initialize a registrar */
   Registrar registrar(stack, regData);

   /* Make it all go */
   stack.run();
   proxy.run();
   registrar.run();
   registrar.join();
   proxy.join();
   stack.join();
   
   // shutdown the stack now...
}
