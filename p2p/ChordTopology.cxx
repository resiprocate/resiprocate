
#include "rutil/Data.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/SHA1Stream.hxx"

#include "p2p/ChordTopology.hxx"
#include "p2p/ConfigObject.hxx"
#include "p2p/FlowId.hxx"
#include "p2p/Message.hxx"
#include "p2p/ChordNodeId.hxx"

using namespace p2p;


//Chord::Chord(ConfigObject& config, TransactionLayer& transactionProcessor )
//{
//}
      

// Messages that the forwarding layer sends to this object
void Chord::newConnectionFormed( NodeId& node )
{
   // go and add this to the finger table
}


void Chord::connectionLost( NodeId& node )
{
   // if this is in the finger table, remove it 
   assert(0);
}


// deal with topoogy change messages 
void Chord::consume(EventWrapper<JoinReq>& event)
{
   // check we are reponsivle for the data from this node 

   // send the data using multiple store messages over to the new node 
   // use a StoreSet to monitor 

   // update the replicated data storage 

   
}


void Chord::consume(EventWrapper<UpdateReq>& event)
{
   // see if this changes the neighbor tables and if it does send updates
}


void Chord::consume(EventWrapper<LeaveReq>& event)
{
   // if this is in the prev/next table, remove it and send updates 
   assert(0);
}


// Deal with routing querries 
NodeId& Chord::findNextHop( NodeId& pNode )
{
   ChordNodeId node(pNode);
   
   assert( !isResponsible(pNode) );
   
   // find if one of the finger table entries is the best one 
   for ( unsigned int i=0; i<mFingerTable.size()-1; i++)
   {
      if ( (mFingerTable[i] <= node ) && ( node < mFingerTable[i+1] ) )
      {
         return mFingerTable[i];
      }
   }
   
   // find if the last finger table entry is the best one 
   if ( mFingerTable.size() > 0 )
   {
      if ( (mFingerTable[mFingerTable.size()-1] <= node ) && ( node < myNodeId ) )
      {
         return mFingerTable[mFingerTable.size()-1];
      }
   }
   
   // nothing found, return the next pointer and increment around slowly 
   assert( mNextTable.size() > 0 );
   return mNextTable[0];
}


NodeId& Chord::findNextHop( ResourceId& resource )
{
   ChordNodeId node( resource.value() );
   return findNextHop( node );
}


// Deal with replication for storage 
std::vector<NodeId> Chord::getReplicationSet(  ResourceId& resource )
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
bool Chord::isResponsible( NodeId& pNode )
{
   ChordNodeId node(pNode);
   
   if (mPrevTable.size() == 0) return false;

   if (  (mPrevTable[0]<node) && (node<=myNodeId) )
   {
      return true;
   }
   return false;
}


bool Chord::isResponsible( ResourceId& resource )
{
  ChordNodeId node( resource.value() );
  return isResponsible( node );
}


// Function to hash resource names into resourceID 
ResourceId Chord::resourceId( resip::Data& resourceName )
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


Chord::~Chord()
{
}


bool Chord::addNewNeighbors(  std::vector<NodeId> nodes )
{
   // This function takes a list of nodes and merges them into next and prev
   // tables. If anything changes, it sends updates 

   assert( nodes.size() > 0 );
   bool changed=false;
   
   for (unsigned int n=0; n<nodes.size(); n++ )
   {
      ChordNodeId node(nodes[n]);
      
      if (mNextTable.size() == 0)
      {
         mNextTable[0] = node; changed=true;
      }
      else if ( (myNodeId<node) && (node<mNextTable[0]) )
      {
         mNextTable[0] = node; changed=true;
      }
      
      if (mPrevTable.size() == 0)
      {
         mPrevTable[0] = node; changed=true;
      }
      else if ( (mPrevTable[0]<node) && (node<myNodeId) )
      {
         mPrevTable[0] = node; changed=true;
      }   
   }
   
   return changed;
}


bool Chord::addNewFingers( std::vector<NodeId> nodes )
{
   assert( nodes.size() > 0 );
   bool changed=false;
  
/* 
   std::vector<NodeId> set = mFingerTable;
   set.append( nodes );
   sort( set.begin() , set.end() );
   unique( set.begin() , set.end() );
   
   // TODO filter the finger for log 2 stuff 

   if ( set == mFingerTable )
   {
      // no change happened 
      return false;
   }

   mFingerTable = set;
*/

   return true;
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
