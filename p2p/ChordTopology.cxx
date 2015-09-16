
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ssl/SHA1Stream.hxx"

#include "p2p/Candidate.hxx"
#include "p2p/ChordTopology.hxx"
#include "p2p/ChordUpdate.hxx"
#include "p2p/Connect.hxx"
#include "p2p/Dispatcher.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"
#include "p2p/P2PSubsystem.hxx"
#include "p2p/Profile.hxx"
#include "p2p/Update.hxx"

using namespace p2p;

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

ChordTopology::ChordTopology(Profile& config,
                             Dispatcher& dispatcher, 
                             Transporter& transporter) :
   TopologyAPI(config, dispatcher, transporter), mJoined(false)
{
}

   
ChordTopology::~ChordTopology()
{
}


void 
ChordTopology::joinOverlay()
{
   // tell the transport layer to form a connection to bootstrap node (special
   // bootstrap connect ). This needs to give up the address of the BP node 
   mTransporter.connect(mProfile.bootstrapNodes().front());
   DebugLog(<< "attempting to connect to bootstrap node");
}


// Messages that the forwarding layer sends to this object
void 
ChordTopology::newConnectionFormed( const NodeId& node, bool inbound )
{
   DebugLog(<< "newConnectionFormed to: " << node << " (" << (inbound ? "inbound" : "outbound") << ")");

   if(!inbound)
   {
      // If this is the first connection we have - then it must be the connection to
      // the bootstrap node
     if(mFingerTable.size() == 0 && mNextTable.size() == 0 && !mProfile.isBootstrap())
      {
        // collect candidates for our NodeId
         attach(mProfile.nodeId());

         // Build finger table
         buildFingerTable();
      }

      // If we are not joined yet and this connection is to our Admitting Peer (next peer)
      // the send a join
      if(mNextTable.size() == 1 && node == mNextTable[0])
      {
         DebugLog(<< "sending join to node: " << node);
   	   DestinationId destination(node);
         std::auto_ptr<Message> joinReq(new JoinReq(destination, mProfile.nodeId()));
         mDispatcher.send(joinReq, *this);      
      }

      // go and add this to the finger table
      resip_assert(mFingerTable.find(node) == mFingerTable.end());
      mFingerTable.insert(node);
   }
}


void 
ChordTopology::connectionLost( const NodeId& node )
{
   DebugLog(<< "connectionLost to: " << node);

   // if node is in the finger table, remove it 
   mFingerTable.erase(node);

   // if node is in the mPrevTable, remove it
   std::vector<NodeId>::iterator it = mPrevTable.begin();
   for(; it != mPrevTable.end(); it++)
   {
      if(*it == node) 
      {
         mPrevTable.erase(it);
      }
   }

   // if this is in the mNextTable, remove it
   it = mNextTable.begin();
   for(; it != mNextTable.end(); it++)
   {
      if(*it == node) 
      {
         mPrevTable.erase(it);
      }
   }

   // TODO - go get another finger table entry?
}


void 
ChordTopology::consume(ConnectReq& msg)
{
   DebugLog(<< "received CONNECT req.");

   // Build Connect Response and store to be sent out after candidate collection completes
   // Real data will be filled out later
   mPendingResponses[msg.getTransactionId()] = msg.makeConnectResponse(resip::Data::Empty /* frag */, 
                                                                       resip::Data::Empty /* password */,
                                                                       RELOAD_APPLICATION_ID,
                                                                       resip::Data::Empty /* role */,
                                                                       msg.getCandidates() /* candidates */);  

   // Collect candidates for response
   startCandidateCollection(msg.getTransactionId(), msg.getResponseNodeId() /* TODO - we really want to retrieve sending NodeId */);

   // Socket connect - once ice is integrated this likely needs to move to after the candidates are collected
   resip::GenericIPAddress stunTurnServer;
   mTransporter.connect(msg.getResponseNodeId(), msg.getCandidates(), stunTurnServer /* stunTurnServer */);
}


// deal with topoogy change messages 
void 
ChordTopology::consume(JoinReq& msg)
{
   DebugLog(<< "received JOIN req from: " << msg.getNodeId());

   // check we are reponsible for the data from this node 
   if(!isResponsible(msg.getNodeId()))
   {
      // send error response - not responsible
      std::auto_ptr<Message> errorAns(msg.makeErrorResponse(Message::Error::NotFound, "Not responsible for this Join"));
      mDispatcher.send(errorAns, *this);
      return;
   }

   // TODO - send the data using multiple store messages over to the new node 
   // use a StoreSet to monitor 

   // TODO - update the replicated data storage    

   // wait for all data to be stored 

   // send them an update to put the joining node in the ring 
   std::vector<NodeId> nodes;
   nodes.push_back(msg.getNodeId());
   if(addNewNeighbors(nodes, false /* adjustNextOnly */))
   {
      sendUpdates();
   }

   std::auto_ptr<Message> joinAns(msg.makeJoinResponse(resip::Data::Empty /* Overlay specific */));
   mDispatcher.send(joinAns, *this);
}


void 
ChordTopology::consume(UpdateReq& msg)
{
   DebugLog(<< "received JOIN req from: ?");

   // if our, prev empty, then this update will have the prev and need to
   // connect to them and set the prev 
   ChordUpdate cordUpdate( msg.getRequestMessageBody() );
   
   if(addNewNeighbors(cordUpdate.getPredecessors(), false /* adjustNextOnly */) ||
      addNewNeighbors(cordUpdate.getSuccessors(), false /* adjustNextOnly */))
   {
      sendUpdates();
   }

   std::auto_ptr<Message> udpateAns(msg.makeUpdateResponse(resip::Data::Empty /* Overlay specific */));
   mDispatcher.send(udpateAns, *this);
}


void 
ChordTopology::consume(LeaveReq& msg)
{
   DebugLog(<< "received LEAVE req from: ?");

   // if this is in the prev/next table, remove it and send updates 
   resip_assert(0);

   std::auto_ptr<Message> leaveAns(msg.makeLeaveResponse());
   mDispatcher.send(leaveAns, *this);
}


void 
ChordTopology::consume(ConnectAns& msg)
{
   DebugLog(<< "received CONNECT ans from: " << msg.getResponseNodeId());
   
}


void 
ChordTopology::consume(JoinAns& msg)
{
   DebugLog(<< "received JOIN ans from: " << msg.getResponseNodeId());

   // TODO check response code?
   mJoined = true;
}


void 
ChordTopology::consume(UpdateAns& msg)
{
   DebugLog(<< "received UPDATE ans from: " << msg.getResponseNodeId());

   // TODO check response - and log?
}


void 
ChordTopology::consume(LeaveAns& msg)
{
   DebugLog(<< "received LEAVE ans from: " << msg.getResponseNodeId());

   // TODO check response - and log?
}


// Deal with routing queries 
const NodeId& 
ChordTopology::findNextHop( const NodeId& node )
{
   resip_assert( !isResponsible(node) );  
   
   if(mFingerTable.size() == 0)
   {
      // return the next pointer and increment around slowly 
      resip_assert( mNextTable.size() > 0 );
      DebugLog(<< "findNextHop returning (from next table): " << mNextTable[0]);

      // TODO - deal with case wehre there is no next 

      return mNextTable[0];
   }

   std::set<NodeId>::iterator it = mFingerTable.begin();
   for(;it != mFingerTable.end(); it++)
   {      
      std::set<NodeId>::iterator nextIt = it;
      nextIt++;
      if(nextIt == mFingerTable.end()) break;
      if((*it <= node) && (node < *nextIt))
      {
         DebugLog(<< "findNextHop returning (from finger table): " << *it);
         return *it;
      }
   }
   DebugLog(<< "findNextHop returning: " << *it);
   return *it;    
}


const NodeId& 
ChordTopology::findNextHop( const ResourceId& resource )
{
   NodeId node( resource.value() );
   return findNextHop( node );
}


const NodeId& 
ChordTopology::findNextHop( const DestinationId& did )
{
   if(did.isNodeId())
   {
      return findNextHop(did.asNodeId());
   }
   else if(did.isResourceId())
   {
      return findNextHop(did.asResourceId());
   }
   else
   {
      resip_assert(false);
      static NodeId none;
      return none;
   }
}


// Deal with replication for storage 
std::vector<NodeId> 
ChordTopology::getReplicationSet(  const ResourceId& resource )
{
   std::vector<NodeId> replicateSet;
   const unsigned int numReplications=2;
   
   for ( unsigned int i=0; i<numReplications; i++)
   {
      if (mNextTable.size() > i )
      {
         replicateSet[i] = mNextTable[i];
      }
   }
   
   return replicateSet;
}


// Functions to find out if this peer is responsible for something
bool 
ChordTopology::isResponsible( const NodeId& node ) const
{
   // for now we are going to assume that if the prev table is empty, then we have a
   // a new ring being formed and we are responsible for this request.  We need to consider
   // the case where we are a node trying to join a stable ring, and the "join" process 
   // has not completed yet - but we receive a message for another node.
   if (mPrevTable.size() == 0) 
   {
      if ( mProfile.isBootstrap() )
      {
         return true;  // only thing in the ring so responsible for everything 
      }
      else
      {
         return false; // have not joined ring yet so responsible for nothing 
      }
   }
   
   if ( (mPrevTable[0] < node) && (node <= mProfile.nodeId()) )
   {
      return true;
   }
   return false;
}


bool 
ChordTopology::isResponsible( const ResourceId& resource ) const
{
  NodeId node( resource.value() );
  return isResponsible( node );
}

bool 
ChordTopology::isResponsible( const DestinationId& did ) const
{
   if(did.isNodeId())
   {
      return isResponsible(did.asNodeId());
   }
   else if(did.isResourceId())
   {
      return isResponsible(did.asResourceId());
   }
   else
   {
      resip_assert(false);
      return false;
   }
}


// Functions to find out if this peer is responsible for something
bool 
ChordTopology::isConnected( const NodeId& node ) const
{
   // Check if node is in finger table
   if(mFingerTable.find(node) != mFingerTable.end())
   {
      return true;
   }

   return false;
}


// Function to hash resource names into resourceID 
ResourceId 
ChordTopology::resourceId( const resip::Data& resourceName )
{
   // call sha1, truncate to 128 bits, and return 
    resip::SHA1Stream strm;
    strm << resourceName;
    resip::ParseBuffer pb(strm.getBin());
    const char* anchor = pb.position();
    pb.skipN(128);
    resip::Data result;
    pb.data(result, anchor);
 
    return ResourceId(result);
}


void 
ChordTopology::post(std::auto_ptr<Event> event)
{
   //will run in same thread as the dispatcher 
   DebugLog(<< "ChordTopology received: " << event->brief());
   event->dispatch(*this);
}

bool 
ChordTopology::addNewNeighbors(const std::vector<NodeId>& nodes, bool adjustNextOnly)
{
   // This function takes a list of nodes and merges them into next and prev
   // tables. If anything changes, it sends updates 

   resip_assert( nodes.size() > 0 );
   bool changed=false;
   
   for (unsigned int n=0; n<nodes.size(); n++ )
   {
      bool setNext = false;
      bool setPrev = false;
      if (mNextTable.size() == 0)
      {
         setNext = true;
      }
      else if ( (mProfile.nodeId() < nodes[n]) && (nodes[n] < mNextTable[0]) )
      {
         setNext = true;
      }

      if(!adjustNextOnly)
      {
         if (mPrevTable.size() == 0)
         {
            setPrev = true;
         }
         else if ( (mPrevTable[0] < nodes[n]) && (nodes[n] < mProfile.nodeId()) )
         {
            setPrev = true;
         }   
      }

      if(setNext)
      {
         DebugLog(<< "new next neighbour added: " << nodes[n]);
         mNextTable[0] = nodes[n]; 
      }

      if(setPrev)
      {
         DebugLog(<< "new prev neighbour added: " << nodes[n]);
         mPrevTable[0] = nodes[n]; 
      }

      if(setNext || setPrev)
      {
         changed=true;
         // kick start connection to newly added node if not admitting peer addition
         DebugLog(<< "collecting candidates for new neighbour: " << nodes[n]);
         if(!adjustNextOnly) attach(nodes[n]);  
      }
   }
   
   return changed;
}

// May not actually be needed
bool 
ChordTopology::addNewFingers(const std::vector<NodeId>& nodes)
{
   resip_assert( nodes.size() > 0 );
   bool changed=false;

   // iterate through finger table and check if we are actually adding new nodes
   std::vector<NodeId>::const_iterator it = nodes.begin();
   for(;it != nodes.end(); it++)
   {
      std::set<NodeId>::iterator it2 = mFingerTable.find(*it);
      if(it2 == mFingerTable.end())
      {
         changed = true;
         mFingerTable.insert(*it);
      }
   }
   return changed;
}


// TODO - need timers that periodically call this
void 
ChordTopology::buildFingerTable()
{
   resip_assert(mProfile.numInitialFingers() <= 127);
   for(unsigned int i = 0; i < mProfile.numInitialFingers(); i++)
   {
      NodeId fingerNodeId = mProfile.nodeId().add2Pow(127-i);
      DebugLog(<< "collecting candidates for fingertable: " << fingerNodeId);
      attach(fingerNodeId);
   }
}


void
ChordTopology::sendUpdates()
{
   // Create our update message
   ChordUpdate ourUpdate;
   ourUpdate.setUpdateType(ChordUpdate::Neighbors);

   // set our predecessors and successors
   ourUpdate.setPredecessors(mPrevTable);
   ourUpdate.setSuccessors(mNextTable);

   // TODO - put our nodeid somewhere in the message?

   // if this changes the neighbor tables and if it does send updates to all
   // peers in prev/next table 
   std::vector<NodeId>::iterator it = mPrevTable.begin();
   for(; it != mPrevTable.end(); it++)
   {   
      DebugLog(<< "sending update to prev neighbour: " << *it);
      DestinationId destination(*it);
      std::auto_ptr<Message> updateReq(new UpdateReq(destination, ourUpdate.encode()));
      mDispatcher.send(updateReq, *this);
   }

   it = mNextTable.begin();
   for(; it != mNextTable.end(); it++)
   {
      DebugLog(<< "sending update to next neighbour: " << *it);
	   DestinationId destination(*it);
      std::auto_ptr<Message> updateReq(new UpdateReq(destination, ourUpdate.encode()));
      mDispatcher.send(updateReq, *this);
   }
}



// Connection management subsystem... This will eventually need to be
// moved somewhere else, but for now...

void 
ChordTopology::attach(const NodeId &attachTo)
{
   DebugLog(<< "Attaching to node " << attachTo);

   startCandidateCollection(0,attachTo);
}

void
ChordTopology::startCandidateCollection(const UInt64 tid, const NodeId &attachTo)
{
   DebugLog(<< "Starting candidate collection");
   
   mTransporter.collectCandidates(tid, attachTo);
}


void 
ChordTopology::candidatesCollected(UInt64 tid,
                                   const NodeId& node, unsigned short appId, std::vector<Candidate>& candidates)
{
   
   if(tid==0)
   {
      DebugLog(<< "candidateCollection completed, sending CONNECT req to: " << node);

      // We're initiating 

      // This needs to be a resourceId to ensure correct routing
      // hack-o-rama
      ResourceId rid(node.encodeToNetwork());
      DestinationId destination(rid);

      std::auto_ptr<Message> connectReq(new ConnectReq(destination, resip::Data::Empty /* icefrag */, resip::Data::Empty /* password */, appId, resip::Data::Empty /* ice tcp role */, candidates));
      mDispatcher.send(connectReq, *this);
   }
   else
   {
      DebugLog(<< "candidateCollection completed, sending CONNECT response to: " << node);
      // We're responding

      // Find pending response
      PendingResponseMap::iterator it = mPendingResponses.find(tid);
      if(it != mPendingResponses.end())
      {
         if(it->second->getType() == Message::ConnectAnsType)
         {
            // Fill in the remaining Connect Response data
            ConnectAns* connectAns = (ConnectAns*)it->second;
            connectAns->setCandidates(candidates);
            mDispatcher.send(std::auto_ptr<Message>(it->second), *this);
            // Send Connect Ans
            mPendingResponses.erase(it);
         }
         else
         {
            // Response not of expected type
            resip_assert(false);
         }
      }
      else
      {
         // Response not found
         resip_assert(false);
      }
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
