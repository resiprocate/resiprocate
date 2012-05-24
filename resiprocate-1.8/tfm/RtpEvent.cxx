#include "tfm/RtpEvent.hxx"
#include "tfm/TestRtp.hxx"

RtpEvent::RtpEvent(TestRtp *tua, Type event) :
   Event(tua),
   mType(event)
{
}
