#include "repro/Proxy.hxx"
#include "repro/monkeys/RouteProcessor.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"


using repro;
using resip;
using std;

int
main(int argc, char** argv)
{

/* Initialize a stack */
   Log::initialize(Log::COUT, Log::INFO, argv[0]);
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
