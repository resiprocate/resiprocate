#ifndef __P2P_TOPOLOGYAPI_HXX
#define __P2P_TOPOLOGYAPI_HXX 1

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"

#include "p2p/NodeId.hxx"
#include "p2p/ResourceId.hxx"
#include "p2p/Profile.hxx"
#include "p2p/Message.hxx"
#include "p2p/EventConsumer.hxx"
#include "p2p/Join.hxx"
#include "p2p/Update.hxx"
#include "p2p/Leave.hxx"
#include "p2p/Message.hxx"
//#include "p2p/Dispatcher.hxx"
#include "p2p/Transporter.hxx"

namespace p2p
{

class Dispatcher;
class TransactionLayer;

/// This is an abstract base class from which to derive the actual topology plugins
class TopologyAPI :  public EventConsumer
{
   public:
      virtual ~TopologyAPI() = 0;
      TopologyAPI(Profile& config, Dispatcher& dispatcher, Transporter& transporter);

      // called to join the overlay 
      virtual void joinOverlay()=0;
      
      // need a fifo to receive timer events 

      // Messages that the forwarding layer sends to this object
      virtual void newConnectionFormed( const NodeId& node, bool inbound )=0;
      virtual void connectionLost( const NodeId& node )=0;
      virtual void candidatesCollected( UInt64 tid, const NodeId& node, unsigned short appId, std::vector<Candidate>& candidates )=0;
       
      // deal with topology change messages 
      virtual void consume(ConnectReq& event)=0;
      virtual void consume(JoinReq& event)=0;
      virtual void consume(UpdateReq& event)=0;
      virtual void consume(LeaveReq& event)=0;

      // deal with responses
      virtual void consume(ConnectAns& msg)=0;
      virtual void consume(JoinAns& msg)=0;
      virtual void consume(UpdateAns& msg)=0;
      virtual void consume(LeaveAns& msg)=0;

      // Deal with routing querries 
      virtual const NodeId& findNextHop( const DestinationId& did) = 0;
      virtual const NodeId& findNextHop( const NodeId& node )=0;
      virtual const NodeId& findNextHop( const ResourceId& resource )=0;
      
      // Deal with replication for storage 
      virtual std::vector<NodeId> getReplicationSet( const ResourceId& resource )=0;
      ///    Returns list of nodes to replicate on

      // Functions to find out if this peer is responsible for something
      virtual bool isResponsible( const DestinationId& did) const=0;
      virtual bool isResponsible( const NodeId& node ) const=0;
      virtual bool isResponsible( const ResourceId& resource ) const=0;

      // Function to determine if we are connected to a node
      virtual bool isConnected( const NodeId& node ) const=0;

      // Function to hash resource names into resourceID 
      virtual ResourceId resourceId( const resip::Data& resourceName )=0;

      virtual resip::Data brief() const
      {
         return "TopologyAPI";
      }
      

   protected:
      Profile&     mProfile;
      Dispatcher&  mDispatcher;
      Transporter& mTransporter;
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
