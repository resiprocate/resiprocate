#include "p2p/ForwardingLayer.hxx"
#include "p2p/Dispatcher.hxx"

namespace p2p
{

void
ForwardingLayer::process(int ms)
{
   Event *event = mRxFifo.getNext(ms);
   if (event) 
   {
      event->dispatch(*this);
   }
}


void 
ForwardingLayer::forward( std::auto_ptr<Message> m )
{
   // get the destination node from m 
  redo:
   DestinationId did = m->nextDestination();
   if (did.isCompressedId())
   {
      assert(0);
   }
   else if (mTopology.isResponsible(did))
   {
      if (did.isNodeId())
      {
         if (did == mProfile.nodeId()) // this is me
         {
            m->popNextDestinationId();
            if (m->isDestinationListEmpty())
            {
               goto redo;
            }
            else
            {
               mDispatcher.post(m);
            }
         }
         else // not me
         {
            if (mTopology.isConnected(did.asNodeId()))
            {
               mTransporter.send(did.asNodeId(), m); 
            }
            else
            {
               // drop on the floor
            }
         }
      }
      else // resourceID
      {
         assert (did.isResourceId());
         m->popNextDestinationId();
         if (m->isDestinationListEmpty())
         {
            mDispatcher.post(m);            
         }
         else
         {
            // drop on the floor
         }
      }
   }
   else // not responsible and not compressed
   {
      mTransporter.send(mTopology.findNextHop(did), m);
   }
}


void 
ForwardingLayer::consume(ConnectionOpened& m)
{
   mTopology.newConnectionFormed(m.getNodeId());
}

void 
ForwardingLayer::consume(ConnectionClosed& m)
{
   mTopology.connectionLost(m.getNodeId());   
}

void 
ForwardingLayer::consume(MessageArrived& m)
{
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
   // mTopology.candidatesCollected();
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
