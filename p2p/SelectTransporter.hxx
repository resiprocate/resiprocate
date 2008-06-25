#ifndef __P2P_SELECT_TRANSPORTER_HXX
#define __P2P_SELECT_TRANSPORTER_HXX 1

#include <vector>
#include <map>

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/Socket.hxx"

#include "p2p/NodeId.hxx"
#include "p2p/Transporter.hxx"

namespace p2p
{

class Event;
class Candidate;
class Message;
class Profile;
class FlowId;
class TransporterCommand;

class SelectTransporter : public Transporter
{
   public:
      friend class AddListenerCommand;
      friend class SendP2pCommand;
      friend class SendApplicationCommand;
      friend class CollectCandidatesCommand;
      friend class ConnectP2pCommand;
      friend class ConnectApplicationCommand;

      SelectTransporter( Profile &configuration );

      ~SelectTransporter();

      // ms is the value passed to select. 
      bool process(int ms=0);

   protected:
      void addListenerImpl(resip::TransportType transport,
                           resip::GenericIPAddress &address);
   
      void sendImpl(NodeId nodeId, std::auto_ptr<p2p::Message> msg);
      void sendImpl(FlowId flowId, std::auto_ptr<resip::Data> data);
   
  void collectCandidatesImpl(UInt64 tid, NodeId nodeId, unsigned short appId);

      virtual void connectImpl(resip::GenericIPAddress &);
   
      void connectImpl(NodeId nodeId, 
                       std::vector<Candidate> remoteCandidates,
                       resip::GenericIPAddress &stunTurnServer);
   
      void connectImpl(NodeId nodeId, 
                       std::vector<Candidate> remoteCandidates,
                       unsigned short application,
                       resip::Fifo<Event>&,
                       resip::GenericIPAddress &stunTurnServer);

   private:
     std::map<NodeId, FlowId> mNodeFlowMap;

     typedef std::map<std::pair<NodeId,unsigned short>,
                      std::pair<resip::Socket, resip::GenericIPAddress> > 
             ListenerMap;

     ListenerMap mListenerMap;

     // This will also change -- or really, go away -- when we add ice
     in_addr mLocalAddress;
     unsigned short mNextPort;

     bool mHasBootstrapSocket;
     resip::GenericIPAddress mBootstrapAddress;
     resip::Socket mBootstrapSocket;

     // OS independant implementations
     size_t readSocket(resip::Socket s, char* buffer, unsigned int size);
     int writeSocket(resip::Socket s, char* buffer, unsigned int size);
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
