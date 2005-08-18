#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/IdentityHandler.hxx"
#include "resiprocate/external/HttpProvider.hxx"
#include "resiprocate/external/HttpGetMessage.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

IdentityHandler::IdentityHandler(DialogUsageManager& dum, TargetCommand::Target& target)
   : DumFeature(dum, target)
{
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
                                          mDum);
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
   RequiresCerts::iterator it = mRequiresCerts.find(msg.tid());
   if (it != mRequiresCerts.end())
   {
      mDum.getSecurity()->checkAndSetIdentity( *it->second, msg.getBodyData() );
      mRequiresCerts.erase(it);
      postCommand(auto_ptr<Message>(it->second));
   }
#endif
}
