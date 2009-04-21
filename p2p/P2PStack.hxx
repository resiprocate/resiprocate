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

      void run(); // this never returns 
      void process(int waitTimeMS); // run a time slice and return 
      
      void listenOn(int port);
      void join();

      ResourceId getIdFromResourceName( const resip::Data& resourceName );
      
      
   private:
      Profile mProfile;

      Dispatcher mDispatcher;
      SelectTransporter mTransporter;
      ChordTopology mChord; // TODO - this needs to be changed to a Topology
                            // object 
      ForwardingLayer mForwarder;
};

   
}

#endif
