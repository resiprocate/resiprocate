#include "p2p/P2PStack.hxx"

using namespace p2p;

P2PStack::P2PStack(const Profile& profile) : 
   mConfig(mProfile),
   mDispatcher(),
   mChord(mProfile, mDispatcher),
   mTransporter(mProfile),
   mForwarder(mDispatcher, mTransporter, mChord)
{
}

void
P2PStack::run()
{
   while (1)
   {
      process(1000);
   }
}
