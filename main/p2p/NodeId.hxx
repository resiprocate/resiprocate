#ifndef __P2P_NODE_ID_HXX
#define __P2P_NODE_ID_HXX 1

#include "rutil/Compat.hxx"

namespace resip
{
class Data;
}

namespace p2p
{

// This is pretty much wrong, but it serves as a good placeholder at
// the moment.

class NodeId
{
   public:
	  NodeId() { mValue[0] = mValue[1] = 0; }
      NodeId(const resip::Data& data);
      NodeId& operator=(const resip::Data& data);
      NodeId& operator=(const NodeId& data);
      
      const resip::Data getValue() const;

      bool operator<(const NodeId& rhs) const;
      bool operator<=(const NodeId& rhs) const;
      bool operator==(const NodeId& rhs) const;
      
   private:
      // NOTE: this should be 128 bits
      unsigned char mValue[16];
};
   

class CompressedId
{
};

// Either a CompressedId, NodeId or ResourceId
class DestinationId
{
   public:
      NodeId asNodeId() const;
      
      bool isCompressed() const;
      bool isDestinationId() const;
      bool isNodeId() const;
      bool isResourceId() const;

      bool operator==(const NodeId& nid) const;
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
