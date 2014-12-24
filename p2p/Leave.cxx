#include "p2p/Leave.hxx"

using namespace p2p;

LeaveAns::LeaveAns()
{
}

LeaveAns::LeaveAns(LeaveReq *request)
{
    copyForwardingData(*request);
}

void 
LeaveAns::getEncodedPayload(resip::DataStream &data) 
{
	resip_assert(0);
}

void
LeaveAns::decodePayload(resip::DataStream &dataStream)
{

}

LeaveReq::LeaveReq()
{
}

void 
LeaveReq::getEncodedPayload(resip::DataStream &data) 
{
	resip_assert(0);
}

LeaveReq::LeaveReq(const DestinationId &dest, NodeId node) :
	mNode(node)
{

}

void
LeaveReq::decodePayload(resip::DataStream &dataStream)
{

}


