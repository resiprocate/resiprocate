#include "tfcommon/RtpEvent.hxx"
#include "tfcommon/TestRtp.hxx"

RtpEvent::RtpEvent(TestRtp *tua, Type event) :
   Event(tua),
   mType(event)
{
}
