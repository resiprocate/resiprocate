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
  SipStack stack();
  stack.addTransport(UDP,5060);
  stack.addTransport(TCP,5060);

/* Initialize a proxy */
  RequestProcessorChain requestProcessors;

  RouteProcessor rp();
  requestProcessors.addProcessor(rp);

  DigestAuthenticator da();
  requestProcessors.addProcessor(rp); 
   
  InMemoryRegistrationDatabase regData();
  LocationServer ls(regData);
  requestProcessors.addProcessor(rp);

  Proxy theProxyTU(stack,requestProcessors);

/* Initialize a registrar */
  Registrar registrar();
  Profile   profile();
  
  profile.clearSupportedMethods();
  profile.addSupportedMethod(resip::REGISTER);
 
  DialogUsageManager registrarDum(stack);
  registrarDum.setServerRegistrationHandler(&registrar);
  registrarDum.setRegistrationPersistenceManager(&regData);
  registrarDum.setProfile(&profile);

/* Make it all go */
  theProxyTu.run();
  registrarDum.run();

  //!RjS! catch signals instead
  while (1)
  {
  }

}
