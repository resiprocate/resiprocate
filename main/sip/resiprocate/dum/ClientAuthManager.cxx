#include <cassert>

#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/UserProfile.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

ClientAuthManager::ClientAuthManager() 
{
}


bool 
ClientAuthManager::handle(UserProfile& userProfile, SipMessage& origRequest, const SipMessage& response)
{
   assert( response.isResponse() );
   assert( origRequest.isRequest() );

   DialogSetId id(origRequest);
   AttemptedAuthMap::iterator it = mAttemptedAuths.find(id);

   // is this a 401 or 407  
   const int& code = response.header(h_StatusLine).statusCode();
   if (code < 180)
   {
      return false;
   }
   else if (! (  code == 401 || code == 407 ))
   {
      if (it != mAttemptedAuths.end())
      {
         it->second.state = Cached;
      }      
      return false;
   }   
   
   //one try per credential, one credential per user per realm
   if (it != mAttemptedAuths.end())
   {
      if (it->second.state == Current)
      {
         bool stale = false;         
         if (response.exists(h_WWWAuthenticates))
         {      
            for (Auths::const_iterator i = response.header(h_WWWAuthenticates).begin();  
                 i != response.header(h_WWWAuthenticates).end(); ++i)                    
            {    
               if (i->exists(p_stale) && isEqualNoCase(i->param(p_stale), "true"))
               {
                  stale = true;
                  break;
               }
            }
         }
         if (response.exists(h_ProxyAuthenticates))
         {      
            for (Auths::const_iterator i = response.header(h_ProxyAuthenticates).begin();  
                 i != response.header(h_ProxyAuthenticates).end(); ++i)                    
            {    
               if (i->exists(p_stale) && isEqualNoCase(i->param(p_stale), "true"))
               {
                  stale = true;
                  break;
               }
            }
         }
         if (!stale)
         {
            InfoLog (<< "Failed client auth for " << userProfile << endl << response);
            it->second.state = Failed;         
//         mAttemptedAuths.erase(it);
            return false;
         }
         else
         {
            it->second.clear();
         }
      }
      else if (it->second.state == Failed)
      {
         it->second.state = Failed;         
         InfoLog (<< "Failed client auth for " << userProfile << endl << response);
//         mAttemptedAuths.erase(it);
         return false;
      }
      else
      {
         it->second.clear();
      }
   }
   else
   {
      //yeah, yeah, should be tricky insert w/ make_pair
      mAttemptedAuths[id] = AuthState();
      it = mAttemptedAuths.find(id);
   }   
   
   it->second.state = Current;   

   DebugLog (<< "Doing client auth");
   // !ah! TODO : check ALL appropriate auth headers.
   if (!(response.exists(h_WWWAuthenticates) || response.exists(h_ProxyAuthenticates)))
   {
      it->second.state = Failed;
      InfoLog (<< "Failed client auth for " << userProfile << endl << response);
      return false;
   }

   if (response.exists(h_WWWAuthenticates))
   {      
      for (Auths::const_iterator i = response.header(h_WWWAuthenticates).begin();  
           i != response.header(h_WWWAuthenticates).end(); ++i)                    
      {    
         if (!handleAuthHeader(userProfile, *i, it, origRequest, response, false))
         {
            it->second.state = Failed;   
            InfoLog (<< "Failed client auth for " << userProfile << endl << response);
            return false;
         }
      }
   }
   if (response.exists(h_ProxyAuthenticates))
   {      
      for (Auths::const_iterator i = response.header(h_ProxyAuthenticates).begin();  
           i != response.header(h_ProxyAuthenticates).end(); ++i)                    
      {    
         if (!handleAuthHeader(userProfile, *i, it, origRequest, response, true))
         {
            it->second.state = Failed;   
            InfoLog (<< "Failed client auth for " << userProfile << endl << response);
            return false;
         }
      }
   }
   assert(origRequest.header(h_Vias).size() == 1);
   origRequest.header(h_CSeq).sequence()++;
   DebugLog (<< "Produced response to digest challenge for " << userProfile );
   return true;
}

bool 
ClientAuthManager::handleAuthHeader(UserProfile& userProfile, 
                                    const Auth& auth, 
                                    AttemptedAuthMap::iterator authState,
                                    SipMessage& origRequest, 
                                    const SipMessage& response, 
                                    bool proxy)
{
   const Data& realm = auth.param(p_realm);                   
   
   //!dcm! -- icky, expose static empty soon...ptr instead of reference?
   UserProfile::DigestCredential credential = userProfile.getDigestCredential(realm);
   if ( credential.realm.empty() )                       
   {                                        
      InfoLog( << "Got a 401 or 407 but could not find credentials for realm: " << realm);
      DebugLog (<< auth);
      DebugLog (<< response);
      return false;                                        
   }                                                           
   if (proxy)
   {
      authState->second.proxyCredentials[auth] = credential;
   }
   else
   {
      authState->second.wwwCredentials[auth] = credential;
   }
   return true;   
}

void 
ClientAuthManager::addAuthentication(SipMessage& request)
{
   DialogSetId id(request);
   AttemptedAuthMap::iterator itState = mAttemptedAuths.find(id);
   
   if (itState != mAttemptedAuths.end())
   {      
      AuthState& authState = itState->second;
      assert(authState.state != Invalid);
      if (authState.state == Failed)
      {
         return;
      }
      
      request.remove(h_ProxyAuthorizations);
      request.remove(h_Authorizations);  

      authState.cnonce = Random::getCryptoRandomHex(8); //!dcm! -- inefficient
      
      for (AuthState::CredentialMap::iterator it = authState.wwwCredentials.begin(); 
           it != authState.wwwCredentials.end(); it++)
      {
         authState.cnonceCountString.clear();         
         request.header(h_Authorizations).push_back( Helper::makeChallengeResponseAuth(request,
                                                                                             it->second.user,
                                                                                             it->second.password,
                                                                                             it->first,
                                                                                             authState.cnonce, 
                                                                                             authState.cnonceCount,
                                                                                             authState.cnonceCountString) );
      }
      for (AuthState::CredentialMap::iterator it = authState.proxyCredentials.begin(); 
           it != authState.proxyCredentials.end(); it++)
      {
         authState.cnonceCountString.clear();         
         request.header(h_ProxyAuthorizations).push_back(Helper::makeChallengeResponseAuth(request,
                                                                                                 it->second.user,
                                                                                                 it->second.password,
                                                                                                 it->first,
                                                                                                 authState.cnonce, 
                                                                                                 authState.cnonceCount,
                                                                                                 authState.cnonceCountString));
      }

   }
}

void ClientAuthManager::dialogSetDestroyed(const DialogSetId& id)
{
   if ( mAttemptedAuths.find(id) != mAttemptedAuths.end())
   {
      mAttemptedAuths.erase(id);
   }
}

bool
ClientAuthManager::CompareAuth::operator()(const Auth& lhs, const Auth& rhs) const
{
   if (lhs.param(p_realm) < rhs.param(p_realm))
   {
      return true;
   }
   else if (lhs.param(p_realm) > rhs.param(p_realm))
   {
      return false;
   }
   else
   {
      return lhs.param(p_username) < rhs.param(p_username);
   }
}

ClientAuthManager::AuthState::AuthState() :
   state(Invalid),
   cnonceCount(0)
{}            

void 
ClientAuthManager::AuthState::clear()
{
   proxyCredentials.clear();
   wwwCredentials.clear();
   cnonceCount = 0;   
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
