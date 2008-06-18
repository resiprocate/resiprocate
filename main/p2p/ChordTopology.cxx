
#include "rutil/Data.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/SHA1Stream.hxx"

#include "p2p/ChordTopology.hxx"
#include "p2p/Profile.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"
#include "p2p/Dispatcher.hxx"
#include "p2p/Connect.hxx"

using namespace p2p;

ChordTopology::ChordTopology(Profile& config, Dispatcher& dispatcher, Transporter& transporter) :
   TopologyAPI(config, dispatcher, transporter) 
{
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
ChordTopology::newConnectionFormed( NodeId& node )
{
   // If this is the first connection we ever have - then it must be the connection to
   // the bootstrap node
   if(mFingerTable.size() == 0 && mNextTable.size() == 0)
   {
      // add the bootstrap node to our next table 
      mNextTable[0] = node;

      // send the ConnectReq to our NodeID to get NP 
      std::auto_ptr<Message> connectReq(new ConnectReq);
      mDispatcher.send(connectReq);
      
// callback here 
      // sent connect reqest to NNP

      // send connected to bunch of fingers 

      // send join to AP 
   }
   else
   {
      // go and add this to the finger table
      assert(mFingerTable.find(node) == mFingerTable.end());
      mFingerTable.insert(node);
   }
}


void 
ChordTopology::connectionLost( NodeId& node )
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


// deal with topoogy change messages 
void 
ChordTopology::consume(EventWrapper<JoinReq>& event)
{
   // check we are reponsible for the data from this node 

   // send the data using multiple store messages over to the new node 
   // use a StoreSet to monitor 

   // update the replicated data storage    

// wait for all data to be stored 

   // send them an update to put the joining node in the ring 
}


void 
ChordTopology::consume(EventWrapper<UpdateReq>& event)
{
   // if our, prev empty, then this update will have the prev and need to
   // connect to them and set the prev 

   // see if this changes the neighbor tables and if it does send updates to all
   // peers in prev/next table 
}


void 
ChordTopology::consume(EventWrapper<LeaveReq>& event)
{
   // if this is in the prev/next table, remove it and send updates 
   assert(0);
}


// Deal with routing queries 
const NodeId& 
ChordTopology::findNextHop( NodeId& node )
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
ChordTopology::findNextHop( ResourceId& resource )
{
   NodeId node( resource.value() );
   return findNextHop( node );
}


// Deal with replication for storage 
std::vector<NodeId> 
ChordTopology::getReplicationSet(  ResourceId& resource )
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
ChordTopology::isResponsible( NodeId& node )
{
   if (mPrevTable.size() == 0) return false;

   if (  (mPrevTable[0] < node) && (node <= mProfile.nodeId()) )
   {
      return true;
   }
   return false;
}


bool 
ChordTopology::isResponsible( ResourceId& resource )
{
  NodeId node( resource.value() );
  return isResponsible( node );
}


// Function to hash resource names into resourceID 
ResourceId 
ChordTopology::resourceId( resip::Data& resourceName )
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
ChordTopology::addNewNeighbors(std::vector<NodeId>& nodes)
{
   // This function takes a list of nodes and merges them into next and prev
   // tables. If anything changes, it sends updates 

   assert( nodes.size() > 0 );
   bool changed=false;
   
   for (unsigned int n=0; n<nodes.size(); n++ )
   {
      NodeId node(nodes[n]);
      
      if (mNextTable.size() == 0)
      {
         mNextTable[0] = node; changed=true;
      }
      else if ( (mProfile.nodeId() < node) && (node < mNextTable[0]) )
      {
         mNextTable[0] = node; changed=true;
      }
      
      if (mPrevTable.size() == 0)
      {
         mPrevTable[0] = node; changed=true;
      }
      else if ( (mPrevTable[0] < node) && (node < mProfile.nodeId()) )
      {
         mPrevTable[0] = node; changed=true;
      }   
   }
   
   return changed;
}


bool 
ChordTopology::addNewFingers(std::vector<NodeId>& nodes)
{
   assert( nodes.size() > 0 );
   bool changed=false;

   // iterate through finger table and check if we are actually adding new nodes
   std::vector<NodeId>::iterator it = nodes.begin();
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
