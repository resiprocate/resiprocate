#include "rutil/ResipAssert.h"

#include "resip/dum/ChallengeInfo.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/DumFeatureChain.hxx"
#include "resip/dum/ServerAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "rutil/Logger.hxx"
#include "resip/dum/UserAuthInfo.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

ServerAuthManager::ServerAuthManager(DialogUsageManager& dum, TargetCommand::Target& target, bool challengeThirdParties, const Data& staticRealm) :
   DumFeature(dum, target),
   mChallengeThirdParties(challengeThirdParties),
   mStaticRealm(staticRealm)
{
}


ServerAuthManager::~ServerAuthManager()
{
   InfoLog(<< "~ServerAuthManager:  " << mMessages.size() << " messages in memory when destroying.");
}

// !bwc! We absolutely, positively, MUST NOT throw here. This is because in
// DialogUsageManager::process(), we do not know if a DumFeature has taken
// ownership of msg until we get a return. If we throw, the ownership of msg
// is unknown. This is unacceptable.
DumFeature::ProcessingResult 
ServerAuthManager::process(Message* msg)
{
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);

   if (sipMsg)
   {
      //!dcm! -- unecessary happens in handle
      switch ( handle(sipMsg) )
      {
         case ServerAuthManager::Challenged:
            InfoLog(<< "ServerAuth challenged request " << sipMsg->brief());
            return DumFeature::ChainDoneAndEventDone;            
         case ServerAuthManager::RequestedInfo:
            InfoLog(<< "ServerAuth requested info (requiresChallenge) " << sipMsg->brief());
            return DumFeature::EventTaken;
         case ServerAuthManager::RequestedCredentials:
            InfoLog(<< "ServerAuth requested credentials " << sipMsg->brief());
            return DumFeature::EventTaken;
         case ServerAuthManager::Rejected:
            InfoLog(<< "ServerAuth rejected request " << sipMsg->brief());
            return DumFeature::ChainDoneAndEventDone;            
         default:   // includes Skipped
            return DumFeature::FeatureDone;            
      }
   }

   ChallengeInfo* challengeInfo = dynamic_cast<ChallengeInfo*>(msg);
   if(challengeInfo)
   {
      InfoLog(<< "ServerAuth got ChallengeInfo " << challengeInfo->brief());
      MessageMap::iterator it = mMessages.find(challengeInfo->getTransactionId());
      resip_assert(it != mMessages.end());
      std::auto_ptr<SipMessage> sipMsg(it->second);
      mMessages.erase(it);

      if(challengeInfo->isFailed()) 
      {
        // some kind of failure occurred while checking whether a 
        // challenge is required
        InfoLog(<< "ServerAuth requiresChallenge() async failed");
        SharedPtr<SipMessage> response(new SipMessage);
        Helper::makeResponse(*response, *sipMsg, 500, "Server Internal Error");
        mDum.send(response);
        return DumFeature::ChainDoneAndEventDone;
      }

      if(challengeInfo->isChallengeRequired()) 
      {
        issueChallenge(sipMsg.get());
        InfoLog(<< "ServerAuth challenged request (after async) " << sipMsg->brief());
        return DumFeature::ChainDoneAndEventDone;
      } 
      else 
      {
        // challenge is not required, re-instate original message
        postCommand(auto_ptr<Message>(sipMsg));
        return FeatureDoneAndEventDone;
      }
   }

   UserAuthInfo* userAuth = dynamic_cast<UserAuthInfo*>(msg);
   if (userAuth)
   {
      //InfoLog(<< "Got UserAuthInfo");
      UserAuthInfo* userAuth = dynamic_cast<UserAuthInfo*>(msg);
      if (userAuth)
      {
         Message* result = handleUserAuthInfo(userAuth);
         if (result)
         {
            postCommand(auto_ptr<Message>(result));
            return FeatureDoneAndEventDone;
         }
         else
         {
            InfoLog(<< "ServerAuth rejected request " << *userAuth);
            return ChainDoneAndEventDone;            
         }
      }
   }
   return FeatureDone;   
}

SipMessage*
ServerAuthManager::handleUserAuthInfo(UserAuthInfo* userAuth)
{
   resip_assert(userAuth);

   MessageMap::iterator it = mMessages.find(userAuth->getTransactionId());
   resip_assert(it != mMessages.end());
   SipMessage* requestWithAuth = it->second;
   mMessages.erase(it);

   InfoLog( << "Checking for auth result in realm=" << userAuth->getRealm() 
            << " A1=" << userAuth->getA1());

   if (userAuth->getMode() == UserAuthInfo::UserUnknown || 
       (userAuth->getMode() == UserAuthInfo::RetrievedA1 && userAuth->getA1().empty()))
   {
      InfoLog (<< "User unknown " << userAuth->getUser() << " in " << userAuth->getRealm());
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *requestWithAuth, 404, "User unknown.");
      mDum.send(response);
      onAuthFailure(BadCredentials, *requestWithAuth);
      delete requestWithAuth;
      return 0;
   }

   if (userAuth->getMode() == UserAuthInfo::Error)
   {
      InfoLog (<< "Error in auth procedure for " << userAuth->getUser() << " in " << userAuth->getRealm());
      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *requestWithAuth, 503, "Server Error.");
      mDum.send(response);
      onAuthFailure(Error, *requestWithAuth);
      delete requestWithAuth;
      return 0;
   }

   bool stale = false;
   bool digestAccepted = (userAuth->getMode() == UserAuthInfo::DigestAccepted);
   if(userAuth->getMode() == UserAuthInfo::RetrievedA1)
   {
      //!dcm! -- need to handle stale/unit test advancedAuthenticateRequest
      //!dcm! -- delta? deal with.
      std::pair<Helper::AuthResult,Data> resPair = 
         Helper::advancedAuthenticateRequest(*requestWithAuth, 
                                             userAuth->getRealm(),
                                             userAuth->getA1(),
                                             3000,
                                             proxyAuthenticationMode());

      switch (resPair.first) 
      {
         case Helper::Authenticated:
            digestAccepted = true;
            break;
         case Helper::Failed:
            // digestAccepted = false;   // already false by default
            break;
         case Helper::BadlyFormed:
            if(rejectBadNonces())
            {
               InfoLog (<< "Authentication nonce badly formed for " << userAuth->getUser());
   
               SharedPtr<SipMessage> response(new SipMessage);
               Helper::makeResponse(*response, *requestWithAuth, 403, "Invalid nonce");
               mDum.send(response);
               onAuthFailure(InvalidRequest, *requestWithAuth);
               delete requestWithAuth;
               return 0;
            }
            else
            {
               stale=true;
            }
            break;
         case Helper::Expired:
            stale = true;
            break;
         default:
            break;
      }
   }

   if(stale || userAuth->getMode() == UserAuthInfo::Stale) 
   {
      InfoLog (<< "Nonce expired for " << userAuth->getUser());

      issueChallenge(requestWithAuth);
      delete requestWithAuth;
      return 0;
   }

   if(digestAccepted)
   {
      if (authorizedForThisIdentity(userAuth->getUser(), userAuth->getRealm(),
                                    requestWithAuth->header(h_From).uri()))
      {
         InfoLog (<< "Authorized request for " << userAuth->getRealm());
         onAuthSuccess(*requestWithAuth);
         return requestWithAuth;
      }
      else
      {
         // !rwm! The user is trying to forge a request.  Respond with a 403
         InfoLog (<< "User: " << userAuth->getUser() << " at realm: " << userAuth->getRealm() <<
                  " trying to forge request from: " << requestWithAuth->header(h_From).uri());

         SharedPtr<SipMessage> response(new SipMessage);
         Helper::makeResponse(*response, *requestWithAuth, 403, "Invalid user name provided");
         mDum.send(response);
         onAuthFailure(InvalidRequest, *requestWithAuth);
         delete requestWithAuth;
         return 0;
      }
   } 
   else 
   {
      // Handles digestAccepted == false, DigestNotAccepted and any other
      // case that is not recognised by the foregoing logic

      InfoLog (<< "Invalid password provided for " << userAuth->getUser() << " in " << userAuth->getRealm());
      InfoLog (<< "  a1 hash of password from db was " << userAuth->getA1() );

      SharedPtr<SipMessage> response(new SipMessage);
      Helper::makeResponse(*response, *requestWithAuth, 403, "Invalid password provided");
      mDum.send(response);
      onAuthFailure(BadCredentials, *requestWithAuth);
      delete requestWithAuth;
      return 0;
   }
}

            
bool
ServerAuthManager::useAuthInt() const
{
   return false;
}


bool
ServerAuthManager::proxyAuthenticationMode() const
{
   return true;
}


bool
ServerAuthManager::rejectBadNonces() const
{
   return true;
}


ServerAuthManager::AsyncBool
ServerAuthManager::requiresChallenge(const SipMessage& msg)
{
   if(!mChallengeThirdParties)
   {
      const Uri& fromUri = msg.header(h_From).uri();
      if(!mDum.isMyDomain(fromUri.host()))
         return False;
   }
   return True;  
}


bool
ServerAuthManager::authorizedForThisIdentity(const resip::Data &user, 
                                               const resip::Data &realm, 
                                                resip::Uri &fromUri)
{
   // !rwm! good enough for now.  TODO eventually consult a database to see what
   // combinations of user/realm combos are authorized for an identity

   // First try the form where the username parameter in the auth
   // header is just the username component of the fromUri
   //
   if ((fromUri.user() == user) && (fromUri.host() == realm))
      return true;

   // Now try the form where the username parameter in the auth
   // header is the full fromUri, e.g.
   //    Proxy-Authorization: Digest username="user@domain" ...
   //
   if (fromUri.getAorNoPort() == user)
      return true;

   // catch-all: access denied
   return false;
}


const Data& 
ServerAuthManager::getChallengeRealm(const SipMessage& msg)
{
   // (1) Check if static realm is defined
   if (!mStaticRealm.empty())
   {
      return mStaticRealm;
   }

   // (2) Check From domain
   if (mDum.isMyDomain(msg.header(h_From).uri().host()))
   {
      return msg.header(h_From).uri().host();
   }

   // (3) Punt: Use Request URI
   return msg.header(h_RequestLine).uri().host();
}


bool
ServerAuthManager::isMyRealm(const Data& realm)
{
   if(!mStaticRealm.empty())
   {
      return mStaticRealm == realm;
   }
   return mDum.isMyDomain(realm);
}


// return true if request has been consumed 
ServerAuthManager::Result
ServerAuthManager::handle(SipMessage* sipMsg)
{
   //InfoLog( << "trying to do auth" );
   if (sipMsg->isRequest())
   {
      if(sipMsg->method() == CANCEL)
      {
         // If we receive a cancel - check to see if we have the matching INVITE in our message map.
         // If we do, then we haven't created a DUM dialog for it yet, since we are still waiting
         // for the credential information to arrive.  We need to properly respond to the CANCEL here
         // since it won't be handled externally.
         MessageMap::iterator it = mMessages.find(sipMsg->getTransactionId());
         if(it != mMessages.end())
         {
            // Ensure message is an INVITE - if not then something fishy is going on.  Either
            // someone has cancelled a non-INVITE transaction or we have a tid collision.
            if(it->second->isRequest() && it->second->method() == INVITE)
            {
               std::auto_ptr<SipMessage> inviteMsg(it->second);
               mMessages.erase(it);  // Remove the INVITE from the message map and respond to it

               InfoLog (<< "Received a CANCEL for an INVITE request that we are still waiting on auth "
                        << "info for, responding appropriately, tid=" 
                        << sipMsg->getTransactionId());

               // Send 487/Inv
               SharedPtr<SipMessage> inviteResponse(new SipMessage);
               Helper::makeResponse(*inviteResponse, *inviteMsg, 487);  // Request Cancelled
               mDum.send(inviteResponse);

               // Send 200/Cancel
               SharedPtr<SipMessage> cancelResponse(new SipMessage);
               Helper::makeResponse(*cancelResponse, *sipMsg, 200);  
               mDum.send(cancelResponse);

               return Rejected; // Use rejected since handling is what we want - stop DUM from processing the cancel any further
            }
         }
      }
      else if(sipMsg->method() != ACK)  // Do not challenge ACKs or CANCELs (picked off above)
      {
         ParserContainer<Auth>* auths;
         if (proxyAuthenticationMode())
         {
            if(!sipMsg->exists(h_ProxyAuthorizations))
            {
               return issueChallengeIfRequired(sipMsg);
            }
            auths = &sipMsg->header(h_ProxyAuthorizations);
         }
         else
         {
            if(!sipMsg->exists(h_Authorizations))
            {
               return issueChallengeIfRequired(sipMsg);
            }
            auths = &sipMsg->header(h_Authorizations);
         }
 
         try
         {
            for(Auths::iterator it = auths->begin(); it != auths->end(); it++)
            {
               if (isMyRealm(it->param(p_realm)))
               {
                  InfoLog (<< "Requesting credential for " 
                           << it->param(p_username) << " @ " << it->param(p_realm));
               
                  requestCredential(it->param(p_username),
                                    it->param(p_realm), 
                                    *sipMsg,
                                     *it,
                                    sipMsg->getTransactionId());
                  mMessages[sipMsg->getTransactionId()] = sipMsg;
                  return RequestedCredentials;
               }
            }

            InfoLog (<< "Didn't find matching realm ");
            return issueChallengeIfRequired(sipMsg);
         }
         catch(BaseException& e)
         {
            InfoLog (<< "Invalid auth header provided " << e);
            SharedPtr<SipMessage> response(new SipMessage);
            Helper::makeResponse(*response, *sipMsg, 400, "Invalid auth header");
            mDum.send(response);
            onAuthFailure(InvalidRequest, *sipMsg);
            return Rejected;
         }
      }
   }
   return Skipped;
}

ServerAuthManager::Result
ServerAuthManager::issueChallengeIfRequired(SipMessage *sipMsg) 
{
   // Is challenge required for this message
   AsyncBool required = requiresChallenge(*sipMsg);
   switch(required) 
   {
     case False:
        return Skipped;
     case Async:
        mMessages[sipMsg->getTransactionId()] = sipMsg;
        return RequestedInfo;
     case True:
     default:
        issueChallenge(sipMsg);
        return Challenged;
   }
}

void
ServerAuthManager::issueChallenge(SipMessage *sipMsg) 
{
  //assume TransactionUser has matched/repaired a realm
  SharedPtr<SipMessage> challenge(Helper::makeChallenge(*sipMsg,
                                                        getChallengeRealm(*sipMsg), 
                                                        useAuthInt(), 
                                                        false /*stale*/,
                                                        proxyAuthenticationMode()));

  InfoLog (<< "Sending challenge to " << sipMsg->brief());
  mDum.send(challenge);
}

void 
ServerAuthManager::onAuthSuccess(const SipMessage& msg) 
{
   // sub class may want to create a log entry
}

void 
ServerAuthManager::onAuthFailure(AuthFailureReason reason, const SipMessage& msg) 
{
   // sub class may want to create a log entry
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
