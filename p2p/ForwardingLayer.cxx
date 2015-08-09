#include "p2p/ForwardingLayer.hxx"
#include "p2p/Dispatcher.hxx"
#include "p2p/P2PSubsystem.hxx"

namespace p2p
{

#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

void
ForwardingLayer::process(int ms)
{
   Event *event = mRxFifo.getNext(ms);
   if (event) 
   {
      DebugLog(<< "ForwardingLayer received:" << event->brief());

      event->dispatch(*this);
   }
}


void 
ForwardingLayer::forward( std::auto_ptr<Message> m )
{
   // get the destination node from m 
   DebugLog(<< "ForwardingLayer: message arrived");

  redo:
   DestinationId did = m->nextDestination();

   if (did.isCompressedId())
   {
      resip_assert(0);
   }
   else if (did.isNodeId())
   {
      DebugLog(<< "ForwardingLayer: this is a node-id");
      
      if (did == mProfile.nodeId()) // this is me
      {
         DebugLog(<< "ForwardingLayer: addressed to me");
         
         m->popNextDestinationId();
         if (!m->isDestinationListEmpty())
         {
            DebugLog(<< "ForwardingLayer: more entries on destination list. Reentering forwarding loop");
            goto redo;
         }
         else
         { 
            DebugLog(<< "ForwardingLayer: no more entries on destination list. Posting.");
            mDispatcher.post(m);
         }
      }
      else // not me
      {
         DebugLog(<< "ForwardingLayer: not addressed to me");
         
         if (mTopology.isConnected(did.asNodeId()))
         { 
            DebugLog(<< "ForwardingLayer: forwarding to directly connected node");
            
            mTransporter.send(did.asNodeId(), m); 
         }
         else if(mTopology.isResponsible(did.asNodeId()))
         {
            // We own this section of space so we'd know about this node if
            // it existed
            DebugLog(<< "ForwardingLayer: dropping packet");
         }
         else
         {
            // We're not responsible, try to route to someone who is
            DebugLog(<< "ForwardingLayer: routing to next hop");
            
            mTransporter.send(mTopology.findNextHop(did), m);
         }
      }
   }
   else // resourceID
   {
      resip_assert (did.isResourceId());
      
      DebugLog(<< "ForwardingLayer: destination is resource-id");
      
      if(mTopology.isResponsible(did.asResourceId()))
      {
         m->popNextDestinationId();
         if (m->isDestinationListEmpty())
         {
            DebugLog(<< "ForwardingLayer: delivering");
            
            mDispatcher.post(m);            
         }
         else
         {
            DebugLog(<< "ForwardingLayer: discarding illegal destination list (resource-id at nonterminal position)");
            
            // drop on the floor
         }
      }
      else
      {
         // Not responsible so try to route 
         DebugLog(<< "ForwardingLayer: routing to next hop");
         
         mTransporter.send(mTopology.findNextHop(did.asResourceId()), m);
      }
   }
}


void 
ForwardingLayer::consume(ConnectionOpened& m)
{
   DebugLog(<< "ForwardingLayer: new connection formed");

   if(m.getApplication() == RELOAD_APPLICATION_ID)
   {
      mTopology.newConnectionFormed(m.getNodeId(), m.isInbound());
   }
   else
   {
      // TODO - notify application that new connection is formed
   }
}

void 
ForwardingLayer::consume(ConnectionClosed& m)
{
   DebugLog(<< "ForwardingLayer: connection closed");

   if(m.getApplicationId() == RELOAD_APPLICATION_ID)
   {
      mTopology.connectionLost(m.getNodeId());   
   }
   else
   {
      // TODO - notify application that connection is closed
   }
}

void 
ForwardingLayer::consume(MessageArrived& m)
{
   DebugLog(<< "ForwardingLayer: message arrived");

   forward(m.getMessage());
}

void 
ForwardingLayer::consume(ApplicationMessageArrived& m)
{
   // do nothing for now
}

void 
ForwardingLayer::consume(LocalCandidatesCollected& m)
{
   // pass to the TopologyAPI
   if(m.getAppId() == RELOAD_APPLICATION_ID)
   {
      mTopology.candidatesCollected(m.getTransactionId(),
                                    m.getNodeId(), m.getAppId(), m.getCandidates());
   }
   else
   {
      // TODO - notify application that candidates are collected
   }
}

void 
ForwardingLayer::post(std::auto_ptr<Event> event)
{
   event->dispatch(*this);
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
