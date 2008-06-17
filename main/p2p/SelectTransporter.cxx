#include "rutil/GenericIPAddress.hxx"
#include "rutil/DnsUtil.hxx"

#include "p2p/SelectTransporter.hxx"
#include "p2p/ConfigObject.hxx"
#include "p2p/TransporterMessage.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"
#include "p2p/Candidate.hxx"

namespace p2p
{

SelectTransporter::SelectTransporter (resip::Fifo<TransporterMessage>& rxFifo,
                          ConfigObject &configuration)
  : Transporter(rxFifo, configuration)
{
   // We really shouldn't do this in the constructor -- but it goes
   // away when we add ICE.
   resip::Data localIp = resip::DnsUtil::getLocalIpAddress();
   resip::DnsUtil::inet_pton(localIp, mLocalAddress.sin_addr);
#ifndef WIN32
   mLocalAddress.sin_len = sizeof(struct sockaddr_in);
#endif
   mLocalAddress.sin_family = AF_INET;
   mLocalAddress.sin_port = 39835;
   memset(mLocalAddress.sin_zero, 0, sizeof(mLocalAddress.sin_zero));

   mTcpDescriptor = ::socket(AF_INET, SOCK_STREAM, 0);
   ::bind(mTcpDescriptor, reinterpret_cast<sockaddr *>(&mLocalAddress),
          sizeof(struct sockaddr_in));
   ::listen(mTcpDescriptor, 20);
}

SelectTransporter::~SelectTransporter()
{
   resip::closeSocket(mTcpDescriptor);

   // Tear down any of the sockets we have hanging around.   
   std::map<NodeId, FlowId>::iterator i;
   for (i = mNodeFlowMap.begin(); i != mNodeFlowMap.end(); i++)
   {
      resip::closeSocket((i->second).getSocket());
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
  // XXX
  assert(0);
}

void
SelectTransporter::sendImpl(NodeId nodeId, std::auto_ptr<p2p::Message> msg)
{
   std::map<NodeId, FlowId>::iterator i;
   i = mNodeFlowMap.find(nodeId);

   if (i == mNodeFlowMap.end())
   {
      // XXX FIX ME -- should send error to application
      return;
   }

   //**********************************************************************
   // XXX For datagram transports, we would need to fragment here.
   //**********************************************************************

   resip::Data data;
   data = msg->encode();

   ::send((i->second).getSocket(), data.c_str(), data.size(), 0);
   // XXX should check send response, and inform app of error if fail
}

void
SelectTransporter::sendImpl(FlowId flowId, std::auto_ptr<resip::Data> data)
{
   ::send(flowId.getSocket(), data->c_str(), data->size(), 0);
   // XXX should check send response, and inform app of error if fail
}

void
SelectTransporter::collectCandidatesImpl()
{
  // For right now, we just return one candidate: a single TCP
  // listener. And we return it right away.

  std::vector<Candidate> candidates;
  resip::GenericIPAddress addr(mLocalAddress);
  Candidate c(resip::TCP, addr); // XXX -- SHOULD BE TLS
  candidates.push_back(c);

  LocalCandidatesCollected *lcc = new LocalCandidatesCollected(candidates);
  mRxFifo.add(lcc);
}

void
SelectTransporter::connectImpl(NodeId nodeId,
                               std::vector<Candidate> remoteCandidates,
                               resip::GenericIPAddress &stunTurnServer)
{
   connectImpl(nodeId, remoteCandidates, RELOAD_APPLICATION_ID,
                 stunTurnServer);

   std::map<NodeId, FlowId>::iterator i;
   i = mNodeFlowMap.find(nodeId);
   assert (i != mNodeFlowMap.end());

   // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
   // Blow our node ID out on the wire (because there is no cert)
   ::send((i->second).getSocket(), (const char *)(mConfiguration.nodeId()), 
          sizeof(NodeId), 0);
   // ********** XXX REMOVE THIS WHEN WE GO TO TLS/DTLS XXX ********** 
}

void
SelectTransporter::connectImpl(NodeId nodeId,
                         std::vector<Candidate> remoteCandidates,
                         unsigned short application,
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
   ::connect(s, &(candidate.getAddress().address), sizeof(sockaddr_in));

   FlowId flowId(nodeId, application, s);

   mNodeFlowMap.insert(std::map<NodeId, FlowId>::value_type(nodeId, flowId));

   ConnectionOpened *co = new ConnectionOpened(flowId,
                                               application,
                                               candidate.getTransportType(),
                                               0 /* no cert for you */);
   mRxFifo.add(co);
}

//----------------------------------------------------------------------

// This isn't anything like finished yet -- need to wait for data on our
// descriptors also.
bool
SelectTransporter::process(int ms)
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
