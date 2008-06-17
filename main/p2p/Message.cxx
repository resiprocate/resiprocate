#include "p2p/Message.hxx"
#include "p2p/Join.hxx"
#include "p2p/Update.hxx"
#include "p2p/Leave.hxx"

#include <assert.h>

using namespace p2p;

Message::Message(ResourceId rid, const resip::Data& overlayName) :
	mResourceId(rid),
	mOverlayName(overlayName)
{

}

Message::~Message() 
{

}

Message *
Message::makeErrorResponse(ErrorResponseCode code, const resip::Data& reason) const
{
	assert(0);
	return 0;
}


bool
Message::isRequest() const
{
	unsigned int reqValue = static_cast<unsigned int>(getMessageType());
	return ((reqValue % 2) == 1);
}

Message *
Message::parse(const resip::Data &message, NodeId senderID)
{
	// placeholder
	Message::MessageType messageType = UpdateReq; // remove me
	Message *newMessage = 0;
	
	// parse the forwarding header
	
	switch(messageType)
	{
		case UpdateReq:
			break;
		case UpdateAns:
			break;
		case JoinReq:
			break;
		case JoinAns:
			break;
		default:
			assert(0); // unknown value
	}
	
	return newMessage;
}

UInt8 
Message::getTTL() const
{
	return mTtl;
}

UInt32 
Message::getOverlay() const
{
	return mOverlay;
}

UInt64 
Message::getTransactionID() const
{
	return mTransactionId;
}

UInt16 
Message::getFlags() const 
{
	return mFlags;
}

JoinAns *
Message::makeJoinResponse(const resip::Data &overlaySpecific)
{
	p2p::JoinAns *response = new p2p::JoinAns(overlaySpecific);
	return response;
}

LeaveAns *
Message::makeLeaveResponse() 
{
	return new p2p::LeaveAns();
}

UpdateAns *
Message::makeUpdateResponse()
{
	p2p::UpdateAns *response = new p2p::UpdateAns;
	return response;
}

resip::Data
Message::encode() const
{
	resip::Data encodedData;

	// encode forwarding header

	// ask message type to encode it's payload
	getPayload(encodedData);

	// we should optimize this eventually to avoid this copy
	return encodedData;
}

std::vector<resip::Data> 
Message::collectSignableData() const
{
	assert(0);
   std::vector<resip::Data> list;
   return list;
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



