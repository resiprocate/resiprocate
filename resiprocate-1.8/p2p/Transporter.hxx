#ifndef __P2P_TRANSPORTER_HXX
#define __P2P_TRANSPORTER_HXX 1

#include <memory>
#include <vector>
#include <map>

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/Socket.hxx"

#include "p2p/Profile.hxx"
#include "p2p/NodeId.hxx"

namespace p2p
{

class Candidate;
class Event;
class FlowId;
class Message;
class Profile;
class TransporterCommand;
class Event;

class Transporter
{
   public:
      friend class AddListenerCommand;
      friend class SendP2pCommand;
      friend class SendApplicationCommand;
      friend class CollectCandidatesCommand;
      friend class ConnectBootstrapCommand;
      friend class ConnectP2pCommand;
      friend class ConnectApplicationCommand;

      Transporter(Profile &configuration);

      virtual ~Transporter();

      void setRxFifo(resip::Fifo<Event> *fifo) {mRxFifo = fifo;}

      virtual bool process(int ms=0) = 0;
   
      void addListener(resip::TransportType transport,
                       resip::GenericIPAddress &address);
   
      void send(NodeId nodeId, std::auto_ptr<p2p::Message> msg);
      void send(FlowId flowId, std::auto_ptr<resip::Data> data);
   
      void collectCandidates(UInt64 tid, NodeId nodeId,
                             unsigned short appId = RELOAD_APPLICATION_ID);
   
      void connect(resip::GenericIPAddress &);

      void connect(NodeId nodeId, 
                   std::vector<Candidate> remoteCandidates,
                   resip::GenericIPAddress &stunTurnServer);
   
      void connect(NodeId nodeId, 
                   std::vector<Candidate> remoteCandidates,
                   unsigned short application,
                   resip::Fifo<Event>&,
                   resip::GenericIPAddress &stunTurnServer);

   protected:
      virtual void addListenerImpl(resip::TransportType transport,
                           resip::GenericIPAddress &address) = 0;
   
      virtual void sendImpl(NodeId nodeId, std::auto_ptr<p2p::Message> msg) = 0;
      virtual void sendImpl(FlowId flowId, std::auto_ptr<resip::Data> data) = 0;
   
      virtual void collectCandidatesImpl(UInt64 tid, NodeId, unsigned short appId) = 0;

      virtual void connectImpl(resip::GenericIPAddress &) = 0;
   
      virtual void connectImpl(NodeId nodeId, 
                       std::vector<Candidate> remoteCandidates,
                       resip::GenericIPAddress &stunTurnServer) = 0;
   
      virtual void connectImpl(NodeId nodeId, 
                       std::vector<Candidate> remoteCandidates,
                       unsigned short application,
                       resip::Fifo<Event>&,
                       resip::GenericIPAddress &stunTurnServer) = 0;

     resip::Fifo<TransporterCommand> mCmdFifo;
     resip::Fifo<Event>* mRxFifo;

     Profile &mConfiguration;

   private:
};

/// This class is used internally by Transporter. Don't get any crazy ideas.
class TransporterCommand
{
   public:
     TransporterCommand(Transporter *transporter) : mTransporter(transporter) {;}

     virtual ~TransporterCommand() {;}

     virtual void operator()() = 0;

   protected:
      Transporter *mTransporter;
};


}

#endif

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
