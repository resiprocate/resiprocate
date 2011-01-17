#include "tfm/tfdum/DumEvent.hxx"
#include "tfm/tfdum/DumUserAgent.hxx"

using namespace resip;

DumEvent::DumEvent(DumUserAgent* dua) : 
   Event(dua)
{}

DumEvent::DumEvent(DumUserAgent* dua, const SipMessage& msg) :
   Event(dua),
   mMessage(dynamic_cast<SipMessage*>(msg.clone()))
{
}

DumEvent::DumEvent(DumUserAgent* dua, const SipMessage* msg) :
Event(dua),
mMessage(msg ? dynamic_cast<SipMessage*>(msg->clone()) : NULL)
{
}
