#include "EventInfo.h"

using namespace p2p;
using namespace std;

std::set<EventId> EventInfo::mEventIdSet;

EventInfo::EventInfo(GenericStage * inEventSource)
{
	mEventSource = inEventSource;
}

EventInfo::~EventInfo()
{
	p2p::EventInfo::mEventIdSet.erase(mEventId);
}