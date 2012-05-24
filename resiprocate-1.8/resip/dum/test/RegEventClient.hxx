#if !defined(RESIP_RegEventClient_hxx)
#define RESIP_RegEventClient_hxx

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumThread.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "rutil/SharedPtr.hxx"

namespace resip
{
class NameAddr;
class SipMessage;
class Security;
class MasterProfile;
}

class RegEventClient  : public resip::ClientSubscriptionHandler,
                        public resip::ClientRegistrationHandler
{
   public:
      RegEventClient(resip::SharedPtr<resip::MasterProfile> profile);
      virtual ~RegEventClient();
      
      void run();

      void watchAor(const resip::Uri& aor);
      //void unwatchAor(const resip::Uri& aor);

      virtual void onRegEvent(const resip::Data& aor, const resip::Data& reg)=0;
      virtual void onRegEventError(const resip::Data& aor, const resip::Data& reg)=0;
      
   protected:
      virtual void onUpdatePending(resip::ClientSubscriptionHandle, 
                                   const resip::SipMessage& notify, 
                                   bool outOfOrder);
      virtual void onUpdateActive(resip::ClientSubscriptionHandle, 
                                  const resip::SipMessage& notify, 
                                  bool outOfOrder);
      virtual void onUpdateExtension(resip::ClientSubscriptionHandle,
                                     const resip::SipMessage& notify, 
                                     bool outOfOrder);
      virtual int onRequestRetry(resip::ClientSubscriptionHandle, 
                                 int retrySeconds, 
                                 const resip::SipMessage& notify);
      virtual void onTerminated(resip::ClientSubscriptionHandle, 
                                const resip::SipMessage* msg);   
      virtual void onNewSubscription(resip::ClientSubscriptionHandle, 
                                     const resip::SipMessage& notify);


      virtual void onSuccess(resip::ClientRegistrationHandle, 
                             const resip::SipMessage& response)
      {
      }
      
      virtual void onRemoved(resip::ClientRegistrationHandle, 
                             const resip::SipMessage& response)
      {
      }
      
      virtual int onRequestRetry(resip::ClientRegistrationHandle, 
                                 int retrySeconds, 
                                 const resip::SipMessage& response)
      {
         return -1;
      }
      
      virtual void onFailure(resip::ClientRegistrationHandle, 
                             const resip::SipMessage& response)
      {
      }
      

   protected:
      resip::Security* mSecurity;
      resip::SipStack mStack;
      resip::StackThread mStackThread;
      resip::DialogUsageManager mDum;
      resip::DumThread mDumThread;
      
      resip::SharedPtr<resip::MasterProfile> mProfile;
      friend class AddAor;
};

class AddAor : public resip::DumCommand
{
   public:
      AddAor(RegEventClient& client, const resip::Uri& aor);
      virtual void executeCommand();

      virtual resip::Message* clone() const;
#ifdef RESIP_USE_STL_STREAMS
      virtual std::ostream& encode(std::ostream&) const;
      virtual std::ostream& encodeBrief(std::ostream&) const;
#else
      virtual resip::ResipFastOStream& encode(resip::ResipFastOStream&) const;
      virtual resip::ResipFastOStream& encodeBrief(resip::ResipFastOStream&) const;
#endif
      
   private:
      RegEventClient& mClient;
      const resip::Uri mAor;
};

#endif
