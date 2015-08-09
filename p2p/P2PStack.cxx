
#include "p2p/P2PStack.hxx"
#include <rutil/DnsUtil.hxx>

using namespace p2p;

P2PStack::P2PStack(const Profile& profile) : 
   mProfile(profile),
   mDispatcher(),
   mTransporter(mProfile),
   mChord(mProfile, mDispatcher, mTransporter),
   mForwarder(mProfile, mDispatcher, mTransporter, mChord)
{
   mDispatcher.init(mForwarder);
}

void
P2PStack::run()
{
   while (1)
   {
      resip_assert(0);
      //process(1000);
   }

}

void
P2PStack::process(int waitTimeMS)
{
   mTransporter.process(waitTimeMS);
   mForwarder.process(waitTimeMS);
}


void
P2PStack::join()
{
   mChord.joinOverlay();
}


void 
P2PStack::listenOn(int port)
{
   resip::TransportType transport = resip::TCP;

   struct in_addr addr;
   if(resip::DnsUtil::inet_pton("0.0.0.0", addr)==0)
   {
      resip_assert(0);
   }
   
   sockaddr_in addr_in;
   addr_in.sin_family = AF_INET;
   addr_in.sin_addr = addr;
   addr_in.sin_port = htons(port);    

   resip::GenericIPAddress address(addr_in);
   mTransporter.addListener(transport,address);
}


ResourceId 
P2PStack::getIdFromResourceName( const resip::Data& resourceName )
{
   return mChord.resourceId(resourceName);
}
