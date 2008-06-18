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
      ChordTopology(Profile& config, Dispatcher& dispatcher, Transporter& transporter);
      virtual ~ChordTopology();

      virtual void joinOverlay();
      
      // need a fifo to receive timer events 

      // Messages that the forwarding layer sends to this object
      virtual void newConnectionFormed( NodeId& node );
      virtual void connectionLost( NodeId& node );
      virtual void candidatesCollected( NodeId& node, std::vector<Candidate>& candidates)=0;
       
      // deal with topology change messages 
      virtual void consume(JoinReq& msg);
      virtual void consume(UpdateReq& msg);
      virtual void consume(LeaveReq& msg);

      virtual void consume(ConnectAns& msg);
   
      // called when store set completes, cases update to get sent
      // virtual void consume(EventWrapper<StoreSetFoo>& event);

      // Deal with routing querries 
      virtual const NodeId& findNextHop( NodeId& node );
      virtual const NodeId& findNextHop( ResourceId& resource );
      
      // Deal with replication for storage 
      virtual std::vector<NodeId> getReplicationSet(  ResourceId& resource );
      ///    Returns list of nodes to replicate on

      // Functions to find out if this peer is responsible for something
      virtual bool isResponsible( NodeId& node );
      virtual bool isResponsible( ResourceId& resource );

      // Function to determine if we are connected to a node
      virtual bool isConnected( NodeId& node );

      // Function to hash resource names into resourceID 
      virtual ResourceId resourceId( resip::Data& resourceName );
              
   private:
      
      std::set<NodeId> mFingerTable;
      std::vector<NodeId> mPrevTable;
      std::vector<NodeId> mNextTable;

      bool addNewNeighbors(  std::vector<NodeId>& nodes ); // return true if
                                                          // anything changed
      bool addNewFingers( std::vector<NodeId>& nodes ); // return true if changed
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
