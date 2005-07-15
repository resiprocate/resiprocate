#include "IdentityHandler.hxx"
#include "resiprocate/external/HttpProvider.hxx"
#include "resiprocate/external/HttpGetMessage.hxx"

ProcessingResult 
IdentityHandler::process(Message* msg)
{
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
   if (sipMsg)
   {
      if (queueForIdentityCheck)
      {
         return EventTaken;
      }
      else
      {
         return FeatureDone;
      }
   }
   
   HttpGetMessage* httpMsg = dynamic_cast<HttpGetMessage*>(msg.get());
   if (httpMsg)
   {
      processIdentityCheckResponse(*httpMsg);         
      return FeatureDoneAndEventDone;         
   }

   return FeatureDone;   
}

bool
DialogUsageManager::queueForIdentityCheck(SipMessage* sipMsg)
{
#if defined(USE_SSL)
   if (sipMsg->exists(h_Identity) &&
       sipMsg->exists(h_IdentityInfo) &&
       sipMsg->exists(h_Date))
   {
      if (getSecurity()->hasDomainCert(sipMsg->header(h_From).uri().host()))
      {
         getSecurity()->checkAndSetIdentity(*sipMsg);
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
                                          *this);
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
DialogUsageManager::processIdentityCheckResponse(const HttpGetMessage& msg)
{
#if defined(USE_SSL)
   InfoLog(<< "DialogUsageManager::processIdentityCheckResponse: " << msg.brief());   
   RequiresCerts::iterator it = mRequiresCerts.find(msg.tid());
   if (it != mRequiresCerts.end())
   {
      getSecurity()->checkAndSetIdentity( *it->second, msg.getBodyData() );
      mRequiresCerts.erase(it);
      mDum.post(it->second);
   }
#endif
}
