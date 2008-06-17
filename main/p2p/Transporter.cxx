#include "rutil/GenericIPAddress.hxx"

#include "p2p/Transporter.hxx"
#include "p2p/ConfigObject.hxx"
#include "p2p/TransporterMessage.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"
#include "p2p/Candidate.hxx"

namespace p2p
{

//----------------------------------------------------------------------
// Command Objects from here to the next line -- Transporter methods
// come after that.

class TransporterCommand
{
   public:
     TransporterCommand(Transporter *transporter) : mTransporter(transporter) {;}

     virtual ~TransporterCommand();

     virtual void operator()() = 0;

   protected:
      Transporter *mTransporter;
};

class AddListenerCommand : public TransporterCommand
{
   public:
      AddListenerCommand(Transporter *transporter,
                         resip::TransportType transport,
                         resip::GenericIPAddress &address)
        : TransporterCommand(transporter), 
          mTransport(transport),
          mAddress(address) {;}

      void operator()() { mTransporter->addListenerImpl(mTransport, mAddress); }

   private:
     resip::TransportType mTransport;
     resip::GenericIPAddress mAddress;
};

class SendP2pCommand : public TransporterCommand
{
   public:
      SendP2pCommand(Transporter *transporter,
                        NodeId nodeId,
                        std::auto_ptr<p2p::Message> message)
        : TransporterCommand(transporter), 
          mNodeId(nodeId),
          mMessage(message) {;}

      void operator()() { mTransporter->sendImpl(mNodeId, mMessage); }

   private:
      NodeId mNodeId;
      std::auto_ptr<p2p::Message> mMessage;
};

class SendApplicationCommand : public TransporterCommand
{
   public:
      SendApplicationCommand(Transporter *transporter,
                        FlowId flowId,
                        std::auto_ptr<resip::Data> message)
        : TransporterCommand(transporter), 
          mFlowId(flowId),
          mMessage(message) {;}

      void operator()() { mTransporter->sendImpl(mFlowId, mMessage); }

   private:
      FlowId mFlowId;
      std::auto_ptr<resip::Data> mMessage;
};

class CollectCandidatesCommand : public TransporterCommand
{
   public:
      CollectCandidatesCommand(Transporter *transporter)
        : TransporterCommand(transporter) {;}

      void operator()() { mTransporter->collectCandidatesImpl(); }

   private:
};

class ConnectP2pCommand : public TransporterCommand
{
   public:
      ConnectP2pCommand(Transporter *transporter,
                        NodeId nodeId,
                        std::vector<Candidate> remoteCandidates,
                        resip::GenericIPAddress &stunTurnServer)
        : TransporterCommand(transporter), 
          mNodeId(nodeId),
          mRemoteCandidates(remoteCandidates),
          mStunTurnServer (stunTurnServer) {;}

      void operator()() { mTransporter->connectImpl(mNodeId, mRemoteCandidates, mStunTurnServer); }

   private:
      NodeId mNodeId;
      std::vector<Candidate> mRemoteCandidates;
      resip::GenericIPAddress mStunTurnServer;
};

class ConnectApplicationCommand : public TransporterCommand
{
   public:
      ConnectApplicationCommand(Transporter *transporter,
                   NodeId nodeId,
                   std::vector<Candidate> remoteCandidates,
                   unsigned short application,
                   resip::GenericIPAddress &stunTurnServer)

        : TransporterCommand(transporter), 
          mNodeId(nodeId),
          mRemoteCandidates(remoteCandidates),
          mApplication(application),
          mStunTurnServer (stunTurnServer) {;}

      void operator()() { mTransporter->connectImpl(mNodeId, mRemoteCandidates, mApplication, mStunTurnServer); }

   private:
      NodeId mNodeId;
      std::vector<Candidate> mRemoteCandidates;
      unsigned short mApplication;
      resip::GenericIPAddress mStunTurnServer;
};

//----------------------------------------------------------------------

Transporter::Transporter (resip::Fifo<TransporterMessage>& rxFifo,
                          ConfigObject &configuration)
  : mRxFifo(rxFifo), mConfiguration(configuration)
{
   // We really shouldn't do this in the constructor -- but it goes
   // away when we add ICE.
   mTcpDescriptor = ::socket(AF_INET, SOCK_STREAM, 0);
}

void
Transporter::addListener(resip::TransportType transport,
                 resip::GenericIPAddress &address)
{
  mCmdFifo.add(new AddListenerCommand(this, transport, address));
}

void
Transporter::send(NodeId nodeId, std::auto_ptr<p2p::Message> msg)
{
  mCmdFifo.add(new SendP2pCommand(this, nodeId, msg));
}

void
Transporter::send(FlowId flowId, std::auto_ptr<resip::Data> msg)
{
  mCmdFifo.add(new SendApplicationCommand(this, flowId, msg));
}
  
void
Transporter::collectCandidates()
{
  mCmdFifo.add(new CollectCandidatesCommand(this));
}
  
void
Transporter::connect(NodeId nodeId,
                   std::vector<Candidate> remoteCandidates,
                   resip::GenericIPAddress &stunTurnServer)
{
  mCmdFifo.add(new ConnectP2pCommand(this, nodeId, remoteCandidates, stunTurnServer));
}
  
void
Transporter::connect(NodeId nodeId,
                   std::vector<Candidate> remoteCandidates,
                   unsigned short application,
                   resip::GenericIPAddress &stunTurnServer)
{
  mCmdFifo.add(new ConnectApplicationCommand(this, nodeId, remoteCandidates, application, stunTurnServer));
}

//----------------------------------------------------------------------
// Impls follow

/**
  @note This method is to be called by bootstrap nodes only -- all other
        modes need to use ICE-allocated addresses.
*/
void
Transporter::addListenerImpl(resip::TransportType transport,
                             resip::GenericIPAddress &address)
{
  // XXX
  assert(0);
}

void
Transporter::sendImpl(NodeId nodeId, std::auto_ptr<p2p::Message> msg)
{
  // XXX
  assert(0);
}

void
Transporter::sendImpl(FlowId flowId, std::auto_ptr<resip::Data> data)
{
  // XXX
  assert(0);
}

void
Transporter::collectCandidatesImpl()
{
  // For right now, we just return one candidate: a single TCP
  // listener.

  // XXX
  assert(0);
}

void
Transporter::connectImpl(NodeId nodeId,
                         std::vector<Candidate> remoteCandidates,
                         resip::GenericIPAddress &stunTurnServer)
{
  // XXX
  assert(0);
}

void
Transporter::connectImpl(NodeId nodeId,
                         std::vector<Candidate> remoteCandidates,
                         unsigned short application,
                         resip::GenericIPAddress &stunTurnServer)
{
  // XXX
  assert(0);
}

//----------------------------------------------------------------------

// This isn't anything like finished yet -- need to wait for data on our
// descriptors also.
bool
Transporter::process(int ms)
{
  TransporterCommand *cmd;
  if ((cmd = mCmdFifo.getNext(ms)) != 0)
  {
    (*cmd)();
    delete cmd;
    return true;
  }
  return false;
}

}
