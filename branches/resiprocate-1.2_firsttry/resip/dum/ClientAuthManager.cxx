#include <cassert>

#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/UserProfile.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

ClientAuthManager::ClientAuthManager() 
{
}


bool 
ClientAuthManager::handle(UserProfile& userProfile, SipMessage& origRequest, const SipMessage& response)
{
   try
   {
      assert( response.isResponse() );
      assert( origRequest.isRequest() );
      
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
         assert(origRequest.header(h_Vias).size() == 1);
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
      assert(0);
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


ClientAuthManager::AuthState::AuthState() :
   mFailed(false)
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
      for (Auths::const_iterator i = challenge.header(h_ProxyAuthenticates).begin();  
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
      if(!handled)
      {
         InfoLog( << "ClientAuthManager::AuthState::handleChallenge failed for: " << challenge);
      }
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
   mNonceCount(0)
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
         assert(0);
         break;
      case Current:
      case Cached:
      case TryOnce:
         transition(Cached);
         break;
      case Failed:
         assert(0);
         break;         
   };
}

bool 
ClientAuthManager::RealmState::handleAuth(UserProfile& userProfile, const Auth& auth, bool isProxyCredential)
{   
   DebugLog( << "ClientAuthManager::RealmState::handleAuth: " << this << " " << auth << " is proxy: " << isProxyCredential);
   mIsProxyCredential = isProxyCredential;   //this cahnging dynamically would
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
   if (!Helper::algorithmAndQopSupported(auth))
   {
      DebugLog(<<"Unsupported algorithm or qop: " << auth);
      return false;
   }

   const Data& realm = auth.param(p_realm);                   
   //!dcm! -- icky, expose static empty soon...ptr instead of reference?
   mCredential = userProfile.getDigestCredential(realm);
   if ( mCredential.realm.empty() )                       
   {                                        
      DebugLog( << "Got a 401 or 407 but could not find credentials for realm: " << realm);
//      DebugLog (<< auth);
//      DebugLog (<< response);
      return false;
   }                     
   return true;   
}

void 
ClientAuthManager::RealmState::addAuthentication(SipMessage& request)
{
   assert(mState != Failed);
   if (mState == Failed) return;

   Data cnonce = Random::getCryptoRandomHex(8);

   Auths & target = mIsProxyCredential ? request.header(h_ProxyAuthorizations) : request.header(h_Authorizations);
   Data nonceCountString;
   
   DebugLog( << " Add auth, " << this << " in response to: " << mAuth);
   target.push_back(Helper::makeChallengeResponseAuth(request,
                                                      mCredential.user,
                                                      mCredential.password,
                                                      mAuth, 
                                                      cnonce,
                                                      mNonceCount, 
                                                      nonceCountString));
   DebugLog(<<"ClientAuthManager::RealmState::addAuthentication, proxy: " << mIsProxyCredential << " " << target.back());
}

void ClientAuthManager::dialogSetDestroyed(const DialogSetId& id)
{
   if ( mAttemptedAuths.find(id) != mAttemptedAuths.end())
   {
      mAttemptedAuths.erase(id);
   }
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
