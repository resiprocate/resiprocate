#include "p2p/Leave.hxx"

using namespace p2p;

LeaveAns::LeaveAns(LeaveReq *request)
{
    copyForwardingData(*request);
}

void 
LeaveAns::getEncodedPayload(resip::DataStream &data) const 
{
	assert(0);
}

void 
LeaveReq::getEncodedPayload(resip::DataStream &data) const
{
	assert(0);
}

LeaveReq::LeaveReq(NodeId node) :
	mNode(node)
{

}


