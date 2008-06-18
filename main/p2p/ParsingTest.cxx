#include "rutil/Data.hxx"
#include "p2p/Join.hxx"
#include "p2p/MessageStructsGen.hxx"
#include "rutil/DataStream.hxx"

#include <iostream>

using namespace s2c;
using namespace resip;

int main() 
{
	std::cout << "ctor" << std::endl;
	ForwardingLayerMessageStruct hdr;
	std::cout << "done" << std::endl;

//	hdr.print(std::cout, 2);

	resip::Data d;
	p2p::NodeId n;
	p2p::Message *m = new p2p::JoinReq(n,d);
	m->setOverlayName("duane");
	resip::Data encodedMessage = m->encodePayload();

	//std::cout << 

//	Data d;
//	DataStream ds(d);
//	hdr.encode(ds);
//	Data foo = Data::from(hdr);
	return 0;
}
