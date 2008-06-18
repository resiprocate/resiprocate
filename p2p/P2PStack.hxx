#if !defined(P2PStack_hxx)
#define P2PStack_hxx

#include "p2p/Profile.hxx"
#include "p2p/Dispatcher.hxx"
#include "p2p/ChordTopology.hxx"
#include "p2p/SelectTransporter.hxx"
#include "p2p/ForwardingLayer.hxx"

namespace p2p
{

class P2PStack
{
   public:
      P2PStack(const Profile& profile);
      void run();
      
   private:
      Profile mConfig;

      Dispatcher mDispatcher;
      SelectTransporter mTransporter;
      Chord mChord;
      ForwardingLayer mForwarder;
};

   
}

#endif
