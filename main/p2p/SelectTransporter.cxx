#include "rutil/GenericIPAddress.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"

#include "p2p/SelectTransporter.hxx"
#include "p2p/Profile.hxx"
#include "p2p/TransporterMessage.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"
#include "p2p/Candidate.hxx"
#include "p2p/p2p.hxx"
#include "p2p/P2PSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

namespace p2p
{

SelectTransporter::SelectTransporter ( Profile &configuration )
  : Transporter(configuration), mHasBootstrapSocket(false)
{
   resip::Data localIp = resip::DnsUtil::getLocalIpAddress();
   resip::DnsUtil::inet_pton(localIp, mLocalAddress);
   mNextPort = 30000;
}

SelectTransporter::~SelectTransporter()
{
   // Tear down any of the open sockets we have hanging around.   
   std::map<NodeId, FlowId>::iterator i;
   for (i = mNodeFlowMap.begin(); i != mNodeFlowMap.end(); i++)
   {
      resip::closeSocket((i->second).getSocket());
   }

   // And kill the listeners
   ListenerMap::iterator j;
   for (j = mListenerMap.begin(); j != mListenerMap.end(); j++)
   {
      resip::closeSocket(j->second.first);
   }

}

//----------------------------------------------------------------------
// Impls follow

/**
  @note This method is to be called by bootstrap nodes only -- all other
        modes need to use ICE-allocated addresses.
*/
void
SelectTransporter::addListenerImpl(resip::TransportType transport,
                                   resip::GenericIPAddress &address)
{
   int status;
   assert(!mHasBootstrapSocket);
   assert(transport == resip::TCP);

   DebugLog(<< "Adding bootstrap listener");

   struct sockaddr_in addr = address.v4Address;

   mBootstrapSocket = ::socket(AF_INET, SOCK_STREAM, 0);
   if (!mBootstrapSocket) {ErrLog(<< "::socket() failed");}

   status = ::bind(mBootstrapSocket, reinterpret_cast<sockaddr *>(&addr),
                   sizeof(struct sockaddr_in));
   if (status) { ErrLog( << "Cannot ::bind"); }

   status = ::listen(mBootstrapSocket, 120);
   if (status) { ErrLog( << "Cannot ::listen"); }

   mHasBootstrapSocket = true;
}

void
SelectTransporter::sendImpl(NodeId nodeId, std::auto_ptr<p2p::Message> msg)
{
   if (msg->getTTL() <= 1)
   {
      InfoLog(<< "TTL Exhausted -- dropping message.");
      return;
   }

   msg->decrementTTL();

   std::map<NodeId, FlowId>::iterator i;
   i = mNodeFlowMap.find(nodeId);

   if (i == mNodeFlowMap.end())
   {
      // XXX FIX ME -- should send error to application
      ErrLog( << "Cannot send -- node not in flow map");
      return;
   }

   //**********************************************************************
   // XXX For datagram transports, we would need to fragment here.
   //**********************************************************************

   resip::Data data;
   data = msg->encodePayload();

   size_t bytesSent = ::send((i->second).getSocket(), data.data(), data.size(), 0);
   // XXX should check send response, and inform app of error if fail

   if (bytesSent != data.size()) 
   { 
      ErrLog( << "Cannot send -- ::send returned " << bytesSent); 
   }
}

void
SelectTransporter::sendImpl(FlowId flowId, std::auto_ptr<resip::Data> data)
{
   size_t bytesSent = ::send(flowId.getSocket(), data->data(), data->size(), 0);
   // XXX should check send response, and inform app of error if fail

   if (bytesSent != data->size()) 
   { 
      ErrLog(<< "Cannot send -- ::send returned " << bytesSent); 
   }
}

void
SelectTransporter::collectCandidatesImpl(NodeId nodeId, unsigned short appId)
{
  // For right now, we just return one candidate: a single TCP
  // listener. And we return it right away.
  DebugLog(<< "Collection candidates for application " << appId);

  ListenerMap::iterator i;
  resip::GenericIPAddress addrPort;
  int status;

  i = mListenerMap.find(std::make_pair(nodeId, appId));

  // If we don't already have a listener, we need to create
  // a new one and throw it in the listener map
  if (i == mListenerMap.end())
  {
     DebugLog(<< "Adding new listener for application " << appId);
     struct sockaddr_in addr;
     resip::Socket s;
#ifndef WIN32
     addr.sin_len = sizeof(struct sockaddr_in);
#endif
     addr.sin_family = AF_INET;
     addr.sin_port = mNextPort++;
     memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

     s = ::socket(AF_INET, SOCK_STREAM, 0);
     if (!s) {ErrLog(<< "::socket() failed");}
   
     status = ::bind(s, reinterpret_cast<sockaddr *>(&addr),
            sizeof(struct sockaddr_in));
     if (status) { ErrLog( << "Cannot ::bind"); }

     status = ::listen(s, 1);
     if (status) { ErrLog( << "Cannot ::listen"); }

     addrPort.v4Address = addr;

     mListenerMap.insert(ListenerMap::value_type(
        std::make_pair(nodeId, appId),
        std::make_pair(s,addrPort)));

     i = mListenerMap.find(std::make_pair(nodeId, appId));
     assert(i != mListenerMap.end());
   }
   else
   {
     DebugLog(<< "Found existing listener for application " << appId);
     addrPort = i->second.second;
   }

   std::vector<Candidate> candidates;
   Candidate c(resip::TCP, addrPort); // XXX -- SHOULD BE TLS
   candidates.push_back(c);

   LocalCandidatesCollected *lcc = new LocalCandidatesCollected(nodeId, appId, candidates);
   mRxFifo->add(lcc);
}

void
SelectTransporter::connectImpl(resip::GenericIPAddress &bootstrapServer)
{
   unsigned short application = RELOAD_APPLICATION_ID;
   resip::Socket s;

   s = ::socket(AF_INET, SOCK_STREAM, 0);
   if (!s) {ErrLog(<< "::socket() failed");}
   
   int status = ::connect(s, &(bootstrapServer.address), sizeof(sockaddr_in));
   if (status) { ErrLog( << "Cannot ::connect"); return; }

   // Get the remote node ID from the incoming socket
   unsigned char buffer[16];
   size_t bytesRead = ::read(s, buffer, sizeof(buffer));
   if (bytesRead != sizeof(buffer)) 
   {
      ErrLog( << "Cannot ::read -- returned " << bytesRead); 
   }
   s2c::NodeIdStruct nid;
   nid.mHigh = *((UInt64*)(buffer));
   nid.mLow = *((UInt64*)(buffer+sizeof(UInt64)));

   NodeId nodeId(nid);
   FlowId flowId(nodeId, application, s, *mRxFifo);

   mNodeFlowMap.insert(std::map<NodeId, FlowId>::value_type(nodeId, flowId));

   ConnectionOpened *co = new ConnectionOpened(flowId,
                                               application,
                                               resip::TCP,
                                               0 /* no cert for you */);
   mRxFifo->add(co);
   //assert(0);
}

void
SelectTransporter::connectImpl(NodeId nodeId,
                               std::vector<Candidate> remoteCandidates,
                               resip::GenericIPAddress &stunTurnServer)
{
   connectImpl(nodeId, remoteCandidates, RELOAD_APPLICATION_ID,
               *mRxFifo, stunTurnServer);

   std::map<NodeId, FlowId>::iterator i;
   i = mNodeFlowMap.find(nodeId);
   assert (i != mNodeFlowMap.end());

   // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
   // Blow our node ID out on the wire (because there is no cert)
   const resip::Data nid = mConfiguration.nodeId().encodeToNetwork();
   size_t bytesSent = ::send((i->second).getSocket(),
                             nid.data(), nid.size(), 0);

   if (bytesSent != nid.size()) 
   {
      ErrLog( << "Cannot ::send -- returned " << bytesSent); 
   }
   // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
}

void
SelectTransporter::connectImpl(NodeId nodeId,
                         std::vector<Candidate> remoteCandidates,
                         unsigned short application,
                         resip::Fifo<Event>& dataFifo,
                         resip::GenericIPAddress &stunTurnServer)
{
   // XXX Right now, we just grab the first candidate out of the array
   // and connect to it. Whee!

   resip::Socket s;
   Candidate candidate = remoteCandidates.front();

   // Right now, we're only implementing stream stuff.
   // Later on, we'll have to have different processing
   // based on the transports -- but this will likely be
   // hidden from us by the ICE library
   assert (candidate.getTransportType() == resip::TCP
           || candidate.getTransportType() == resip::TLS);

   s = ::socket(AF_INET, SOCK_STREAM, 0);
   if (!s) {ErrLog(<< "::socket() failed");}

   int status = ::connect(s, &(candidate.getAddress().address), sizeof(sockaddr_in));
   if (status) { ErrLog( << "Cannot ::connect"); }

   FlowId flowId(nodeId, application, s, dataFifo);

   mNodeFlowMap.insert(std::map<NodeId, FlowId>::value_type(nodeId, flowId));

   ConnectionOpened *co = new ConnectionOpened(flowId,
                                               application,
                                               candidate.getTransportType(),
                                               0 /* no cert for you */);
   flowId.getFifo().add(co);
}

//----------------------------------------------------------------------


// XXX FIXME -- Currently, we spin between waiting for the
// descriptor and waiting for the command queue. This should be
// fixed later to have a unified wait.
bool
SelectTransporter::process(int ms)
{
   resip::FdSet fdSet;
   TransporterCommand *cmd;

   // First, we do the socket descriptors...

   std::map<NodeId, FlowId>::iterator i;
   for (i = mNodeFlowMap.begin(); i != mNodeFlowMap.end(); i++)
   {
      fdSet.setRead((i->second).getSocket());
   }

   ListenerMap::iterator j;
   for (j = mListenerMap.begin(); j != mListenerMap.end(); j++)
   {
      fdSet.setRead(j->second.first);
   }

   if (mHasBootstrapSocket)
   {
      fdSet.setRead(mBootstrapSocket);
   }

   // TODO -- add bootstrap listener socket

   fdSet.selectMilliSeconds(ms);

   if (mHasBootstrapSocket && fdSet.readyToRead(mBootstrapSocket))
   {
      // New incoming BOOTSTRAP connection. Yaay!!!
      unsigned short application = RELOAD_APPLICATION_ID;

      resip::Socket s;
      struct sockaddr addr;
      socklen_t addrlen;

      s = accept(j->second.first, &addr, &addrlen);

     // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
     // Blow our node ID out on the wire (because there is no cert)
     const resip::Data nid = mConfiguration.nodeId().encodeToNetwork();
     size_t bytesSent = ::send((i->second).getSocket(),
                               nid.data(), nid.size(), 0);

     if (bytesSent != nid.size()) 
     {
        ErrLog( << "Cannot ::send -- returned " << bytesSent); 
     }
     // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 

      // Get the remote node ID from the incoming socket
      char buffer[16];
      size_t bytesRead = ::read(s, buffer, sizeof(buffer));
      if (bytesRead != sizeof(buffer)) 
      {
         ErrLog( << "Cannot ::read -- returned " << bytesRead); 
      }
      
      // Parse the node id
      s2c::NodeIdStruct nids;
      resip::Data data(resip::Data::Borrow, buffer, sizeof(buffer));
      resip::DataStream strm(data);
      nids.decode(strm);
      NodeId nodeId(nids);

      FlowId flowId(nodeId, application, s, *mRxFifo);

      mNodeFlowMap.insert(
        std::map<NodeId, FlowId>::value_type(nodeId, flowId));

      ConnectionOpened *co = new ConnectionOpened(flowId,
                                                application,
                                                resip::TCP,
                                                0 /* no cert for you */);
      mRxFifo->add(co);
      
   }

   // Check for new incoming connections
   for (j = mListenerMap.begin(); j != mListenerMap.end(); j++)
   {
      if (fdSet.readyToRead(j->second.first))
      {
        // New incoming connection. Yaay!
        NodeId nodeId = j->first.first;
        unsigned short application = j->first.second;

        resip::Socket s;
        struct sockaddr addr;
        socklen_t addrlen;

        s = accept(j->second.first, &addr, &addrlen);

        // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
        // Blow our node ID out on the wire (because there is no cert)
        const resip::Data nid = mConfiguration.nodeId().encodeToNetwork();
        size_t bytesSent = ::send((i->second).getSocket(),
                                  nid.data(), nid.size(), 0);

        if (bytesSent != nid.size()) 
        {
           ErrLog( << "Cannot ::send -- returned " << bytesSent); 
        }
        // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 

        FlowId flowId(nodeId, application, s, *mRxFifo);

        mNodeFlowMap.insert(
          std::map<NodeId, FlowId>::value_type(nodeId, flowId));

        unsigned char buffer[16];
        size_t bytesRead = ::read(s, buffer, sizeof(buffer));
        if (bytesRead != sizeof(buffer)) 
        {
           ErrLog( << "Cannot ::read -- returned " << bytesRead); 
        }
        
        // Ideally, we'd check that the nodeId we just read
        // actually matches the nodeId we were expecting.

        ConnectionOpened *co = new ConnectionOpened(flowId,
                                                  application,
                                                  resip::TCP,
                                                  0 /* no cert for you */);
        mRxFifo->add(co);

        // Open Issue -- should we close and remove the listener here?
        // Adam and ekr say "probably"; fluffy says "hell no".
        resip::closeSocket(j->second.first);
        mListenerMap.erase(j++);
      }
   }


   // Check for new incoming data   
   for (i = mNodeFlowMap.begin(); i != mNodeFlowMap.end(); i++)
   {
      FlowId &flowId = i->second;
      if (fdSet.readyToRead(flowId.getSocket()))
      {
         // There's data waiting to be read
         if (flowId.getApplication() == RELOAD_APPLICATION_ID)
         {
            //**************************************************
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // XXX XXX THIS IS NAIVE AND WRONG -- WE ASSUME ALL
            // THE BYTES FOR THIS MESSAGE ARE ALREADY IN THE
            // LOCAL TCP BUFFER. THIS MUST BE FIXED.
            //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //**************************************************
            char *buffer = new char[16384];
            UInt32 *int_buffer = reinterpret_cast<UInt32*>(buffer);
            // Suck in the header
            size_t bytesRead = ::read(flowId.getSocket(), buffer, 30);

            if (bytesRead != 30) 
            {
               ErrLog( << "Cannot ::read -- returned " << bytesRead); 
            }

            if (int_buffer[0] == htonl(0x80000000 | 0x52454C4F))
            {
               size_t length = ntohl(int_buffer[3]) & 0xFFFFFF;
               bytesRead = ::read(flowId.getSocket(), buffer+30, length-30);

               if (bytesRead != (length - 30)) 
               {
                  ErrLog( << "Cannot ::read -- returned " << bytesRead); 
               }

               resip::Data data(resip::Data::Take, buffer, length);
               std::auto_ptr<p2p::Message> msg(Message::parse(data));
               msg->pushVia(i->first);

               MessageArrived *ma = new MessageArrived(flowId.getNodeId(), msg);

               mRxFifo->add(ma);
            }
            else
            {
               delete(buffer);
               // Yikes! This isn't a reload message!
            }
            
         }
         else
         {
            // Application data is sent as-is.
            char *buffer = new char[4096];
            size_t bytesRead;
            bytesRead = ::read(flowId.getSocket(), buffer, 4096);
            if (bytesRead > 0)
            {
               resip::Data data(resip::Data::Take, buffer, bytesRead);

               ApplicationMessageArrived *ama = new
                  ApplicationMessageArrived(flowId, data);

               flowId.getFifo().add(ama);
            }
            else
            {
              delete [] buffer;
            }
         }
      }
   }

   // ...then, we do the command FIFO
   if ((cmd = mCmdFifo.getNext(ms)) != 0)
   {
      (*cmd)();
      delete cmd;
      return true;
   }
   return false;
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
