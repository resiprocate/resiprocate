#include "resip/dum/ContactInstanceRecord.hxx"
#include "rutil/Timer.hxx"
#include "resip/stack/SipMessage.hxx"

using namespace resip;

ContactInstanceRecord::ContactInstanceRecord() : 
   mRegExpires(0),
   mLastUpdated(Timer::getTimeSecs()),
   mClientFlowId(0)
{
}

ContactInstanceRecord 
ContactInstanceRecord::makeRemoveDelta(const NameAddr& contact)
{
   ContactInstanceRecord c;
   c.mContact = contact;
   return c;
}

ContactInstanceRecord 
ContactInstanceRecord::makeUpdateDelta(const NameAddr& contact, 
                                       UInt64 expires,  // absolute time in secs
                                       const SipMessage& msg)
{
   ContactInstanceRecord c;
   c.mContact = contact;
   c.mRegExpires = expires;
   c.mReceivedFrom = msg.getSource();
   if (msg.exists(h_Paths))
   {
      c.mSipPath = msg.header(h_Paths);
   }
   if (contact.exists(p_Instance))
   {
      c.mInstance = contact.param(p_Instance);
   }
   if (contact.exists(p_FlowId))
   {
      c.mClientFlowId = contact.param(p_FlowId);
   }
   // !jf! need to fill in mServerSessionId here
   return c;
  
}

      
