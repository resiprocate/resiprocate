
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Logger.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/IdentityHandler.hxx"
#include "resip/dum/HttpProvider.hxx"
#include "resip/dum/HttpGetMessage.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

IdentityHandler::IdentityHandler(DialogUsageManager& dum, TargetCommand::Target& target)
   : DumFeature(dum, target)
{
}

IdentityHandler::~IdentityHandler()
{
   for (RequiresCerts::iterator it = mRequiresCerts.begin(); it != mRequiresCerts.end(); ++it)
   {
      delete it->second;
   }
}

DumFeature::ProcessingResult 
IdentityHandler::process(Message* msg)
{
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
   if (sipMsg)
   {
      if (queueForIdentityCheck(sipMsg))
      {
         return EventTaken;
      }
      else
      {
         return FeatureDone;
      }
   }
   
   HttpGetMessage* httpMsg = dynamic_cast<HttpGetMessage*>(msg);
   if (httpMsg)
   {
      processIdentityCheckResponse(*httpMsg);         
      return FeatureDoneAndEventDone;         
   }

   return FeatureDone;   
}

bool
IdentityHandler::queueForIdentityCheck(SipMessage* sipMsg)
{
#if defined(USE_SSL)
   if (sipMsg->exists(h_Identity) &&
       sipMsg->exists(h_IdentityInfo) &&
       sipMsg->exists(h_Date))
   {
      if (mDum.getSecurity()->hasDomainCert(sipMsg->header(h_From).uri().host()))
      {
         mDum.getSecurity()->checkAndSetIdentity(*sipMsg);
         return false;
      }
      else
      {
         if (!HttpProvider::instance())
         {
            return false;
         }
         
         try
         {
            mRequiresCerts[sipMsg->getTransactionId()] = sipMsg;
            InfoLog( << "Dum::queueForIdentityCheck, sending http request to: " 
                     << sipMsg->header(h_IdentityInfo));
            
            HttpProvider::instance()->get(sipMsg->header(h_IdentityInfo), 
                                          sipMsg->getTransactionId(),
                                          mDum,
                                          mDum.dumIncomingTarget());
            return true;
         }
         catch (BaseException&)
         {
         }
      }
   }
#endif

   std::auto_ptr<SecurityAttributes> sec(new SecurityAttributes);
   sec->setIdentity(sipMsg->header(h_From).uri().getAor());
   sec->setIdentityStrength(SecurityAttributes::From);
   sipMsg->setSecurityAttributes(sec);
   return false;
}

void
IdentityHandler::processIdentityCheckResponse(const HttpGetMessage& msg)
{
#if defined(USE_SSL)
   InfoLog(<< "DialogUsageManager::processIdentityCheckResponse: " << msg.brief());   
   RequiresCerts::iterator it = mRequiresCerts.find(msg.getTransactionId());
   if (it != mRequiresCerts.end())
   {
      mDum.getSecurity()->checkAndSetIdentity( *it->second, msg.getBodyData() );
      postCommand(auto_ptr<Message>(it->second));
      mRequiresCerts.erase(it);
   }
#endif
}
