#if !defined DumEvent_hxx
#define DumEvent_hxx

#include "resip/stack/SipMessage.hxx"
#include "tfm/Event.hxx"
#include "resip/dum/InviteSession.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/Handles.hxx"

#include <memory>

class DumUserAgent;

class DumEvent : public Event
{
   public:
      DumEvent(DumUserAgent* dua);
      DumEvent(DumUserAgent* dua, const resip::SipMessage* msg);
      DumEvent(DumUserAgent* dua, const resip::SipMessage& msg);
      
      bool hasMessage() const noexcept { return mMessage.get() != 0; }
      std::shared_ptr<resip::SipMessage> getMessage() const noexcept { return mMessage; }

      // The id of the DUM usage (invite session, subscription, registration,
      // ...) the event is about, if it is about one.  DUM gives each usage an
      // id of its own, never reused.
      virtual bool getUsageId(resip::Handled::Id& /*id*/) const { return false; }
   private:
      std::shared_ptr<resip::SipMessage> mMessage;
};


#endif
