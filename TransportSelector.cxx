#include <sipstack/TransportSelector.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/UdpTransport.hxx>

using namespace Vocal2;


TransportSelector::TransportSelector(int portNum)
  :mPortNum(portNum)
{
  mUdp = new UdpTransport(mPortNum, mRxFifo);
}

void 
TransportSelector::process()
{
  
  mUdp->process();

}

void 
TransportSelector::send( SipMessage& msg )
{
  
  mUdp->send(msg);

}


