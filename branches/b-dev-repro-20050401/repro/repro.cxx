#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/StackThread.hxx"
#include "resiprocate/dum/InMemoryRegistrationDatabase.hxx"

#include "repro/Proxy.hxx"
#include "repro/RequestProcessorChain.hxx"
#include "repro/monkeys/RouteProcessor.hxx"
#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/LocationServer.hxx"
#include "repro/UserDb.hxx"
#include "repro/Registrar.hxx"


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

   RouteProcessor* rp = new RouteProcessor();
   requestProcessors.addProcessor(std::auto_ptr<RequestProcessor>(rp));

   DigestAuthenticator* da = new DigestAuthenticator();
   requestProcessors.addProcessor(std::auto_ptr<RequestProcessor>(da)); 
   
   LocationServer* ls = new LocationServer(regData);
   requestProcessors.addProcessor(std::auto_ptr<RequestProcessor>(ls));

   UserDb userDb;
   Proxy proxy(stack, requestProcessors, userDb);

   /* Initialize a registrar */
   Registrar registrar(stack, regData);

   /* Make it all go */
   stackThread.run();
   proxy.run();
   registrar.run();
   registrar.join();
   proxy.join();
   stackThread.join();
   
   // shutdown the stack now...
}
