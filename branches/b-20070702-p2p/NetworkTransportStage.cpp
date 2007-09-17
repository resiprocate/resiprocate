#include "NetworkTransportStage.h"

namespace p2p
{

std::set<FlowId> NetworkTransportStage::mFlowIdSet;

NetworkTransportStage::NetworkTransportStage(AsyncStage * inAsyncStage, ForwardingStage * inForwardingStage)
{
	mAsyncStage = inAsyncStage;
	mForwardingStage = inForwardingStage;
}

NetworkTransportStage::~NetworkTransportStage()
{
}

}
