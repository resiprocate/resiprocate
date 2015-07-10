#include <algorithm>
#include "rutil/ResipAssert.h"

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "p2p/NodeId.hxx"
#include "p2p/ResourceId.hxx"

using namespace p2p;

NodeId::NodeId() 
{
   resip_assert( sizeof(mNodeId.mHigh) == 8 );
}

NodeId::NodeId(const s2c::NodeIdStruct& nid) : mNodeId(nid)
{
}

NodeId::NodeId(const ResourceId& rid)
{
   resip_assert(rid.value().size() == 16);
   resip::Data buffer(rid.value());
   resip::iDataStream strm(buffer);
   mNodeId.decode(strm);
}

NodeId& NodeId::operator=(const NodeId& rhs)
{
   mNodeId = rhs.mNodeId;
   return *this;
}

bool 
NodeId::operator<(const NodeId &r) const
{
   return ( (mNodeId.mHigh < r.mNodeId.mHigh) || 
            (mNodeId.mHigh == r.mNodeId.mHigh && mNodeId.mLow < r.mNodeId.mLow));
}

bool
NodeId::operator<=(const NodeId& rhs) const
{
   return *this == rhs || *this < rhs;
}

bool
NodeId::operator==(const NodeId& rhs) const
{
   return !(*this < rhs) && !(rhs < *this);
}

NodeId
NodeId::add2Pow( int power ) const
{
   resip_assert( power < 128 );
   resip_assert( power >= 0 );
   NodeId ret;

   ret = *this;
   
   if ( power >= 64 )
   {
      // working on high word 
      power -= 64;
      if (power>0) 
      {
         long long inc = 1;
         inc <<= (power-1);
         ret.mNodeId.mHigh += inc;
      }
   }
   else
   {
      // working on low word
       if (power>0) 
      {
         long long inc = 1;
         inc <<= (power-1);
         ret.mNodeId.mLow += inc;
      }
   }
   
   return ret;
}

const s2c::NodeIdStruct& 
NodeId::getNodeIdStruct() const
{
   return mNodeId;
}


const resip::Data
NodeId::encodeToNetwork() const
{
  resip::Data d;
  resip::DataStream strm(d);
   
   mNodeId.encode(strm);
   strm.flush();
   
   return d;
}

std::ostream& 
p2p::operator<<( std::ostream& strm, const NodeId& node )
{
   strm << "H:" << node.mNodeId.mHigh << " L:" << node.mNodeId.mLow;
   return strm;
}


CompressedId::CompressedId(const resip::Data& cid)
{
   resip_assert(0);
}

bool
CompressedId::operator==(const CompressedId& rhs) const
{
   return false;
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
