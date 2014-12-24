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
   resip_assert(!mHasBootstrapSocket);
   resip_assert(transport == resip::TCP);

   DebugLog(<< "Adding bootstrap listener");

   struct sockaddr_in addr = address.v4Address;

   mBootstrapSocket = ::socket(AF_INET, SOCK_STREAM, 0);
   if (!mBootstrapSocket) {ErrLog(<< "::socket() failed, errno " << resip::getErrno());}

   int on = 1;
#if !defined(WIN32)
   ::setsockopt ( mBootstrapSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#else
   ::setsockopt ( mBootstrapSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
#endif

   status = ::bind(mBootstrapSocket, reinterpret_cast<sockaddr *>(&addr),
                   sizeof(struct sockaddr_in));
   if (status) { ErrLog( << "Cannot ::bind, errno " << resip::getErrno()); }

   status = ::listen(mBootstrapSocket, 120);
   if (status) { ErrLog( << "Cannot ::listen, errno " << resip::getErrno()); }

   mHasBootstrapSocket = true;
}

void
SelectTransporter::sendImpl(NodeId nodeId, std::auto_ptr<p2p::Message> msg)
{
   DebugLog( << "sending " << msg->brief() << " to " << nodeId);
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
      ErrLog( << "Cannot send -- ::send returned " << bytesSent << " errno " << resip::getErrno()); 
      resip_assert(0);
   }
   else
   {
     DebugLog(<< "sent " << msg->brief() << " over " << i->second << "bytes=" << bytesSent );
   } 
}

void
SelectTransporter::sendImpl(FlowId flowId, std::auto_ptr<resip::Data> data)
{
   DebugLog( << "sending raw data to: " << flowId);

   size_t bytesSent = ::send(flowId.getSocket(), data->data(), data->size(), 0);
   // XXX should check send response, and inform app of error if fail

   if (bytesSent != data->size()) 
   { 
      ErrLog(<< "Cannot send -- ::send returned " << bytesSent << " errno " << resip::getErrno()); 
      resip_assert(0);
   }
}

void
SelectTransporter::collectCandidatesImpl(UInt64 tid, NodeId nodeId, unsigned short appId)
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
     DebugLog(<< "Adding new listener for application " << appId << " on port " << mNextPort);
     struct sockaddr_in addr;
     resip::Socket s;
     int yes=1;
     
     int retry=5;
     
     while(retry--)
     {
#ifdef HAVE_sockaddr_in_len
        addr.sin_len = sizeof(struct sockaddr_in);
#endif
        addr.sin_family = AF_INET;
        addr.sin_port = htons(mNextPort++);
        memset(&addr.sin_addr, 0, sizeof(addr.sin_addr));
        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
     

        s = ::socket(AF_INET, SOCK_STREAM, 0);

#if !defined(WIN32)
        ::setsockopt ( s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#else
        ::setsockopt ( s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#endif

        if (!s) {ErrLog(<< "::socket() failed, errno " << resip::getErrno());}

        DebugLog(<< "Binding to port " << ntohs(addr.sin_port));

        status = ::bind(s, reinterpret_cast<sockaddr *>(&addr),
                        sizeof(struct sockaddr_in));
        if (status) { 
           ErrLog( << "Cannot ::bind, errno " << resip::getErrno());
           continue;
        }
     }
     resip_assert(status==0);
     
     status = ::listen(s, 5);
     if (status) { ErrLog( << "Cannot ::listen, errno " << resip::getErrno()); }

     addrPort.v4Address = addr;

     mListenerMap.insert(ListenerMap::value_type(
        std::make_pair(nodeId, appId),
        std::make_pair(s,addrPort)));

     i = mListenerMap.find(std::make_pair(nodeId, appId));
     resip_assert(i != mListenerMap.end());
   }
   else
   {
     DebugLog(<< "Found existing listener for application " << appId);
     addrPort = i->second.second;
   }

   std::vector<Candidate> candidates;
   Candidate c(resip::TCP, addrPort); // XXX -- SHOULD BE TLS
   candidates.push_back(c);

   LocalCandidatesCollected *lcc = new LocalCandidatesCollected(tid, nodeId, appId, candidates);
   mRxFifo->add(lcc);
}

void
SelectTransporter::connectImpl(resip::GenericIPAddress &bootstrapServer)
{
   unsigned short application = RELOAD_APPLICATION_ID;
   resip::Socket s;

   DebugLog(<<"Trying to connect to bootstrap server");

   s = ::socket(AF_INET, SOCK_STREAM, 0);
   if (!s) 
   {
      ErrLog(<< "::socket() failed");
      resip_assert(0);
   }
   
   int status = ::connect(s, &(bootstrapServer.address), sizeof(sockaddr_in));
   if (status) { ErrLog( << "Cannot ::connect, errno " << resip::getErrno()); resip_assert(0); return; }

   DebugLog(<<"Connect succeeded");

   // Get the remote node ID from the incoming socket
   unsigned char buffer[16];
   
   size_t bytesRead = readSocket(s, (char*)buffer, sizeof(buffer));
   DebugLog(<< "Read#1" << bytesRead);

   if (bytesRead != sizeof(buffer)) 
   {
      ErrLog( << "Cannot ::read -- returned " << bytesRead << " errno " << resip::getErrno()); 
      resip_assert(0);
   }
   s2c::NodeIdStruct nid;
   nid.mHigh = *((UInt64*)(buffer));
   nid.mLow = *((UInt64*)(buffer+sizeof(UInt64)));

   NodeId nodeId(nid);
   FlowId flowId(nodeId, application, s, *mRxFifo);

   mNodeFlowMap.insert(std::map<NodeId, FlowId>::value_type(nodeId, flowId));

   // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
   // Blow our node ID out on the wire (because there is no cert)
   
   resip::Data ourNid=mConfiguration.nodeId().encodeToNetwork();
   size_t bytesSent = ::send(s,ourNid.data(), ourNid.size(), 0);
   if (bytesSent != ourNid.size()) 
   {
      ErrLog( << "Cannot ::send -- returned " << bytesSent << " errno " << resip::getErrno()); 
   }
   

   DebugLog(<< "Connected to server: " << flowId << " sending ConnectionOpened to forwarding layer");
   ConnectionOpened *co = new ConnectionOpened(flowId,
                                               application,
                                               resip::TCP,
                                               false /* inbound? */,
                                               0 /* no cert for you */);
   mRxFifo->add(co);
   //assert(0);
}

void
SelectTransporter::connectImpl(NodeId nodeId,
                               std::vector<Candidate> remoteCandidates,
                               resip::GenericIPAddress &stunTurnServer)
{
   DebugLog(<< "connectImpl invoked");

   connectImpl(nodeId, remoteCandidates, RELOAD_APPLICATION_ID,
               *mRxFifo, stunTurnServer);
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

   DebugLog(<< "connectImpl invoked");

   resip::Socket s;
   Candidate candidate = remoteCandidates.front();

   // Right now, we're only implementing stream stuff.
   // Later on, we'll have to have different processing
   // based on the transports -- but this will likely be
   // hidden from us by the ICE library
   resip_assert (candidate.getTransportType() == resip::TCP
           || candidate.getTransportType() == resip::TLS);

   s = ::socket(AF_INET, SOCK_STREAM, 0);
   if (!s) {ErrLog(<< "::socket() failed, errno " << resip::getErrno());}

   int status = ::connect(s, &(candidate.getAddress().address), sizeof(sockaddr_in));
   if (status) { ErrLog( << "Cannot ::connect, errno " << resip::getErrno()); }


   // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
   // Blow our node ID out on the wire (because there is no cert)
   const resip::Data nid = mConfiguration.nodeId().encodeToNetwork();
   size_t bytesSent = ::send(s, nid.data(), nid.size(), 0);
   if (bytesSent != nid.size()) 
   {
      ErrLog( << "Cannot ::send -- returned " << bytesSent << " errno " << resip::getErrno());
   }
   // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 

   // Get the remote node ID from the incoming socket
   char buffer[16];
   size_t bytesRead = readSocket(s, buffer, sizeof(buffer));
   DebugLog(<< "Read#2 " << bytesRead);
   if (bytesRead != sizeof(buffer)) 
   {
      ErrLog( << "Cannot ::read -- returned " << bytesRead << " errno " << resip::getErrno()); 
      resip_assert(0);
   }
      
   // Parse the node id
   s2c::NodeIdStruct nids;
   resip::Data data(resip::Data::Borrow, buffer, sizeof(buffer));
   resip::DataStream strm(data);
   nids.decode(strm);
   NodeId nodeId2(nids);

   FlowId flowId(nodeId2, application, s, *mRxFifo);

   DebugLog(<< "SelectTransporter: connection to node " << nodeId2);

   mNodeFlowMap.insert(std::map<NodeId, FlowId>::value_type(nodeId, flowId));

   

   ConnectionOpened *co = new ConnectionOpened(flowId,
                                               application,
                                               candidate.getTransportType(),
                                               false /* inbound? */,
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
      socklen_t addrlen = sizeof(sockaddr);

      if((s = accept(mBootstrapSocket, &addr, &addrlen))==(-1)){
        ErrLog( << "Could not accept, errno " << resip::getErrno());
        resip_assert(false);
      }
      DebugLog(<<"Accepted a connection");

     // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
     // Blow our node ID out on the wire (because there is no cert)
     const resip::Data nid = mConfiguration.nodeId().encodeToNetwork();
     size_t bytesSent = ::send(s, nid.data(), nid.size(), 0);
     if (bytesSent != nid.size()) 
     {
        ErrLog( << "Cannot ::send -- returned " << bytesSent << " errno " << resip::getErrno());
     }
     // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 

      // Get the remote node ID from the incoming socket
      char buffer[16];
      size_t bytesRead = readSocket(s, buffer, sizeof(buffer));
      DebugLog(<< "Read#2 " << bytesRead);
      if (bytesRead != sizeof(buffer)) 
      {
         ErrLog( << "Cannot ::read -- returned " << bytesRead << " errno " << resip::getErrno()); 
         resip_assert(0);
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
                                                true /* inbound? */,
                                                0 /* no cert for you */);
      mRxFifo->add(co);
   }

   // Check for new incoming connections
   for (j = mListenerMap.begin(); j != mListenerMap.end(); j++)
   {
      DebugLog(<< "XXXXYYY");
      

      if (fdSet.readyToRead(j->second.first))
      {
        // New incoming connection. Yaay!
        NodeId nodeId = j->first.first;
        unsigned short application = j->first.second;

        resip::Socket s;
        struct sockaddr addr;
        socklen_t addrlen = sizeof(sockaddr);

        s = accept(j->second.first, &addr, &addrlen);

        // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
        // Blow our node ID out on the wire (because there is no cert)
        const resip::Data nid = mConfiguration.nodeId().encodeToNetwork();
        size_t bytesSent = ::send(s,nid.data(), nid.size(), 0);

        if (bytesSent != nid.size()) 
        {
           ErrLog( << "Cannot ::send -- returned " << bytesSent << " errno " << resip::getErrno()); 
        }
        // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 

        unsigned char buffer[16];
        size_t bytesRead = readSocket(s, (char*)buffer, sizeof(buffer));
        DebugLog(<< "Read#3" << bytesRead);
        if (bytesRead != sizeof(buffer)) 
        {
           ErrLog( << "Cannot ::read -- returned " << bytesRead << " errno " << resip::getErrno()); 
           resip_assert(0);
        }

        s2c::NodeIdStruct nids;

        nids.mHigh = *((UInt64*)(buffer));
        nids.mLow = *((UInt64*)(buffer+sizeof(UInt64)));
        
        NodeId nodeId2(nids);
        FlowId flowId(nodeId2, application, s, *mRxFifo);

        mNodeFlowMap.insert(
           std::map<NodeId, FlowId>::value_type(nodeId2, flowId));

        DebugLog(<< "SelectTransporter: connection to node " << nodeId);
        
        // Ideally, we'd check that the nodeId we just read
        // actually matches the nodeId we were expecting.

        ConnectionOpened *co = new ConnectionOpened(flowId,
                                                  application,
                                                  resip::TCP,
                                                  true /* inbound? */,
                                                  0 /* no cert for you
                                                     * */);
        
        DebugLog(<< "Notifying about connection opened");

        mRxFifo->add(co);
        
        // Open Issue -- should we close and remove the listener here?
        // Adam and ekr say "probably"; fluffy says "hell no".
        resip::closeSocket(j->second.first);
        // TODO: remove this from the map somehow?
        //ListenerMap::iterator jold=j++;        
        //mListenerMap.erase(jold);
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
            char *ptr;
            size_t bytesRead;
            size_t left=30;
            ptr=buffer;

            while(left){
              bytesRead = readSocket(flowId.getSocket(), ptr, left);
              if(bytesRead == -1) 
              {
                  ErrLog( << "Cannot ::read -- returned " << bytesRead << " errno " << resip::getErrno()); 
                  resip_assert(false);
              }
              left-=bytesRead;
              ptr+=bytesRead;
              DebugLog(<< "Read from socket: " << bytesRead << " left=" << left);
              resip_assert(bytesRead!=0); // Closed...
            }

            if (int_buffer[0] == htonl(0x80000000 | 0x52454C4F))
            {
               size_t length = ntohl(int_buffer[3]) & 0xFFFFFF;
               bytesRead = readSocket(flowId.getSocket(), buffer+30, length-30);

               if (bytesRead != (length - 30)) 
               {
                  ErrLog( << "Cannot ::read -- returned " << bytesRead << " errno " << resip::getErrno()); 
               }

               resip::Data data(resip::Data::Take, buffer, length);
               std::auto_ptr<p2p::Message> msg(Message::parse(data));
               msg->pushVia(i->first);

               MessageArrived *ma = new MessageArrived(flowId.getNodeId(), msg);

               mRxFifo->add(ma);
            }
            else
            {
              ErrLog(<< "Not a correct reload message");
              resip_assert(0);
               delete(buffer);
               // Yikes! This isn't a reload message!
            }
            
         }
         else
         {
            // Application data is sent as-is.
            char *buffer = new char[4096];
            size_t bytesRead;
            bytesRead = readSocket(flowId.getSocket(), buffer, 4096);
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

size_t 
SelectTransporter::readSocket(resip::Socket s, char* buffer, unsigned int size)
{
#if defined(WIN32)
   return ::recv(s, buffer, size, 0);
#else
   return ::read(s, buffer, size);
#endif
}

int
SelectTransporter::writeSocket(resip::Socket s, char* buffer, unsigned int size)
{
#if defined(WIN32)
   return ::send(s, buffer, size, 0);
#else
   return ::write(s, buffer, size);
#endif
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
