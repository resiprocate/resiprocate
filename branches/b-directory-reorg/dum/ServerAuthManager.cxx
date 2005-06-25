#include <cassert>

#include "dum/ServerAuthManager.hxx"
#include "dum/DialogUsageManager.hxx"
#include "rutil/Logger.hxx"
#include "dum/UserAuthInfo.hxx"
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

SipMessage*
ServerAuthManager::handleUserAuthInfo(UserAuthInfo* userAuth)
{
   assert(userAuth);

   MessageMap::iterator it = mMessages.find(userAuth->getTransactionId());
   assert(it != mMessages.end());
   SipMessage* requestWithAuth = it->second;
   mMessages.erase(it);

   InfoLog( << "Checking for auth result in realm=" << userAuth->getRealm() 
            << " A1=" << userAuth->getA1());
         
   if (userAuth->getA1().empty())
   {
      InfoLog (<< "Account does not exist " << userAuth->getUser() << " in " << userAuth->getRealm());
      SipMessage response;
      Helper::makeResponse(response, *requestWithAuth, 404, "Account does not exist.");
      mDum.send(response);
      delete requestWithAuth;
      return 0;
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

      SipMessage response;
      SipMessage* challenge;
      switch (resPair.first) 
      {
         case Helper::Authenticated :
            if (authorizedForThisIdentity(userAuth->getUser(), userAuth->getRealm(), 
                                          requestWithAuth->header(h_From).uri()))
            {
               InfoLog (<< "Authorized request for " << userAuth->getRealm());
               return requestWithAuth;
            }
            else
            {
               // !rwm! The user is trying to forge a request.  Respond with a 403
               InfoLog (<< "User: " << userAuth->getUser() << " at realm: " << userAuth->getRealm() << 
                        " trying to forge request from: " << requestWithAuth->header(h_From).uri());

               Helper::makeResponse(response, *requestWithAuth, 403, "Invalid user name provided");
               mDum.send(response);
               delete requestWithAuth;
               return 0;
            }
            break;
         case Helper::Failed :
            InfoLog (<< "Invalid password provided " << userAuth->getUser() << " in " << userAuth->getRealm());
            InfoLog (<< "  a1 hash of password from db was " << userAuth->getA1() );

            Helper::makeResponse(response, *requestWithAuth, 403, "Invalid password provided");
            mDum.send(response);
            delete requestWithAuth;
            return 0;
            break;
         case Helper::BadlyFormed :
            InfoLog (<< "Authentication nonce badly formed for " << userAuth->getUser());

            Helper::makeResponse(response, *requestWithAuth, 403, "Invalid nonce");
            mDum.send(response);
            delete requestWithAuth;
            return 0;
            break;
         case Helper::Expired :
            InfoLog (<< "Nonce expired for " << userAuth->getUser());

            challenge = Helper::makeProxyChallenge(*requestWithAuth, 
                                                    requestWithAuth->header(h_RequestLine).uri().host(),
                                                    useAuthInt(),
                                                    true /*stale*/);
            InfoLog (<< "Sending challenge to " << requestWithAuth->brief());
            mDum.send(*challenge);
            delete challenge;
            delete requestWithAuth;
            return 0;
         default :
            break;
      }
   }
   return 0;
}

            
bool
ServerAuthManager::useAuthInt() const
{
   return false;
}


bool
ServerAuthManager::authorizedForThisIdentity(const resip::Data &user, 
                                               const resip::Data &realm, 
                                                resip::Uri &fromUri)
{
   // !rwm! good enough for now.  TODO eventually consult a database to see what
   // combinations of user/realm combos are authorized for an identity
   return ((fromUri.user() == user) && (fromUri.host() == realm));
}


// return true if request has been consumed 
ServerAuthManager::Result
ServerAuthManager::handle(const SipMessage& sipMsg)
{
   //InfoLog( << "trying to do auth" );
   if (sipMsg.isRequest())
   {
      if (!sipMsg.exists(h_ProxyAuthorizations))
      {
         //assume TransactionUser has matched/repaired a realm
         SipMessage* challenge = Helper::makeProxyChallenge(sipMsg, 
                                                            sipMsg.header(h_RequestLine).uri().host(),
                                                            useAuthInt(),
                                                            false /*stale*/);
         InfoLog (<< "Sending challenge to " << sipMsg.brief());
         mDum.send(*challenge);
         delete challenge;
         return Challenged;
      }
 
      try
      {
         for(Auths::const_iterator it = sipMsg.header(h_ProxyAuthorizations).begin();
             it  != sipMsg.header(h_ProxyAuthorizations).end(); it++)
         {
            if (mDum.isMyDomain(it->param(p_realm)))
            {
               InfoLog (<< "Requesting credential for " 
                        << it->param(p_username) << " @ " << it->param(p_realm));
               
               requestCredential(it->param(p_username),
                                 it->param(p_realm), 
                                 sipMsg.getTransactionId());
               mMessages[sipMsg.getTransactionId()] = static_cast<SipMessage*>(sipMsg.clone());
               return RequestedCredentials;
            }
         }

         InfoLog (<< "Didn't find matching realm ");
         SipMessage response;
         Helper::makeResponse(response, sipMsg, 404, "Account does not exist");
         mDum.send(response);
         return Rejected;
      }
      catch(BaseException& e)
      {
         InfoLog (<< "Invalid auth header provided " << e);
         SipMessage response;
         Helper::makeResponse(response, sipMsg, 400, "Invalid auth header");
         mDum.send(response);
         return Rejected;
      }
   }
   return Skipped;
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
