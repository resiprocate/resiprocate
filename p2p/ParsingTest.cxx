#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Data.hxx"
#include "p2p/Join.hxx"
#include "p2p/MessageStructsGen.hxx"
#include "rutil/DataStream.hxx"

#include <iostream>

using namespace s2c;
using namespace resip;

int main() 
{
        resip::Log::initialize(resip::Data("cerr"),
        resip::Data("DEBUG"),resip::Data("ParsingTest"));
      
	std::cout << "ctor" << std::endl;
	ForwardingLayerMessageStruct hdr;
	std::cout << "done" << std::endl;

	resip::Data d;
	p2p::NodeId n;
	p2p::Message *m = new p2p::JoinReq(n,d);
	m->setOverlayName("duane");
	resip::Data encodedMessage = m->encodePayload();

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
