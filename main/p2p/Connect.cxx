#include "p2p/Connect.hxx"

using namespace p2p;

ConnectReq::ConnectReq()
{
}

ConnectReq::ConnectReq(const resip::Data &frag, const resip::Data &password, UInt16 port, const resip::Data &role, const std::vector<resip::Data> &candidates)
{

}

void 
ConnectReq::decodePayload(resip::DataStream &dataStream)
{
	decode(dataStream);
}

ConnectAns::ConnectAns()
{

}

void 
ConnectAns::decodePayload(resip::DataStream &dataStream)
{
	decode(dataStream);
}

