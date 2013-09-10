#include "p2p/Join.hxx"

using namespace p2p;

JoinAns::JoinAns()
{

}

JoinAns::JoinAns(p2p::JoinReq *request, const resip::Data &overlaySpecific)
{
   mOverlaySpecificData = overlaySpecific;
   
   copyForwardingData(*request);
}

void 
JoinAns::getEncodedPayload(resip::DataStream &strm) 
{
   encode(strm);
}

void 
JoinAns::decodePayload(resip::DataStream &strm)
{
	decode(strm);
}

JoinReq::JoinReq()
{

}

JoinReq::JoinReq(const DestinationId &dest, const NodeId &node, const resip::Data &overlaySpecific) :
	mNodeId(node)
{
   mJoiningPeerId = new s2c::NodeIdStruct;
   *mJoiningPeerId = node.getNodeIdStruct();
   
   mOverlaySpecificData = overlaySpecific;
}

NodeId 
JoinReq::getNodeId() const
{
	return NodeId(*mJoiningPeerId);
}


void
JoinReq::getEncodedPayload(resip::DataStream &strm) 
{
   encode(strm);
}

void
JoinReq::decodePayload(resip::DataStream &strm)
{
	decode(strm);
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

