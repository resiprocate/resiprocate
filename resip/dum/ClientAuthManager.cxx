#include "rutil/ResipAssert.h"

#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/UserProfile.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "resip/dum/ClientAuthExtension.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

class ClientAuthDecorator : public MessageDecorator
{
public:
   ClientAuthDecorator(bool isProxyCredential, const Auth& auth, const UserProfile::DigestCredential& credential, const Data& authQop, const Data& nonceCountString) :
      mIsProxyCredential(isProxyCredential), mAuth(auth), mCredential(credential), mAuthQop(authQop), mNonceCountString(nonceCountString) {}
   virtual ~ClientAuthDecorator() {}
   virtual void decorateMessage(SipMessage &msg, 
                                const Tuple &source,
                                const Tuple &destination,
                                const Data& sigcompId)
   {
      Data cnonce = Random::getCryptoRandomHex(16);

      Auths & target = mIsProxyCredential ? msg.header(h_ProxyAuthorizations) : msg.header(h_Authorizations);
   
      DebugLog( << " Add auth, " << this << " in response to: " << mAuth);
      Auth auth;
      if (ClientAuthExtension::instance().algorithmAndQopSupported(mAuth))
      {
         DebugLog(<<"Using extension to make auth response");
      
         if(mCredential.isPasswordA1Hash)
         {
            ClientAuthExtension::instance().makeChallengeResponseAuthWithA1(msg,
                                                            mCredential.user,
                                                            mCredential.password,
                                                            mAuth, 
                                                            cnonce,
                                                            mAuthQop, 
                                                            mNonceCountString,
                                                            auth);      
         }
         else
         {
            ClientAuthExtension::instance().makeChallengeResponseAuth(msg,
                                                            mCredential.user,
                                                            mCredential.password,
                                                            mAuth, 
                                                            cnonce,
                                                            mAuthQop, 
                                                            mNonceCountString,
                                                            auth);      
         }
      }
      else
      {
         if(mCredential.isPasswordA1Hash)
         {
            Helper::makeChallengeResponseAuthWithA1(msg, 
                                           mCredential.user, 
                                           mCredential.password, 
                                           mAuth, 
                                           cnonce, 
                                           mAuthQop, 
                                           mNonceCountString, 
                                           auth);
         }
         else
         {
            Helper::makeChallengeResponseAuth(msg, 
                                           mCredential.user, 
                                           mCredential.password, 
                                           mAuth, 
                                           cnonce, 
                                           mAuthQop, 
                                           mNonceCountString, 
                                           auth);
         }
      }
      target.push_back(auth);
   
      DebugLog(<<"ClientAuthDecorator, proxy: " << mIsProxyCredential << " " << target.back());
   }
   virtual void rollbackMessage(SipMessage& msg) 
   {
      Auths & target = mIsProxyCredential ? msg.header(h_ProxyAuthorizations) : msg.header(h_Authorizations);
      target.pop_back();
   }  
   virtual MessageDecorator* clone() const { return new ClientAuthDecorator(mIsProxyCredential, mAuth, mCredential, mAuthQop, mNonceCountString); }
private:
    bool mIsProxyCredential;
    Auth mAuth;
    UserProfile::DigestCredential mCredential;
    Data mAuthQop;
    Data mNonceCountString;
};


ClientAuthManager::ClientAuthManager() 
{
}

bool 
ClientAuthManager::handle(UserProfile& userProfile, SipMessage& origRequest, const SipMessage& response)
{
   try
   {
      resip_assert( response.isResponse() );
      resip_assert( origRequest.isRequest() );
      
      DialogSetId id(origRequest);

      const int& code = response.header(h_StatusLine).statusCode();
      if (code < 101 || code >= 500)
      {
         return false;
      }
      else if (! (  code == 401 || code == 407 )) // challenge success
      {
         AttemptedAuthMap::iterator it = mAttemptedAuths.find(id);     
         if (it != mAttemptedAuths.end())
         {
            DebugLog (<< "ClientAuthManager::handle: transitioning " << id << "to cached");         

            // cache the result
            it->second.authSucceeded();
         }      
         return false;
      }   

      // 401 or 407...
      if (!(response.exists(h_WWWAuthenticates) || response.exists(h_ProxyAuthenticates)))
      {
         DebugLog (<< "Invalid challenge for " << id  << ", nothing to respond to; fail");         
         return false;
      }
   
      AuthState& authState = mAttemptedAuths[id];

      // based on the UserProfile and the challenge, store credentials in the
      // AuthState associated with this DialogSet if the algorithm is supported
      if (authState.handleChallenge(userProfile, response))
      {
         resip_assert(origRequest.header(h_Vias).size() == 1);
         origRequest.header(h_CSeq).sequence()++;
         DebugLog (<< "Produced response to digest challenge for " << userProfile );
         return true;
      }
      else
      {
         return false;
      }
   }
   catch(BaseException& e)
   {
      resip_assert(0);
      ErrLog(<< "Unexpected exception in ClientAuthManager::handle " << e);
      return false;
   }      
}

void
ClientAuthManager::addAuthentication(SipMessage& request)
{
   AttemptedAuthMap::iterator it = mAttemptedAuths.find(DialogSetId(request));
   if (it != mAttemptedAuths.end())
   {
      it->second.addAuthentication(request);
   }
}

void 
ClientAuthManager::clearAuthenticationState(const DialogSetId& dsId)
{
   AttemptedAuthMap::iterator it = mAttemptedAuths.find(dsId);
   if (it != mAttemptedAuths.end())
   {
      mAttemptedAuths.erase(it);
   }
}

void 
ClientAuthManager::dialogSetDestroyed(const DialogSetId& id)
{
   clearAuthenticationState(id);
}

ClientAuthManager::AuthState::AuthState() :
   mFailed(false),
   mCacheUseLimit(0),
   mCacheUseCount(0)
{
}

bool 
ClientAuthManager::AuthState::handleChallenge(UserProfile& userProfile, const SipMessage& challenge)
{
   if (mFailed)
   {
      return false;
   }   
   bool handled = true;
   if (challenge.exists(h_WWWAuthenticates))
   {
      for (Auths::const_iterator i = challenge.header(h_WWWAuthenticates).begin();  
         i != challenge.header(h_WWWAuthenticates).end(); ++i)                    
      {    
         if (i->exists(p_realm))
         {
            if (!mRealms[i->param(p_realm)].handleAuth(userProfile, *i, false))
            {
               handled = false;
               break;
            }
         }
         else
         {
            return false;
         }
      }
   }
   if (challenge.exists(h_ProxyAuthenticates))
   {
      for(Auths::const_iterator i = challenge.header(h_ProxyAuthenticates).begin();  
          i != challenge.header(h_ProxyAuthenticates).end(); ++i)                    
      {    
         if (i->exists(p_realm))
         {
            if (!mRealms[i->param(p_realm)].handleAuth(userProfile, *i, true))
            {
               handled = false;
               break;
            }
         }
         else
         {
            return false;
         }
      }
   }
   if(!handled)
   {
      InfoLog( << "ClientAuthManager::AuthState::handleChallenge failed for: " << challenge);
   }
   else
   {
      mCacheUseLimit = userProfile.getDigestCacheUseLimit();
   }
   return handled;
}

void 
ClientAuthManager::AuthState::authSucceeded()
{
   for(RealmStates::iterator i = mRealms.begin(); i!=mRealms.end(); i++)
   {
      i->second.authSucceeded();
   }
   mCacheUseCount++;
   if(mCacheUseLimit != 0 && mCacheUseCount >= mCacheUseLimit)
   {
      // Cache use limit reached - clear auth state
      mRealms.clear();
      mCacheUseCount = 0;
   }
}

void 
ClientAuthManager::AuthState::addAuthentication(SipMessage& request)
{
   request.remove(h_ProxyAuthorizations);
   request.remove(h_Authorizations);  

   if (mFailed) return;

   for(RealmStates::iterator i = mRealms.begin(); i!=mRealms.end(); i++)
   {
      i->second.addAuthentication(request);
   }
}
   

ClientAuthManager::RealmState::RealmState() :
   mIsProxyCredential(false),
   mState(Invalid),
   mNonceCount(0),
   mAuthPtr(NULL)
{
}

Data RealmStates[] = 
{
   "invalid",
   "cached",
   "current",
   "tryonce",
   "failed",
};

const Data&
ClientAuthManager::RealmState::getStateString(State s) 
{
   return RealmStates[s];
}

void 
ClientAuthManager::RealmState::transition(State s)
{
   DebugLog(<< "ClientAuthManager::RealmState::transition from " << getStateString(mState) << " to " << getStateString(s));
   mState = s;
}

void
ClientAuthManager::RealmState::authSucceeded()
{
   switch(mState)
   {
      case Invalid:
         resip_assert(0);
         break;
      case Current:
      case Cached:
      case TryOnce:
         transition(Cached);
         break;
      case Failed:
         resip_assert(0);
         break;         
   };
}

bool 
ClientAuthManager::RealmState::handleAuth(UserProfile& userProfile, const Auth& auth, bool isProxyCredential)
{   
   DebugLog( << "ClientAuthManager::RealmState::handleAuth: " << this << " " << auth << " is proxy: " << isProxyCredential);
   mIsProxyCredential = isProxyCredential;   //this changing dynamically would
                                             //be very bizarre..should trap w/ enum
   switch(mState)
   {
      case Invalid:
         mAuth = auth;
         transition(Current);
         break;         
      case Current:
         if (auth.exists(p_stale) && auth.param(p_stale) == "true")
         {
            DebugLog (<< "Stale nonce:" <<  auth);
            mAuth = auth;
            clear();
         }
         else if(auth.exists(p_nonce) && auth.param(p_nonce) != mAuth.param(p_nonce))
         {
            DebugLog (<< "Different nonce, was: " << mAuth.param(p_nonce) << " now " << auth.param(p_nonce));
            mAuth = auth;
            clear();
            transition(TryOnce);            
         }
         else
         {
            DebugLog( << "Challenge response already failed for: " << auth);
            transition(Failed);            
            return false;
         }
         break;         
      case TryOnce:
         DebugLog( << "Extra chance still failed: " << auth);
         transition(Failed);
         return false;
      case Cached: //basically 1 free chance, here for interop, may not be
                   //required w/ nonce check in current
         mAuth = auth;
         clear();
         transition(Current);
         break;         
      case Failed:
         return false;
   }

   if (findCredential(userProfile, auth))
   {
      return true;
   }
   else
   {
      transition(Failed);
      return false;
   }
}

void
ClientAuthManager::RealmState::clear()
{
   mNonceCount = 0;
}
                     
bool 
ClientAuthManager::RealmState::findCredential(UserProfile& userProfile, const Auth& auth)
{
   if (!(Helper::algorithmAndQopSupported(auth) 
         || (ClientAuthExtension::instance().algorithmAndQopSupported(auth))))
   {
      DebugLog(<<"Unsupported algorithm or qop: " << auth);
      return false;
   }

   const Data& realm = auth.param(p_realm);                   
   //!dcm! -- icky, expose static empty soon...ptr instead of reference?
   mCredential = userProfile.getDigestCredential(realm);
   if (mCredential.realm.empty())                       
   {                                        
      DebugLog( << "Got a 401 or 407 but could not find credentials for realm: " << realm);
      // DebugLog (<< auth);
      // DebugLog (<< response);
      return false;
   }                     
   return true;   
}

void 
ClientAuthManager::RealmState::addAuthentication(SipMessage& request)
{
   resip_assert(mState != Failed);
   if (mState == Failed) return;

   Data nonceCountString;
   Data authQop = Helper::qopOption(mAuth);
   if(!authQop.empty())
   {
       Helper::updateNonceCount(mNonceCount, nonceCountString);
   }
   
   // Add client auth decorator so that we ensure any body hashes are calcuated after user defined outbound decorators that
   // may be modifying the message body
   std::auto_ptr<MessageDecorator> clientAuthDecorator(new ClientAuthDecorator(mIsProxyCredential, mAuth, mCredential, authQop, nonceCountString));
   request.addOutboundDecorator(clientAuthDecorator);
}

// bool
// ClientAuthManager::CompareAuth::operator()(const Auth& lhs, const Auth& rhs) const
// {
//    if (lhs.param(p_realm) < rhs.param(p_realm))
//    {
//       return true;
//    }
//    else if (lhs.param(p_realm) > rhs.param(p_realm))
//    {
//       return false;
//    }
//    else
//    {
//       return lhs.param(p_username) < rhs.param(p_username);
//    }
// }


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
