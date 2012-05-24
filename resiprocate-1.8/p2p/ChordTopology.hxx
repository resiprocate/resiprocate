#ifndef __P2P_CHORDTOPOLOGY_HXX
#define __P2P_CHORDTOPOLOGY_HXX 1

#include <set>
#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"

#include "p2p/NodeId.hxx"
#include "p2p/ResourceId.hxx"
#include "p2p/Profile.hxx"
#include "p2p/Message.hxx"
#include "p2p/TopologyAPI.hxx"

namespace p2p
{

class Dispatcher;

/// This is an abstract base class from which to derive the actually topology plugins
class ChordTopology : public TopologyAPI
{
   public:
      ChordTopology(Profile& config, 
                    Dispatcher& dispatcher, 
                    Transporter& transporter);
      virtual ~ChordTopology();

      virtual void joinOverlay();
      
      // need a fifo to receive timer events 

      // Messages that the forwarding layer sends to this object
      virtual void newConnectionFormed( const NodeId& node, bool inbound );
      virtual void connectionLost( const NodeId& node );
       
      // deal with topology change messages 
      virtual void consume(ConnectReq& msg);
      virtual void consume(JoinReq& msg);
      virtual void consume(UpdateReq& msg);
      virtual void consume(LeaveReq& msg);

      // deal with responses
      virtual void consume(ConnectAns& msg);
      virtual void consume(JoinAns& msg);
      virtual void consume(UpdateAns& msg);
      virtual void consume(LeaveAns& msg);
   
      // called when store set completes, cases update to get sent
      // virtual void consume(EventWrapper<StoreSetFoo>& event);

      // Deal with routing querries 
      virtual const NodeId& findNextHop( const DestinationId& did);
      virtual const NodeId& findNextHop( const NodeId& node );
      virtual const NodeId& findNextHop( const ResourceId& resource );
      
      // Deal with replication for storage 
      virtual std::vector<NodeId> getReplicationSet( const ResourceId& resource );
      ///    Returns list of nodes to replicate on

      // Functions to find out if this peer is responsible for something
      virtual bool isResponsible( const DestinationId& did ) const;
      virtual bool isResponsible( const NodeId& node ) const;
      virtual bool isResponsible( const ResourceId& resource ) const;

      // Function to determine if we are connected to a node
      virtual bool isConnected( const NodeId& node ) const;

      // Function to hash resource names into resourceID 
      virtual ResourceId resourceId( const resip::Data& resourceName );

      // not public api
      virtual void post(std::auto_ptr<Event> message);
              
   private:
      
      std::set<NodeId> mFingerTable;
      std::vector<NodeId> mPrevTable;
      std::vector<NodeId> mNextTable;
      bool mJoined;

      typedef std::map<UInt64, Message* > PendingResponseMap;
      PendingResponseMap mPendingResponses;

      bool addNewNeighbors(  const std::vector<NodeId>& nodes, bool adjustNextOnly ); // return true if
                                                          // anything changed
      bool addNewFingers( const std::vector<NodeId>& nodes ); // return true if changed
      void buildFingerTable();
      void sendUpdates();
      
      // Attach subsystem
      void attach(const NodeId &attachTo);
      void startCandidateCollection(const UInt64 tid, const NodeId &id);
      virtual void candidatesCollected( UInt64 tid, 
                                        const NodeId& node, 
                                        unsigned short appId, 
                                        std::vector<Candidate>& candidates);
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
