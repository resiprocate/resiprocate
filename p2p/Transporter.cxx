#include "rutil/GenericIPAddress.hxx"

#include "p2p/Transporter.hxx"
#include "p2p/ConfigObject.hxx"
#include "p2p/TransporterMessage.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"

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

class SendReloadCommand : public TransporterCommand
{
   public:
      SendReloadCommand(Transporter *transporter,
                        NodeId nodeId,
                        std::auto_ptr<p2p::Message> message)
        : TransporterCommand(transporter), 
          mNodeId(nodeId),
          mMessage(message) {;}

      void operator()() { mTransporter->send(mNodeId, mMessage); }

   private:
      NodeId mNodeId;
      std::auto_ptr<p2p::Message> mMessage;
};

//----------------------------------------------------------------------

Transporter::Transporter (resip::Fifo<TransporterMessage>& rxFifo,
                          ConfigObject &configuration)
  : mRxFifo(rxFifo)
{
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
  mCmdFifo.add(new SendReloadCommand(this, nodeId, msg));
}

/*  
void
Transporter::send(FlowId flowId, std::auto_ptr<resip::Data> data)
{
  mCmdFifo.add(new SendApplicationCommand(this, flowId, msg));
}
  
void
Transporter::collectLocalCandidates()
{
  mCmdFifo.add(new CollectLocalCandidatesCommand(this));
}
  
void
Transporter::connect(NodeId nodeId,
                   std::vector<Candidate> remoteCandidates,
                   resip::GenericIPAddress &stunTurnServer)
{
  mCmdFifo.add(new ConnectReloadComand(noteId, remoteCandidates, stunTurnServer));
}
  
void
Transporter::connect(NodeId nodeId,
                   std::vector<Candidate> remoteCandidates,
                   unsigned short application,
                   resip::GenericIPAddress &stunTurnServer)
{
  mCmdFifo.add(new ConnectApplicationComand(remoteCandidates, application,
    stunTurnServer));
}
*/

bool
Transporter::process(int ms)
{
  TransporterCommand *cmd;
  if ((cmd = mCmdFifo.getNext(ms)) != 0)
  {
    (*cmd)();
    return true;
  }
  return false;
}

}
