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
#include "rutil/Random.hxx"

#include "p2p/P2PSubsystem.hxx"
#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

#include <iostream>

using namespace s2c;
using namespace resip; // approved by Adam
using namespace p2p;

void
TestConnect()
{
	NodeId node;
	DestinationId dest(node);
	resip::Data noData;

	//const DestinationId &dest, const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<Candidate> &candidates

	resip::Data password = "password";
	UInt16 application = 100;
	resip::Data role = "test";

	std::vector<Candidate> candidates;

	ConnectReq *connect = new ConnectReq(dest, noData, password, application, role, candidates);
	assert(connect);
	assert(connect->getType() == Message::ConnectReqType);

	resip::Data encodedData = connect->encodePayload();
	assert(encodedData.size());

	Message *message = Message::parse(encodedData);
	assert(message->getType() == Message::ConnectReqType);
	assert(*message == *connect);

	ConnectReq *connectReq = static_cast<ConnectReq *>(message);
	assert(connectReq->getPassword() == password);
	assert(connectReq->getRole() == role);
	assert(connectReq->getApplication() == application);

	Candidate c("candidate:1 1 UDP 2130706431 10.0.1.1 8998 typ host");
	assert(connectReq->getCandidates().empty());
	
	candidates.push_back(c);
	ConnectReq *connect2 = new ConnectReq(dest, noData, password, application, role, candidates);
	assert(connect2);

	resip::Data encodedData2 = connect2->encodePayload();
	Message *message2 = Message::parse(encodedData2);
	assert(message2);

	ConnectReq *connectReq3 = static_cast<ConnectReq *>(message2);
	assert(connectReq3);
	assert(!connectReq3->getCandidates().empty());

	std::vector<Candidate> newCandidates = connectReq3->getCandidates();
	assert(newCandidates[0].getIceString() == c.getIceString());
}

void
TestUpdate()
{	
	NodeId node;
	DestinationId dest(node);

	// Test Update Request
	resip::Data noData;
	UpdateReq *update = new UpdateReq(dest, noData);
	assert(update->getType() == Message::UpdateReqType);

	// check for non zero encoded payload
	resip::Data encodedMessage = update->encodePayload();
	assert(encodedMessage.size());

	Message *compareMessage = Message::parse(encodedMessage);
	assert(compareMessage->getType() == Message::UpdateReqType);

	// check that messages are equal
	assert(*compareMessage == *update);
	resip::Data secondMessage = compareMessage->encodePayload();
	
	// check that two similar messages encode the same
	assert(secondMessage == encodedMessage);

	// Test message type on a response
	UpdateAns *updateAns = update->makeUpdateResponse(noData);
	assert(updateAns->getType() == Message::UpdateAnsType);

	// Check transaction ID
	assert(updateAns->getTransactionId() == update->getTransactionId());
}

void
TestChordUpdate()
{
	DebugLog(<< "Testing ChordUpdate");

	NodeId node;
	DestinationId dest(node);

	ChordUpdate update;
	update.setUpdateType(ChordUpdate::PeerReady);
	resip::Data chordData = update.encode();
	assert(chordData.size());

	UpdateReq *updateReq = new UpdateReq(dest, chordData);
	resip::Data encodedMessage = updateReq->encodePayload();

	Message *compareMessage = Message::parse(encodedMessage);
	assert(*compareMessage == *updateReq);

	resip::Data reqBlob = compareMessage->getRequestMessageBody();
	assert(reqBlob.size());

	ChordUpdate update2(reqBlob);
	assert(update2 == update);

	UpdateAns *updateAns = updateReq->makeUpdateResponse(update.encode());
	assert(updateAns);

	assert(updateAns->getTransactionId() == updateReq->getTransactionId());

	assert(updateAns->getType() == Message::UpdateAnsType);
}

void
TestJoin()
{
	DebugLog(<< "Testing Join");

	NodeId node;
	DestinationId dest(node);

	JoinReq *joinReq = new JoinReq(dest, node);
	assert(joinReq->getType() == Message::JoinReqType);
	resip::Data encodedData = joinReq->encodePayload();

	Message *msg = Message::parse(encodedData);
	assert(msg);
	assert(msg->getType() == Message::JoinReqType);

	JoinReq *joinReqMsg = static_cast<JoinReq *>(msg);
	assert(joinReqMsg->getNodeId() == node);

	ResourceId rid(Random::getRandom(16));
	NodeId nid(rid);

	JoinReq *newReq = new JoinReq(dest, nid);
	assert(newReq->getNodeId() == nid);
	Message *testMessage = Message::parse(newReq->encodePayload());
	assert(testMessage->getType() == Message::JoinReqType);

	JoinReq *joinReq2 = static_cast<JoinReq *>(testMessage);
	assert(joinReq2->getNodeId() == nid);

	JoinAns *ans = joinReq2->makeJoinResponse();
	assert(ans->getType() == Message::JoinAnsType);
	assert(!ans->isRequest());

	resip::Data encodedAnswer = ans->encodePayload();
	Message *testMsg = Message::parse(encodedAnswer);
	assert(testMsg);
	assert(testMsg->getType() == Message::JoinAnsType);
}

void
TestMessages()
{
	TestJoin();
	TestUpdate();
	TestChordUpdate();
	TestConnect();
}

int main() 
{
	resip::Log::initialize(resip::Data("cerr"),
	resip::Data("DEBUG"),resip::Data("ParsingTest"));

#if 0
      
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

#endif

	TestMessages();

//	Data d;
//	DataStream ds(d);
//	hdr.encode(ds);
//	Data foo = Data::from(hdr);
	return 0;
}
