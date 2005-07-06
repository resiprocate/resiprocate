#include "resiprocate/dum/RedirectManager.hxx"
#include "resiprocate/NameAddr.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/RedirectHandler.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DialogSet.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

bool
RedirectManager::handle(DialogSet& dSet, SipMessage& origRequest, const SipMessage& response)
{
   assert( response.isResponse() );
   assert( origRequest.isRequest() );

   RedirectHandler* handler = dSet.mDum.getRedirectHandler();   

   int code = response.header(h_StatusLine).statusCode();
   DialogSetId id(origRequest);   
   RedirectedRequestMap::iterator it = mRedirectedRequestMap.find(id);

   //380, 305 fall through to the application
   if (code < 300 || code == 380 || code == 305)
   {
      return false;
   }
   else if (code >= 300 && code < 400)
   {
      if (it == mRedirectedRequestMap.end())
      {
         DebugLog( << "RedirectManager::handle: new TargetSet: " << id);         
         mRedirectedRequestMap[id] = new TargetSet(origRequest, mOrdering);
         it = mRedirectedRequestMap.find(id);         
      }
      if (handler)
      {
         handler->onRedirectReceived(dSet.mAppDialogSet->getHandle(), response);
      }
      TargetSet& tSet = *it->second;      
      tSet.addTargets(response);

      while(tSet.makeNextRequest(origRequest))
      {
         if (handler)
         {
            if (handler->onTryingNextTarget(dSet.mAppDialogSet->getHandle(), origRequest))
            {
               return true;
            }
         }
         else //!dcm! -- accept all if no handler--should a handler be required?
         {
            return true;
         }
      }
      delete it->second;         
      mRedirectedRequestMap.erase(it);
      return false;
   }
   //5xx, 6xx different?
   return false;   
}

void 
RedirectManager::setOrdering(const Ordering& order)
{
   mOrdering = order;   
}

void
RedirectManager::TargetSet::addTargets(const SipMessage& msg)
{
   if (msg.exists(h_Contacts))
   {         
      for (NameAddrs::const_iterator it = msg.header(h_Contacts).begin(); it != msg.header(h_Contacts).end(); it++)
      {         
         if (mTargetSet.find(*it) == mTargetSet.end())
         {
            DebugLog( << "RedirectManager::TargetSet::addTargets:target: " << *it);
            mTargetSet.insert(*it);
            mTargetQueue.push(*it);
         }                     
      }
   }   
}

bool
RedirectManager::TargetSet::makeNextRequest(SipMessage& request)
{
   request = mRequest;
   //dispaly name, check if it's an invite, recurse if it isn't, throw if
   //exhausted? Or make a boolean, elimnate hasTarget   
   while(!mTargetQueue.empty())
   {
      try
      {
         request.mergeUri(mTargetQueue.top().uri());
         mTargetQueue.pop();
         if (request.isRequest() && request.header(h_RequestLine).method() == INVITE)
         {
            DebugLog(<< "RedirectManager::TargetSet::makeNextRequest: " << request);
            request.header(h_CSeq).sequence()++;
            return true;
         }
      }
      catch(BaseException&)
      {
         DebugLog(<< "RedirectManager::TargetSet::makeNextRequest: error generating request");
         mTargetQueue.pop();
      }
   }
   return false;
}

bool RedirectManager::Ordering::operator()(const NameAddr& lhs, const NameAddr& rhs) const
{
   if (lhs.uri().exists(p_q))
   {
      if (rhs.uri().exists(p_q))
      {
         return lhs.uri().param(p_q) < rhs.uri().param(p_q);
      }
      else
      {
         return true;
      }
   }
   else if (rhs.uri().exists(p_q))
   {
      return false;
   }
   else
   {
      return true;
   }
}

void RedirectManager::removeDialogSet(DialogSetId id)
{
   RedirectedRequestMap::iterator it = mRedirectedRequestMap.find(id);
   
   if (it != mRedirectedRequestMap.end())
   {
      delete it->second;         
      mRedirectedRequestMap.erase(it);
   }
}

         
         
      
         
         
