#ifndef __P2P_TOPOLOGYAPI_HXX
#define __P2P_TOPOLOGYAPI_HXX 1

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"

#include "p2p/NodeId.hxx"
#include "p2p/ResourceId.hxx"
#include "p2p/ConfigObject.hxx"


namespace p2p
{

class ResourceId;
class ReloadMessage;

/// This is an abstract base class from which to derive the actually topology plugins
class TopologyAPI
{
   public:
      TopologyAPI(ConfigObject& config, TranscationLayer& transactionProcessor );
      
      // need a fifo to receive timer events 

      // Messages that the forwarding layer sends to this object
      void newConnectionFormed( NodeId& node );
      void connectionLost( NodeId& node );
       
      // deal with topoogy change messages 
<<<<<<< .mine
      void processJoin( Message& message );
      void processUpdate(  Message& message );
      void processLeave(  Message& message );
=======
      void processJoin( ReloadMessage& message );
      void processUpdate( ReloadMessage& message );
      void processLeave( ReloadMessage& message );
>>>>>>> .r7783
      
      // Deal with routing querries 
      NodeId& findNextHop( NodeId& node );
      NodeId& findNextHop( ResourceId& resource );
      
      // Deal with replication for storage 
      std::vector<NodeId> getReplicationSet(  ResourceId& resource );
      ///    Returns list of nodes to replicate on

      // Functions to find out if this peer is responsible for something
      bool isResponsible( NodeId& node );
      bool isResponsible( ResourceId& resource );
      
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
