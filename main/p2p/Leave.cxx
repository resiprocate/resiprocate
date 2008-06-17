#include "p2p/Leave.hxx"

using namespace p2p;

void 
LeaveAns::getPayload(resip::Data &data) const 
{
	assert(0);
}

Message *
LeaveAns::makeErrorResponse(ErrorResponseCode code, const resip::Data& reason) const
{
	assert(0);
	return 0;
}

void 
LeaveReq::getPayload(resip::Data &data) const
{
	assert(0);
}

Message *
LeaveReq::makeErrorResponse(ErrorResponseCode code, const resip::Data& reason) const
{
	assert(0);
	return 0;
}

LeaveReq::LeaveReq(NodeId node) :
	mNode(node)
{

}


