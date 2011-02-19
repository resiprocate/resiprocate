#include "resip/dum/ContactInstanceRecord.hxx"
#include "rutil/Timer.hxx"
#include "resip/stack/SipMessage.hxx"

using namespace resip;

ContactInstanceRecord::ContactInstanceRecord() : 
   mRegExpires(0),
   mLastUpdated(Timer::getTimeSecs()),
   mRegId(0),
   mSyncContact(false),
   mUserInfo(0)
{
}

bool
ContactInstanceRecord::operator==(const ContactInstanceRecord& rhs) const
{
   if(!mInstance.empty() || !rhs.mInstance.empty())
   {
      // If instanceId is specified on any of the instance records, then it must match
      if(mInstance == rhs.mInstance)
      {
         if(mRegId != 0 || rhs.mRegId != 0)
         {
            // If regId is specified on either, then make sure it matches
            return mRegId == rhs.mRegId;
         }
         return true;  // InstanceId matches and no regId provided
      }
      return false;  // instance doesn't match
   }
   else
   {
      // No InstanceId on either, check if contactUri matches
      return mContact.uri() == rhs.mContact.uri();
   }
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
   if (contact.exists(p_regid))
   {
      c.mRegId = contact.param(p_regid);
   }
   // !jf! need to fill in mServerSessionId here
   return c;
  
}

      
