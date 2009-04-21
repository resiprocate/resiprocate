#ifndef __P2P_FLOW_ID_HXX
#define __P2P_FLOW_ID_HXX 1

#include "rutil/Socket.hxx"
#include "rutil/Fifo.hxx"
#include "p2p/NodeId.hxx"

#include <iosfwd>

namespace p2p
{

class Event;

class FlowId
{
   public:
      FlowId(NodeId nodeId, 
             unsigned short application, 
             resip::Socket &socket,
             resip::Fifo<Event>& fifo)
         : mNodeId(nodeId), mApplication(application), 
           mFifo(fifo),
           mDescriptor(socket)
           {;}

      resip::Socket getSocket() const { return mDescriptor; }

      unsigned short getApplication() const {return mApplication;}

      const NodeId &getNodeId() const {return mNodeId;}

      resip::Fifo<Event> &getFifo() {return mFifo;}
      
   private:
      NodeId mNodeId;
      unsigned short mApplication;
      resip::Fifo<Event> &mFifo;

      // This is descriptor for now; it changes to something else
      // when we add ICE
      resip::Socket mDescriptor;
};
std::ostream& operator<<( std::ostream& strm, const FlowId& node );

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
