#ifndef __P2P_NODE_ID_HXX
#define __P2P_NODE_ID_HXX

#include <iosfwd>
#include "p2p/MessageStructsGen.hxx"

namespace p2p
{

class ResourceId;

class NodeId
{
   public:
      NodeId();
      NodeId(const s2c::NodeIdStruct& nid);
      NodeId(const ResourceId& rid);
      NodeId& operator=(const NodeId& data);

      bool operator<(const NodeId& rhs) const;
      bool operator<=(const NodeId& rhs) const;
      bool operator==(const NodeId& rhs) const;
      
      // returns a node ID that is the value of this node plus 2^power passed
      // in. In chord terms, add2Pow( 127 ) is half way around the ring 
      NodeId add2Pow( int power ) const; 

      const s2c::NodeIdStruct& getNodeIdStruct() const;
      const resip::Data  encodeToNetwork() const;

      friend std::ostream& operator<<( std::ostream& strm, const NodeId& node );

   private:
      // NOTE: this should be 128 bits
      s2c::NodeIdStruct mNodeId;

};

class CompressedId
{
   public:
      CompressedId(const resip::Data& cid);
      bool operator==(const CompressedId& rhs) const;
};

std::ostream& operator<<( std::ostream& strm, const p2p::NodeId& node );

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
