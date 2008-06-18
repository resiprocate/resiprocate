#include "p2p/Connect.hxx"
#include "p2p/MessageHelper.hxx"

using namespace p2p;
using namespace s2c;


ConnectReq::ConnectReq()
{
}


/// ***********
/// ConnectReq

ConnectReq::ConnectReq(const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates) :
	ConnectBase(frag, password, application, role, candidates)
{

}

ConnectAns::ConnectAns(Message *req, const resip::Data &frag, const resip::Data &password, UInt16 application, const resip::Data &role, const std::vector<resip::Data> &candidates) :
	ConnectBase(frag, password, application, role, candidates)
{
	assert(req);
	copyForwardingData(*req);
}

ConnectAns::ConnectAns()
{

}


