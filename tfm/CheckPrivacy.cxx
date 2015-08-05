#include "CheckPrivacy.hxx"
#include "tfm/SipEvent.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;

bool 
CheckPrivacy::operator()(boost::shared_ptr<Event> event) const
{
   SipEvent* msgEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(msgEvent);
   boost::shared_ptr<resip::SipMessage> msg = msgEvent->getMessage();
   resip_assert(msg.get());
   
   DebugLog (<< "Checking for privacy: " << *msg);

   if (msg->header(h_From).uri().user() != "anonymous" 
       || msg->header(h_From).uri().host() != "anonymous.invalid")
   {
      return false;
   }
   
   if (msg->exists(h_ReplyTo)
       || msg->exists(h_UserAgent)
       || msg->exists(h_Organization)
       || msg->exists(h_Server)
       || msg->exists(h_Subject)
       || msg->exists(h_InReplyTo)
       || msg->exists(h_CallInfos)
       || msg->exists(h_Warnings))
   {
      return false;
   }
   else
   {
      return true;
   }
}

