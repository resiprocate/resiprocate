#include "p2p/Message.hxx"
#include "p2p/Join.hxx"
#include "p2p/Update.hxx"
#include "p2p/Leave.hxx"

#include "rutil/SHA1Stream.hxx"

#include <assert.h>

using namespace p2p;

Message::Message(ResourceId rid) :
	mResourceId(rid)
{
	mVersion = 0x1; // set by the draft
}

Message::~Message() 
{

}

void
Message::setOverlayName(const resip::Data &overlayName)
{
	mOverlayName = overlayName;
}

Message *
Message::makeErrorResponse(Message::Error::Code code, const resip::Data& reason) const
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
	Message::MessageType messageType = UpdateReqType; // remove me
	Message *newMessage = 0;
	
	// parse the forwarding header
	
	switch(messageType)
	{
		case UpdateReqType:
			break;
		case UpdateAnsType:
			break;
		case JoinReqType:
			break;
		case JoinAnsType:
			break;
		default:
			assert(0); // unknown value
	}
	
	return newMessage;
}

void 
Message::copyForwardingData(const Message &header)
{
	mOverlay = header.mOverlay;
	mTransactionId = header.mTransactionId;
}


void 
Message::decrementTTL()
{
	assert(mTtl);
	mTtl--;
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
	assert(getMessageType() == JoinReqType);

	JoinReq *req = static_cast<JoinReq *>(this);
	JoinAns *response = new JoinAns(req, overlaySpecific);

	return response;
}

LeaveAns *
Message::makeLeaveResponse() 
{
	assert(getMessageType() == LeaveReqType);

	LeaveReq *req = static_cast<LeaveReq *>(this);
	return new LeaveAns(req);
}

UpdateAns *
Message::makeUpdateResponse()
{
	assert(getMessageType() == UpdateReqType);

	UpdateReq *req = static_cast<UpdateReq *>(this);
	UpdateAns *response = new UpdateAns(req);
	return response;
}

resip::Data
Message::encodePayload()
{
	assert(mOverlayName.size());	// user needs to call setOverlayName
	
	// create the overlay field from the overlay name
	resip::SHA1Stream stream;
	stream << mOverlayName;
	resip::Data sha1 = stream.getBin(32);
	mOverlay = ntohl(*reinterpret_cast<const UInt32 *>(sha1.c_str()));

	mMessageCode = static_cast<UInt16>(getMessageType());

	resip::Data encodedData;
	resip::DataStream encodedStream(encodedData);

	// encode forwarding header
	encode(encodedStream);

	encodedStream.flush();
	size_t startOfPayload = encodedData.size();

	// encode specific message payload
	getEncodedPayload(encodedStream);

	encodedStream.flush();
	size_t endOfPayload = encodedData.size();

	// compute signature block
    std::vector<resip::Data> sigChunks;
	sigChunks.push_back(resip::Data(resip::Data::Borrow, encodedData.data() + 4, 4));	// overlay
	sigChunks.push_back(resip::Data(resip::Data::Borrow, encodedData.data() + 16, 8));	// transaction id
	sigChunks.push_back(resip::Data(resip::Data::Borrow, encodedData.data() + startOfPayload, endOfPayload - startOfPayload));	// transaction id

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



