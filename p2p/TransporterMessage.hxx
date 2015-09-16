#ifndef __P2P_TRANSPORTER_MESSAGE_HXX
#define __P2P_TRANSPORTER_MESSAGE_HXX 1

#include "openssl/ssl.h"

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/TransportType.hxx"

#include "p2p/Event.hxx"
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

class ConnectionOpened : public Event
{
   public:
      ConnectionOpened(FlowId flowId,
                       unsigned short application,
                       resip::TransportType transportType,
                       bool inbound,
                       X509 *cert);
      ~ConnectionOpened();
      
      virtual void dispatch(EventConsumer& consumer);
         
      FlowId getFlowId() const;
      unsigned short getApplication() const;
      NodeId getNodeId() const;
      resip::TransportType getTransportType() const;
      bool isInbound() const;
      X509 *getCertificate() const;

      virtual resip::Data brief() const
      {
         return "ConnectionOpened";
      }
      

   private:
      FlowId mFlowId;
      unsigned short mApplication;
      resip::TransportType mTransportType;
      bool mInbound;
      X509* mCert;
};

class ConnectionClosed : public Event
{
   public:
      ConnectionClosed(FlowId flowId, 
                       unsigned short application);
      ~ConnectionClosed();

      NodeId getNodeId() const;
      unsigned short getApplicationId() const;
      virtual void dispatch(EventConsumer& consumer);

      
      virtual resip::Data brief() const 
      {
         return "ConnectionClosed";
      }

   protected:
      FlowId mFlowId;
      unsigned short mApplication;
      
};

class MessageArrived : public Event
{
   public:
      MessageArrived (NodeId nodeId, std::auto_ptr<p2p::Message> message)
        : mNodeId(nodeId), mMessage(message) {}
      ~MessageArrived();

      virtual void dispatch(EventConsumer& consumer);

      NodeId getNodeId() const {return mNodeId;}        
      std::auto_ptr<p2p::Message> getMessage() { resip_assert(mMessage.get());
         return mMessage; }

      virtual resip::Data brief() const
      {
         return "MessageArrived";
      }
      

   protected:

      NodeId mNodeId;
      std::auto_ptr<p2p::Message> mMessage;
};

class ApplicationMessageArrived : public Event
{
   public:
      ApplicationMessageArrived(FlowId flowId, resip::Data &data)
        : mFlowId(flowId), mData(data) {}
      ~ApplicationMessageArrived();

      virtual void dispatch(EventConsumer& consumer);

      FlowId getFlowId() const { return mFlowId; }
      const resip::Data &getData() const { return mData; }

      virtual resip::Data brief() const
      {
         return "ApplicationMessageArrived";
      }
      

   protected:

   private:
     FlowId mFlowId;
     resip::Data mData;
};

class LocalCandidatesCollected : public Event
{
   public:
      LocalCandidatesCollected(UInt64 tid, NodeId& nodeId, unsigned short appId, std::vector<Candidate> &c) : 
         mTransactionId(tid), mNodeId(nodeId), mAppId(appId), mCandidates(c) {}
      ~LocalCandidatesCollected();

      virtual void dispatch(EventConsumer& consumer);

      const UInt64  getTransactionId() const { return mTransactionId; }
      const NodeId& getNodeId() const { return mNodeId; }
      unsigned short getAppId() const { return mAppId; }
      std::vector<Candidate>& getCandidates() { return mCandidates; }

      virtual resip::Data brief() const
      {
         return "LocalCandidatesCollected";
      }
      

   protected:

   private:
      UInt64 mTransactionId;
      NodeId mNodeId;
      unsigned short mAppId;
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
