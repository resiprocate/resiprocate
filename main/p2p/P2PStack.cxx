#include "p2p/P2PStack.hxx"

using namespace p2p;

P2PStack::P2PStack(const Profile& profile) : 
   mConfig(mProfile),
   mDispatcher(),
   mTransporter(mProfile),
   mChord(mProfile, mTransporter, mDispatcher),
   mForwarder(mDispatcher, mTransporter, mChord)
{
   mDispatcher.setForwardingLayer(mForwarder);
}

void
P2PStack::run()
{
   while (1)
   {
      process(1000);
   }
}
