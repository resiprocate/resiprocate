#include <algorithm>
#include "rutil/ResipAssert.h"

#include "rutil/Data.hxx"
#include "p2p/DestinationId.hxx"

using namespace p2p;

DestinationId::DestinationId(s2c::DestinationStruct s) : s2c::DestinationStruct(s)
{
}

DestinationId::DestinationId(const NodeId& nid) 
{
   mPeer.mNodeId = new s2c::NodeIdStruct;
   *(mPeer.mNodeId) = nid.getNodeIdStruct();
   mResource.mResourceId = 0;
	mType = s2c::peer;
}

DestinationId::DestinationId(const ResourceId& rid)
{
   mPeer.mNodeId = 0;
   mResource.mResourceId = new s2c::ResourceIdStruct;
   mResource.mResourceId->mId = rid.value();
	mType = s2c::resource;
}

bool
DestinationId::isNodeId() const
{
   return (mType == s2c::peer);
}

NodeId
DestinationId::asNodeId() const
{
   resip_assert(isNodeId());
   resip_assert(mPeer.mNodeId);
   return NodeId(*mPeer.mNodeId);
}

bool
DestinationId::isCompressedId()const
{
   return (mType == s2c::compressed);
}

CompressedId
DestinationId::asCompressedId() const
{
   resip_assert(isCompressedId());
   return CompressedId(mCompressed.mCompressedId);
}

bool
DestinationId::isResourceId()const
{
   return (mType == s2c::resource);
}

ResourceId
DestinationId::asResourceId() const
{
   resip_assert(isResourceId());
   resip_assert(mResource.mResourceId);
   return ResourceId(mResource.mResourceId->mId);
}

bool
DestinationId::operator==(const NodeId& nid) const
{
   return (isNodeId() && nid == asNodeId());
}

bool
DestinationId::operator==(const DestinationId& nid) const
{
   if (nid.isNodeId() && this->isNodeId())
   {
      return nid.asNodeId() == this->asNodeId();
   }
   else if (nid.isResourceId() && this->isResourceId())
   {
      return nid.asResourceId() == this->asResourceId();
   }
   else if (nid.isCompressedId() && this->isCompressedId())
   {
      return nid.asCompressedId() == this->asCompressedId();
   }
   return false;
}

s2c::DestinationStruct* 
DestinationId::copyDestinationStruct() const
{
   return new s2c::DestinationStruct(*this);
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
