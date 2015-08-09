#include "p2p/Connect.hxx"
#include "p2p/MessageHelper.hxx"

using namespace p2p;
using namespace s2c;

ConnectReq::ConnectReq()
{
}

ConnectReq::ConnectReq(const DestinationId &dest, 
                       const resip::Data &frag, 
                       const resip::Data &password, 
                       UInt16 application, 
                       const resip::Data &role, 
                       const std::vector<Candidate> &candidates) :
	ConnectBase(frag, password, application, role, candidates)
{
	pushDestinationId(dest);
}

ConnectAns::ConnectAns()
{
    ;
}

ConnectAns::ConnectAns(Message *req, 
                       const resip::Data &frag, 
                       const resip::Data &password, 
                       UInt16 application, 
                       const resip::Data &role, 
                       const std::vector<Candidate> &candidates) :
	ConnectBase(frag, password, application, role, candidates)
{
	resip_assert(req);
	copyForwardingData(*req);
}
