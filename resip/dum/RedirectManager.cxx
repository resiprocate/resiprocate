#include "resip/dum/RedirectManager.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Logger.hxx"
#include "resip/dum/RedirectHandler.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

bool
RedirectManager::handle(DialogSet& dSet, SipMessage& origRequest, const SipMessage& response)
{
   resip_assert( response.isResponse() );
   resip_assert( origRequest.isRequest() );

   //380, 305 fall through to the application
   int code = response.header(h_StatusLine).statusCode();
   if (code < 300 || code == 380 || code == 305)
   {
      return false;
   }
   else if (code >= 300 && code < 400)
   {
      RedirectHandler* handler = dSet.mDum.getRedirectHandler();   
      DialogSetId id(origRequest);   
      RedirectedRequestMap::iterator it = mRedirectedRequestMap.find(id);

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
         if (request.isRequest())
         {
            switch(request.header(h_RequestLine).method())
            {
               case ACK:
               case BYE:
               case CANCEL:
               case PRACK:
                  break;
               default:
                  DebugLog(<< "RedirectManager::TargetSet::makeNextRequest: " << request);
                  request.header(h_CSeq).sequence()++;
                  return true;
            }
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
   if (lhs.exists(p_q))
   {
      if (rhs.exists(p_q))
      {
         return lhs.param(p_q) < rhs.param(p_q);
      }
      else
      {
         return lhs.param(p_q) < 1000;  // 1.0
      }
   }
   else
   {
      // Note: if lhs is not present and treated as 1.0, it will never to less than rhs
      return false;
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

         
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
