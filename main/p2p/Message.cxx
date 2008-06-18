#include "p2p/Message.hxx"
#include "p2p/Join.hxx"
#include "p2p/Update.hxx"
#include "p2p/Connect.hxx"
#include "p2p/Leave.hxx"
#include "p2p/Event.hxx"

#include "rutil/SHA1Stream.hxx"
#include "rutil/Socket.hxx" 
#include "rutil/Log.hxx"

#include <assert.h>

using namespace p2p;
using namespace s2c;
using namespace resip;

const UInt8 Message::MessageVersion = 0x1;
const UInt8 Message::MessageTtl = 0x20;

#include "p2p/P2PSubsystem.hxx"
#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

Message::Message(ResourceId rid) :
	mResourceId(rid)
{
	// not really sure what this ctor does it
	initForwardingData();
}

Message::Message()
{
	initForwardingData();
}

void
Message::initForwardingData()
{
	mPDU.mHeader = new ForwardingHeaderStruct();
	mPDU.mHeader->mVersion = MessageVersion; // set by the draft
	mPDU.mHeader->mTransactionId = 	
								(static_cast<UInt64>(rand()) << 48) |
							 	(static_cast<UInt64>(rand()) << 32) |
							 	(static_cast<UInt64>(rand()) << 16) |
					 			(static_cast<UInt64>(rand()));

	mPDU.mHeader->mTtl = Message::MessageTtl;
}

Message::~Message() 
{

}

void
Message::setOverlayName(const resip::Data &overlayName)
{
	mOverlayName = overlayName;

	// create the overlay field from the overlay name
	resip::SHA1Stream stream;
	stream << mOverlayName;
	mPDU.mHeader->mOverlay = stream.getUInt32();
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
	unsigned int reqValue = static_cast<unsigned int>(getType());
	return ((reqValue % 2) == 1);
}

Message *
Message::parse(const resip::Data &message)
{
	Message *newMessage = 0;

	resip::Data copyData = message;
	resip::DataStream stream(copyData);

	ForwardingHeaderStruct header;
	header.decode(stream);

	// figure out what type of message this is
	Message::MessageType messageType = static_cast<Message::MessageType>(header.mMessageCode);
	
	// parse the forwarding header
	
	switch(messageType)
	{
		case UpdateReqType:
			DebugLog(<< "UpdateReqType message received");
			newMessage = new UpdateReq();
			break;
		case UpdateAnsType:
			DebugLog(<< "UpdateAnsType message received");
			newMessage = new UpdateAns();
			break;
		case JoinReqType:
			DebugLog(<< "JoinReqType message received");
			newMessage = new JoinReq();
			break;
		case JoinAnsType:
			DebugLog(<< "JoinAns message received");
			newMessage = new JoinAns();
			break;
		case LeaveReqType:
			DebugLog(<< "LeaveReq message received");
			newMessage = new LeaveReq();
			break;
		case LeaveAnsType:
			DebugLog(<< "LeaveAns message received");
			newMessage = new LeaveAns();
			break;
		case ConnectReqType:
			DebugLog(<< "ConnectReq message received");
			newMessage = new ConnectReq();
			break;
		case ConnectAnsType:
			DebugLog(<< "ConnectAns message received");
			newMessage = new ConnectAns();
			break;
		default:
			DebugLog(<< "Unhandled message");
			assert(0); // unknown value
	}

	newMessage->decodePayload(stream);
	
	return newMessage;
}

void 
Message::copyForwardingData(const Message &header)
{
	mPDU.mHeader->mOverlay = header.mPDU.mHeader->mOverlay;		
	mPDU.mHeader->mTransactionId = header.mPDU.mHeader->mTransactionId;
}


void 
Message::decrementTTL()
{
	assert(mPDU.mHeader->mTtl);
	mPDU.mHeader->mTtl--;
}

UInt8 
Message::getTTL() const
{
	return mPDU.mHeader->mTtl;
}

void
Message::setTTL(UInt8 ttl)
{
	mPDU.mHeader->mTtl = ttl;
}

UInt32 
Message::getOverlay() const
{
	return mPDU.mHeader->mOverlay;
}

UInt64 
Message::getTransactionID() const
{
	return mPDU.mHeader->mTransactionId;
}

UInt16 
Message::getFlags() const 
{
	return mPDU.mHeader->mFlags;
}

JoinAns *
Message::makeJoinResponse(const resip::Data &overlaySpecific)
{
	assert(getType() == JoinReqType);

	JoinReq *req = static_cast<JoinReq *>(this);
	JoinAns *response = new JoinAns(req, overlaySpecific);

	return response;
}

LeaveAns *
Message::makeLeaveResponse() 
{
	assert(getType() == LeaveReqType);

	LeaveReq *req = static_cast<LeaveReq *>(this);
	return new LeaveAns(req);
}

UpdateAns *
Message::makeUpdateResponse()
{
	assert(getType() == UpdateReqType);

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

	mPDU.mHeader->mMessageCode = static_cast<UInt16>(getType());
   mPDU.mHeader->mOverlay = stream.getUInt32();
   
	// TODO: Set flag to something goofy
  	mPDU.mHeader->mFlags = 0xfeeb;

	resip::Data encodedData;
	resip::DataStream encodedStream(encodedData);

	// encode forwarding header
	mPDU.mHeader->encode(encodedStream);

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

	s2c::SignatureStruct *sigBlock = sign(sigChunks);
	mPDU.mSig = sigBlock;

	mPDU.mSig->encode(encodedStream);
	encodedStream.flush();

	size_t finalLength = encodedData.size();
	std::cout << "final size: " << finalLength << std::endl;

	// add the length to the header
	assert(mPDU.mHeader->mVersion);
	UInt32 *lengthWord = reinterpret_cast<UInt32 *>(const_cast<char *>(encodedData.data()) + 12);
	(*lengthWord) = (*lengthWord | (htonl(finalLength & 0xfff) >> 8));
	std::cout << *lengthWord << std::endl;

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

bool 
Message::isDestinationListEmpty() const
{
   assert(0);
   return true;
}

DestinationId 
Message::nextDestination() const
{
   assert(0);
   DestinationId did;
   return did;
}

void 
Message::popNextDestinationId()
{
   assert(0);
}



std::auto_ptr<Event> 
Message::event()
{
   assert(0);
   return std::auto_ptr<Event>(0);
}

void
Message::pushVia(NodeId id)
{
   assert(0);
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



