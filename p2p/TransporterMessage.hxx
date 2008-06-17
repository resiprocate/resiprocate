#ifndef __P2P_TRANSPORTER_MESSAGE_HXX
#define __P2P_TRANSPORTER_MESSAGE_HXX 1

#include "openssl/ssl.h"

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/TransportType.hxx"

#include "p2p/NodeId.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Candidate.hxx"
#include "p2p/Message.hxx"

namespace p2p
{

class ConnectionOpened;
class ConnectionClosed;
class ApplicationMessageArrived;
class MessageArrived;
class LocalCandidatesCollected;

class Message;
class FlowId;
class Candidate;

class TransporterMessage
{
   public:
      virtual ~TransporterMessage() {;}

      virtual ConnectionOpened* castConnectionOpened();
      virtual ConnectionClosed* castConnectionClosed();
      virtual ApplicationMessageArrived* castApplicationMessageArrived();
      virtual MessageArrived* castMessageArrived();
      virtual LocalCandidatesCollected* castLocalCandidatesCollected();

   protected:

      typedef enum
      {
         ConnectionOpenedType,
         ConnectionClosedType,
         ApplicationMessageArrivedType,
         MessageArrivedType,
         LocalCandidatesCollectedType
      } MessageType;

      virtual MessageType getMessageType() = 0;
};

class ConnectionOpened : public TransporterMessage
{
   public:
      ConnectionOpened(
                       FlowId flowId,
                       unsigned short application,
                       resip::TransportType transportType,
                       X509 *cert
                       ) {}

      FlowId getFlowId();
      unsigned short getApplication();
      NodeId getNodeId() { return getFlowId().getNodeId(); }
      resip::TransportType getTransportType();
      X509 *getCertificate();

   protected:
      virtual MessageType getMessageType() {return ConnectionOpenedType;}
};

class ConnectionClosed : public TransporterMessage
{
   public:
      NodeId getNodeId();
      unsigned short getApplicationId();

   protected:
      virtual MessageType getMessageType() {return ConnectionClosedType;}
};

class MessageArrived : public TransporterMessage
{
   public:
      MessageArrived (NodeId nodeId, std::auto_ptr<p2p::Message> message)
        : mNodeId(nodeId), mMessage(message) {;}

      NodeId getNodeId() {return mNodeId;}
      std::auto_ptr<p2p::Message> getMessage() { return mMessage; }

   protected:
      virtual MessageType getMessageType() {return MessageArrivedType;}

      NodeId mNodeId;
      std::auto_ptr<p2p::Message> mMessage;
};

class ApplicationMessageArrived : public TransporterMessage
{
   public:
      ApplicationMessageArrived(FlowId flowId, resip::Data &data)
        : mFlowId(flowId), mData(data) {;}

      FlowId getFlowId() const { return mFlowId; }
      const resip::Data &getData() const { return mData; }

   protected:
      virtual MessageType getMessageType() {return ApplicationMessageArrivedType;}

   private:
     FlowId mFlowId;
     resip::Data mData;
};

class LocalCandidatesCollected : public TransporterMessage
{
   public:
      LocalCandidatesCollected(std::vector<Candidate> &c) : 
         mCandidates(c) {;}

         std::vector<Candidate> &getCandidates() { return mCandidates; }

   protected:
      virtual MessageType getMessageType() {return LocalCandidatesCollectedType;}

   private:
      std::vector<Candidate> mCandidates;
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
