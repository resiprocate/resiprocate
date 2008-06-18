#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Data.hxx"
#include "p2p/Join.hxx"
#include "p2p/Connect.hxx"
#include "p2p/Update.hxx"
#include "p2p/MessageStructsGen.hxx"
#include "rutil/DataStream.hxx"

#include "p2p/P2PSubsystem.hxx"
#define RESIPROCATE_SUBSYSTEM P2PSubsystem::P2P

#include <iostream>

using namespace s2c;
using namespace resip;
using namespace p2p;

void
TestUpdate()
{
	resip::Data noData;
	UpdateReq *update = new UpdateReq(noData);
	resip::Data encodedMessage = update->encodePayload();

}

void
TestChordUpdate()
{

}

int main() 
{
	resip::Log::initialize(resip::Data("cerr"),
	resip::Data("DEBUG"),resip::Data("ParsingTest"));
      
	std::cout << "ctor" << std::endl;
	ForwardingLayerMessageStruct hdr;
	std::cout << "done" << std::endl;

	resip::Data d;
	std::vector<resip::Data> v;
	Message *m2 = new ConnectReq(d, d, 0, d, v);

	p2p::NodeId n;
	p2p::Message *m = new p2p::JoinReq(n,d);
	m->setOverlayName("duane");
	resip::Data encodedMessage = m->encodePayload();

	DebugLog(<< "attempting to parse");

	Message *newMessage = Message::parse(encodedMessage);

	std::cout << encodedMessage.hex() << std::endl;

	ForwardingLayerMessageStruct hdr2;
	resip::DataStream is(encodedMessage);
	hdr2.decode(is);

//	Data d;
//	DataStream ds(d);
//	hdr.encode(ds);
//	Data foo = Data::from(hdr);
	return 0;
}
