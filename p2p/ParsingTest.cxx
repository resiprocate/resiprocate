#include "rutil/Data.hxx"
#include "p2p/MessageStructsGen.hxx"

#include <iostream>

using namespace s2c;
using namespace resip;

int main() 
{
	std::cout << "ctor" << std::endl;
	ForwardingLayerMessageStruct hdr;
	std::cout << "done" << std::endl;

//	hdr.print(std::cout, 2);

//	Data d;
//	DataStream ds(d);
//	hdr.encode(ds);
//	Data foo = Data::from(hdr);
	return 0;
}
