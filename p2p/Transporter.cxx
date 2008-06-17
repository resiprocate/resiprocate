#include "rutil/GenericIPAddress.hxx"
#include "rutil/DnsUtil.hxx"

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
}

Transporter::~Transporter()
{
}

}

/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */
