#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Data.hxx"
#include "p2p/ChordUpdate.hxx"
#include "p2p/Join.hxx"
#include "p2p/Connect.hxx"
#include "p2p/Update.hxx"
#include "p2p/MessageStructsGen.hxx"
#include "p2p/Candidate.hxx"
#include "rutil/DataStream.hxx"

#include "p2p/P2PSubsystem.hxx"
#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

#include <iostream>

using namespace s2c;
using namespace resip; // approved by Adam
using namespace p2p;

void
TestUpdate()
{	
	NodeId node;
	DestinationId dest(node);

	// Test Update Request
	resip::Data noData;
	UpdateReq *update = new UpdateReq(dest, noData);
	assert(update->getType() == Message::UpdateReqType);

	resip::Data encodedMessage = update->encodePayload();

	Message *compareMessage = Message::parse(encodedMessage);
	assert(*compareMessage == *update);

	// Test responses
	UpdateAns *updateAns = update->makeUpdateResponse(noData);
	assert(updateAns->getType() == Message::UpdateAnsType);

	// Check transaction ID
	assert(updateAns->getTransactionId() == update->getTransactionId());
}

void
TestChordUpdate()
{
	NodeId node;
	DestinationId dest(node);

	ChordUpdate update;
	UpdateReq *updateReq = new UpdateReq(dest, update.encode());
	resip::Data encodedMessage = updateReq->encodePayload();

	Message *compareMessage = Message::parse(encodedMessage);
	assert(*compareMessage == *updateReq);

	resip::Data reqBlob = updateReq->getRequestMessageBody();
	ChordUpdate update2(reqBlob);
	assert(update2 == update);

	UpdateAns *updateAns = updateReq->makeUpdateResponse(update.encode());
	assert(updateAns);

	assert(updateAns->getTransactionId() == updateReq->getTransactionId());
}

void
TestJoin()
{

}

void
TestMessages()
{
	TestUpdate();
	TestChordUpdate();
}

int main() 
{
	resip::Log::initialize(resip::Data("cerr"),
	resip::Data("DEBUG"),resip::Data("ParsingTest"));
      
	std::cout << "ctor" << std::endl;
	ForwardingLayerMessageStruct hdr;
	std::cout << "done" << std::endl;

	NodeId node;
	DestinationId dest(node);

	resip::Data d;
	std::vector<Candidate> v;
	Message *m2 = new ConnectReq(dest, d, d, 0, d, v);

	p2p::NodeId n;
	p2p::Message *m = new p2p::JoinReq(dest, n,d);
	m->setOverlayName("duane");
	resip::Data encodedMessage = m->encodePayload();

	DebugLog(<< "attempting to parse");

	Message *newMessage = Message::parse(encodedMessage);

	std::cout << encodedMessage.hex() << std::endl;

	ForwardingLayerMessageStruct hdr2;
	resip::DataStream is(encodedMessage);
	hdr2.decode(is);

	TestMessages();

//	Data d;
//	DataStream ds(d);
//	hdr.encode(ds);
//	Data foo = Data::from(hdr);
	return 0;
}
