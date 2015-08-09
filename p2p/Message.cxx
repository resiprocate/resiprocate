#include "p2p/Message.hxx"
#include "p2p/Join.hxx"
#include "p2p/Update.hxx"
#include "p2p/Connect.hxx"
#include "p2p/Leave.hxx"
#include "p2p/Event.hxx"
#include "p2p/DestinationId.hxx"

#include "rutil/ssl/SHA1Stream.hxx"
#include "rutil/Socket.hxx" 
#include "rutil/Log.hxx"

#include <openssl/rand.h>
#include "rutil/ResipAssert.h"

using namespace p2p;
using namespace s2c;
using namespace resip;

const UInt8 Message::MessageVersion = 0x1;
const UInt8 Message::MessageTtl = 0x20;
const UInt32 Message::MessageReloToken=0xd2454c4f;

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

Message::Message(const DestinationId &dest)
{
    // this is the constructor for requests
	initForwardingData();

	// add to the destination list here
}

void
Message::dump() const
{
	DebugLog(<< "type: " << mPDU.mHeader->mMessageCode << ", txid: " << mPDU.mHeader->mTransactionId);
}

NodeId 
Message::getResponseNodeId() const
{
	// todo grab from via
	NodeId n;	
	return n;
}

bool
Message::compareDestinationLists(const std::vector<DestinationStruct *> &l1, const std::vector<DestinationStruct *> &l2) const
{
	std::vector<DestinationStruct*>::const_iterator iter1 = l1.begin();
	std::vector<DestinationStruct*>::const_iterator iter2 = l2.begin();

	bool isOk = true;
	while (iter1 != l1.end() && iter2 != l2.end() && isOk)
	{
		DestinationId id1(**iter1);
		DestinationId id2(**iter2);

		isOk = isOk && (id1 == id2);
	
		++iter1;
		++iter2;
	}

	return isOk;
}

bool 
Message::operator==(const Message& msg) const
{
	DebugLog(<< "txid1: " << mPDU.mHeader->mTransactionId << ", txid2: " << msg.mPDU.mHeader->mTransactionId);

	bool headerPayloadOk =  
		(mPDU.mHeader->mReloToken == msg.mPDU.mHeader->mReloToken) &&
		(mPDU.mHeader->mOverlay == msg.mPDU.mHeader->mOverlay) &&
		(mPDU.mHeader->mTtl == msg.mPDU.mHeader->mTtl) &&
		(mPDU.mHeader->mReserved == msg.mPDU.mHeader->mReserved) &&
		(mPDU.mHeader->mFragment == msg.mPDU.mHeader->mFragment) &&
//		(mPDU.mHeader->mLength == msg.mPDU.mHeader->mLength);  && 
		(mPDU.mHeader->mTransactionId == msg.mPDU.mHeader->mTransactionId)  &&
		(mPDU.mHeader->mFlags == msg.mPDU.mHeader->mFlags) &&
		(mPDU.mHeader->mRouteLogLenDummy == msg.mPDU.mHeader->mRouteLogLenDummy)  &&
		(getType() == msg.getType()) &&
		(mPDU.mHeader->mViaList.size() == msg.mPDU.mHeader->mViaList.size()) &&
		(mPDU.mHeader->mDestinationList.size() == msg.mPDU.mHeader->mDestinationList.size())  &&
		(mPDU.mPayload == msg.mPDU.mPayload);  

	DebugLog(<< "operator state " << headerPayloadOk);

	// check Vias
	if (headerPayloadOk)
	{
		headerPayloadOk = (headerPayloadOk && compareDestinationLists(mPDU.mHeader->mDestinationList, msg.mPDU.mHeader->mDestinationList));
	}

	DebugLog(<< "operator state 2 " << headerPayloadOk);

	if (headerPayloadOk)
	{
		headerPayloadOk = (headerPayloadOk && compareDestinationLists(mPDU.mHeader->mViaList, msg.mPDU.mHeader->mViaList));
	}

	DebugLog(<< "operator state 3 " << headerPayloadOk);

	// check sig
	return headerPayloadOk;
}


void
Message::initForwardingData()
{
	mPDU.mHeader = new ForwardingHeaderStruct();
	mPDU.mHeader->mVersion = MessageVersion; // set by the draft
    
    // Random transaction ID
    RAND_bytes((unsigned char *)&mPDU.mHeader->mTransactionId,4);

	mPDU.mHeader->mTtl = Message::MessageTtl;

	// remove me
	setOverlayName("test");
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
	resip_assert(0);
	return 0;
}

resip::Data 
Message::getRequestMessageBody() const
{
	resip_assert(isRequest());

	return mRequestMessageBody;
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
    DebugLog(<< "Transaction ID =" << std::hex << header.mTransactionId << "TTL=" <<
             std::hex << (int)header.mTtl);

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
			resip_assert(0); // unknown value
	}

	// let's decode the payload
	MessagePayloadStruct payloadStruct;
	payloadStruct.decode(stream);
	newMessage->mRequestMessageBody = payloadStruct.mPayload;
	*newMessage->mPDU.mHeader = header;
	
	DebugLog(<< "Message Body Payload Size Is " << newMessage->mRequestMessageBody.size());
	resip::DataStream payloadStream( newMessage->mRequestMessageBody );

	// get the signature
	SignatureStruct signature;
	signature.decode(stream);

	// todo: verify the signature here
	newMessage->decodePayload(payloadStream);
	
	return newMessage;
}

void 
Message::copyForwardingData(const Message &header)
{
   resip_assert(header.isRequest());
   mPDU.mHeader->mOverlay = header.mPDU.mHeader->mOverlay;		
   mPDU.mHeader->mTransactionId = header.mPDU.mHeader->mTransactionId;
   
   resip_assert(mPDU.mHeader->mDestinationList.empty());
   std::vector<DestinationStruct*>::reverse_iterator i;
   for (i=header.mPDU.mHeader->mViaList.rbegin(); i!=header.mPDU.mHeader->mViaList.rend(); ++i)
   {
      DestinationStruct* ds = new DestinationStruct(**i);
      mPDU.mHeader->mDestinationList.push_back(ds);
   }
}

void 
Message::decrementTTL()
{
	resip_assert(mPDU.mHeader->mTtl);
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
Message::getTransactionId() const
{
	return mPDU.mHeader->mTransactionId;
}

UInt16 
Message::getFlags() const 
{
	return mPDU.mHeader->mFlags;
}

ConnectAns *
Message::makeConnectResponse(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<Candidate> &candidates)
{
	resip_assert(getType() == ConnectReqType);

	ConnectReq *req = static_cast<ConnectReq *>(this);
	ConnectAns *response = new ConnectAns(req, frag, password, application, role, candidates);

	return response;
}


JoinAns *
Message::makeJoinResponse(const resip::Data &overlaySpecific)
{
	resip_assert(getType() == JoinReqType);

	JoinReq *req = static_cast<JoinReq *>(this);
	JoinAns *response = new JoinAns(req, overlaySpecific);

	return response;
}

LeaveAns *
Message::makeLeaveResponse() 
{
	resip_assert(getType() == LeaveReqType);

	LeaveReq *req = static_cast<LeaveReq *>(this);
	return new LeaveAns(req);
}

UpdateAns *
Message::makeUpdateResponse(const resip::Data &overlaySpecific)
{
	resip_assert(getType() == UpdateReqType);
	
	UpdateReq *req = static_cast<UpdateReq *>(this);
	UpdateAns *response = new UpdateAns(req,overlaySpecific);

	return response;
}

resip::Data
Message::encodePayload()
{
        
	resip_assert(mOverlayName.size());	// user needs to call setOverlayName
	
	// create the overlay field from the overlay name
	resip::SHA1Stream stream;
	stream << mOverlayName;
        mPDU.mHeader->mReloToken=Message::MessageReloToken;
	mPDU.mHeader->mMessageCode = static_cast<UInt16>(getType());
   mPDU.mHeader->mOverlay = stream.getUInt32();
   
	// TODO: Set flag to something goofy
  	mPDU.mHeader->mFlags = 0xfeeb;

	resip::Data encodedData;
	resip::DataStream encodedStream(encodedData);
        
        int fff=encodedStream.tellp();
        std::cerr << fff;

	// encode forwarding header
	mPDU.mHeader->encode(encodedStream);

	encodedStream.flush();
	size_t startOfPayload = encodedData.size();

	// encode specific message payload
	MessagePayloadStruct msgPayload;
	{
		resip::DataStream payloadStream(msgPayload.mPayload);
		getEncodedPayload(payloadStream);
	}

	msgPayload.encode(encodedStream);

	encodedStream.flush();
	size_t endOfPayload = encodedData.size();

	DebugLog(<< "Encoded Payload Size Is: " << (endOfPayload - startOfPayload));
        

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

	// add the length to the header
	resip_assert(mPDU.mHeader->mVersion);
	UInt32 *lengthWord = reinterpret_cast<UInt32 *>(const_cast<char *>(encodedData.data()) + 13);

	(*lengthWord) = (*lengthWord | (htonl(finalLength & 0x0fff) >> 8));

	// we should optimize this eventually to avoid this copy
	return encodedData;
}

std::vector<resip::Data> 
Message::collectSignableData() const
{
	resip_assert(0);
   std::vector<resip::Data> list;
   return list;
}

bool 
Message::isDestinationListEmpty() const
{
   return mPDU.mHeader->mDestinationList.empty();
}

DestinationId 
Message::nextDestination() const
{
   resip_assert(!isDestinationListEmpty());
   return DestinationId(*mPDU.mHeader->mDestinationList.front());
}

void 
Message::popNextDestinationId()
{
   resip_assert(!isDestinationListEmpty());
   mPDU.mHeader->mDestinationList.erase(mPDU.mHeader->mDestinationList.begin());
}

std::auto_ptr<Event> 
Message::event()
{
   resip_assert(0);
   return std::auto_ptr<Event>(0);
}

void
Message::pushVia(const DestinationId& did)
{
   resip_assert(!did.isResourceId());
   mPDU.mHeader->mViaList.push_back(did.copyDestinationStruct());
}

void
Message::pushDestinationId(const DestinationId& did)
{
   mPDU.mHeader->mDestinationList.push_back(did.copyDestinationStruct());
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



