#include "EndPoint.hxx"

#include "rutil/Logger.hxx"
#include "rutil/Data.hxx"

#include "tfm/SequenceSet.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

EndPoint::~EndPoint()
{
}

void
EndPoint::clean()
{
}

void
EndPoint::handleEvent(Event* eventRaw)
{
   boost::shared_ptr<Event> event(eventRaw);
   DebugLog(<< "EndPoint::handleEvent: " << *eventRaw);
   boost::shared_ptr<SequenceSet> sset= getSequenceSet().lock();   
   if (sset)
   {
      sset->enqueue(event);
   }
   else
   {
      WarningLog(<< *this << " has no associated SequenceSet: discarding event " << *event);
   }
}
