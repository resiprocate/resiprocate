#include <cassert>

#include "resiprocate/dum/ServerAuthManager.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/UserAuthInfo.hxx"
#include "resiprocate/Helper.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ServerAuthManager::ServerAuthManager(DialogUsageManager& dum) :
   mDum(dum)
{
}


ServerAuthManager::~ServerAuthManager()
{
}

bool 
ServerAuthManager::handleUserAuthInfo(std::auto_ptr<Message>& msg)
{
   InfoLog( << "Checking for auth result" );

   std::auto_ptr<UserAuthInfo> userAuth(dynamic_cast<UserAuthInfo*>(msg.get()));
   
   if (!userAuth.get())
   {
      return false;
   }

   MessageMap::iterator it = mMessages.find(userAuth->getTransactionId());
   assert(it != mMessages.end());
   SipMessage* requestWithAuth = it->second;
   mMessages.erase(it);
   if (userAuth->getA1().empty())
   {
      InfoLog (<< "Account does not exist " << userAuth->getUser() << " in " << userAuth->getRealm());
      SipMessage response;
      Helper::makeResponse(*requestWithAuth, 404, "Account does not exist.");
      mDum.send(response);
      delete requestWithAuth;
      return true;
   }
   else
   {
      //!dcm! -- need to handle stale/unit test advancedAuthenticateRequest
      //!dcm! -- delta? deal with.
      std::pair<Helper::AuthResult,Data> resPair = 
         Helper::advancedAuthenticateRequest(*requestWithAuth, 
                                             userAuth->getRealm(),
                                             userAuth->getA1(),
                                             3000);
      
      if (resPair.first == Helper::Authenticated)
      {
         InfoLog (<< "Retrieved stored message with challenge and passed on to dum");
         msg = std::auto_ptr<Message>(requestWithAuth);
         return false;
      }
      else
      {
         InfoLog (<< "Invalid password provided " << userAuth->getUser() << " in " << userAuth->getRealm());

         SipMessage response;
         Helper::makeResponse(*requestWithAuth, 403, "Invalid password provided");
         mDum.send(response);
         delete requestWithAuth;
         return true;
      }
   }
}
      
// return true if request has been consumed 
bool 
ServerAuthManager::handle(std::auto_ptr<Message>& msg)
{
   //InfoLog( << "trying to do auth" );

   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg.get());
   assert(sipMsg);
   
   if (sipMsg->isResponse())
   {
      return false;
   }

   if (!sipMsg->exists(h_ProxyAuthorizations))
   {
      //assume TransactionUser has matched/repaired a realm
      SipMessage* challenge = 
         Helper::makeProxyChallenge(*sipMsg, 
                                    sipMsg->header(h_RequestLine).uri().host(),
                                    true,
                                    false);
      mDum.send(*challenge);
      delete challenge;
      return true;
   }
 
   try
   {
      for(Auths::iterator it = sipMsg->header(h_ProxyAuthorizations).begin();
          it  != sipMsg->header(h_ProxyAuthorizations).end(); it++)
      {
         if (mDum.isMyDomain(it->param(p_realm)))
         {
            requestCredential(it->param(p_username),
                              it->param(p_realm), 
                              sipMsg->getTransactionId());
            mMessages[sipMsg->getTransactionId()] = sipMsg;
            msg.release();
            return true;
         }
      }
      return false;
   }
   catch(BaseException& e)
   {
      InfoLog (<< "Invalid auth header provided " << e);
      SipMessage response;
      Helper::makeResponse(*sipMsg, 400, "Invalid auth header");
      mDum.send(response);
      return true;
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
