
#include "rutil/Data.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/SHA1Stream.hxx"

#include "p2p/ChordTopology.hxx"
#include "p2p/Profile.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"
#include "p2p/Dispatcher.hxx"
#include "p2p/Candidate.hxx"
#include "p2p/Connect.hxx"
#include "p2p/Update.hxx"
#include "p2p/ChordUpdate.hxx"

using namespace p2p;

ChordTopology::ChordTopology(Profile& config, Dispatcher& dispatcher, Transporter& transporter) :
   TopologyAPI(config, dispatcher, transporter) 
{
   // register with dispatcher for reload request events
   mDispatcher.registerPostable(Message::JoinReqType, *this);
   mDispatcher.registerPostable(Message::UpdateReqType, *this);
   mDispatcher.registerPostable(Message::LeaveReqType, *this);

   // register with dispatcher for reload response events
   mDispatcher.registerPostable(Message::ConnectAnsType, *this);
}
      
void 
ChordTopology::joinOverlay()
{
   // tell the transport layer to form a connection to bootstrap node (special
   // bootstrap connect ). This needs to give up the address of the BP node 
   mTransporter.connect(mProfile.bootstrapNodes().front());
}


// Messages that the forwarding layer sends to this object
void 
ChordTopology::newConnectionFormed( const NodeId& node )
{
   // If this is the first connection we have - then it must be the connection to
   // the bootstrap node
   if(mFingerTable.size() == 0 && mNextTable.size() == 0)
   {
      // collect candidates for our NodeId
      mTransporter.collectCandidates(mProfile.nodeId());

      // Build finger table
      buildFingerTable();
   }

   // go and add this to the finger table
   assert(mFingerTable.find(node) == mFingerTable.end());
   mFingerTable.insert(node);
}


void 
ChordTopology::connectionLost( const NodeId& node )
{
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
ChordTopology::candidatesCollected( const NodeId& node, unsigned short appId, std::vector<Candidate>& candidates)
{
   // Connect to node - send the ConnectReq
   std::vector<resip::Data> dataCandidates;
   std::vector<Candidate>::iterator it = candidates.begin();
   for(;it != candidates.end(); it++)
   {
      dataCandidates.push_back(resip::Data::Empty);  // Todo convert Candidate to Data (in SDP format)
   }
   std::auto_ptr<Message> connectReq(new ConnectReq(resip::Data::Empty /* icefrag */, resip::Data::Empty /* password */, appId, resip::Data::Empty /* ice tcp role */, dataCandidates));
   mDispatcher.send(connectReq);
      
// callback here 

      // sent connect reqest to NNP

      // send connect to bunch of fingers 

      // send join to AP 

}


// deal with topoogy change messages 
void 
ChordTopology::consume(JoinReq& msg)
{
   // check we are reponsible for the data from this node 

   // send the data using multiple store messages over to the new node 
   // use a StoreSet to monitor 

   // update the replicated data storage    

// wait for all data to be stored 

   // send them an update to put the joining node in the ring 
}


void 
ChordTopology::consume(UpdateReq& msg)
{
   // if our, prev empty, then this update will have the prev and need to
   // connect to them and set the prev 
   ChordUpdate cordUpdate( msg.getRequestMessageBody() );
   if(addNewNeighbors(cordUpdate.getPredecessors(), false /* adjustNextOnly */) ||
      addNewNeighbors(cordUpdate.getSuccessors(), false /* adjustNextOnly */))
   {
      // Create our update message
      ChordUpdate ourUpdate;
      ourUpdate.setUpdateType(ChordUpdate::Neighbors);
      // TODO set our predecessors and successors

      // if this changes the neighbor tables and if it does send updates to all
      // peers in prev/next table 
      std::vector<NodeId>::iterator it = mPrevTable.begin();
      for(; it != mPrevTable.end(); it++)
      {         
         std::auto_ptr<Message> updateReq(new UpdateReq(ourUpdate.encode()));
         mDispatcher.send(updateReq);
      }

      it = mNextTable.begin();
      for(; it != mNextTable.end(); it++)
      {
         std::auto_ptr<Message> updateReq(new UpdateReq(ourUpdate.encode()));
         mDispatcher.send(updateReq);
      }
   }
}


void 
ChordTopology::consume(LeaveReq& msg)
{
   // if this is in the prev/next table, remove it and send updates 
   assert(0);
}


void 
ChordTopology::consume(ConnectAns& msg)
{
   // Get NodeId from message and check if it is a neighbour
   std::vector<NodeId> nodes;
   nodes.push_back(msg.getResponseNodeId());
   addNewNeighbors(nodes, true /* adjustNextOnly */);

   // Socket connect
   //mTransporter.connect(   TODO
}


// Deal with routing queries 
const NodeId& 
ChordTopology::findNextHop( const NodeId& node )
{
   assert( !isResponsible(node) );
   
   if(mFingerTable.size() == 0)
   {
      // return the next pointer and increment around slowly 
      assert( mNextTable.size() > 0 );
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
         return *it;
      }
   }
   return *it;    
}


const NodeId& 
ChordTopology::findNextHop( const ResourceId& resource )
{
   NodeId node( resource.value() );
   return findNextHop( node );
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
ChordTopology::isResponsible( const NodeId& node )
{
   if (mPrevTable.size() == 0) return false;

   if (  (mPrevTable[0] < node) && (node <= mProfile.nodeId()) )
   {
      return true;
   }
   return false;
}


bool 
ChordTopology::isResponsible( const ResourceId& resource )
{
  NodeId node( resource.value() );
  return isResponsible( node );
}


// Functions to find out if this peer is responsible for something
bool 
ChordTopology::isConnected( const NodeId& node )
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


ChordTopology::~ChordTopology()
{
}


bool 
ChordTopology::addNewNeighbors(const std::vector<NodeId>& nodes, bool adjustNextOnly)
{
   // This function takes a list of nodes and merges them into next and prev
   // tables. If anything changes, it sends updates 

   assert( nodes.size() > 0 );
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
         mNextTable[0] = nodes[n]; 
      }

      if(setNext || setPrev)
      {
         changed=true;
         // kick start connection to newly added node if not admitting peer addition
         if(!adjustNextOnly) mTransporter.collectCandidates(nodes[n]);  
      }
   }
   
   return changed;
}

// May not actually be needed
bool 
ChordTopology::addNewFingers(const std::vector<NodeId>& nodes)
{
   assert( nodes.size() > 0 );
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


void 
ChordTopology::buildFingerTable()
{
   assert(mProfile.numInitialFingers() <= 127);
   for(unsigned int i = 0; i < mProfile.numInitialFingers(); i++)
   {
      NodeId fingerNodeId = mProfile.nodeId().add2Pow(127-i);
      mTransporter.collectCandidates(fingerNodeId);     
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
