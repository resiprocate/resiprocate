#include <sipstack/TransportSelector.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/UdpTransport.hxx>
#include <sipstack/SipStack.hxx>

using namespace Vocal2;


TransportSelector::TransportSelector(SipStack& stack) :
   mStack(stack),
   mUdp(new UdpTransport(5060, stack.mStateMacFifo))
{
}

void 
TransportSelector::process()
{
  mUdp->process();
}

void 
TransportSelector::send( SipMessage* msg )
{
   //mUdp->send(msg);
   assert(0);
}


