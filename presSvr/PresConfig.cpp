#include "resiprocate/SipStack.hxx"
#include "PresConfig.h"
#include "ResourceMgr.h"

using namespace resip;
PresConfig* PresConfig::theInstance = 0;

PresConfig & 
PresConfig::instance()
{
  if (!theInstance)
  {
    theInstance = new PresConfig();
  }
  return (*theInstance);
}

void
PresConfig::initializeStack(SipStack& stack, int argc, char **argv)
{
    int fromPort = (argc>2?atoi(argv[2]):5060);
    stack.addTransport(Transport::UDP, fromPort);
    stack.addTransport(Transport::TCP, fromPort);
//    stack.addTransport(Transport::UDP, fromPort,Data("207.219.179.228"));
//    stack.addTransport(Transport::TCP, fromPort,Data("207.219.179.228"));
}

void
PresConfig::initializeResources(int argc, char **argv)
{
    ResourceMgr::instance()
      .addResource(Data("RjS@127.0.0.1"));
//    ResourceMgr::instance()
//      .addResource(Data("pekka@ds.sipit.net"));
}
