#include <cassert>

#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

ClientAuthManager::ClientAuthManager(Profile& profile) :
   mProfile(profile)
{
}


bool 
ClientAuthManager::handle(SipMessage& origRequest, const SipMessage& response)
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
            it->second.state = Failed;         
//         mAttemptedAuths.erase(it);
            return false;
         }
      }
      else if (it->second.state == Failed)
      {
         it->second.state = Failed;         
//         mAttemptedAuths.erase(it);
         return false;
      }
      else
      {
         //not sure about this clear
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
      return false;
   }

   if (response.exists(h_WWWAuthenticates))
   {      
      for (Auths::const_iterator i = response.header(h_WWWAuthenticates).begin();  
           i != response.header(h_WWWAuthenticates).end(); ++i)                    
      {    
         if (!handleAuthHeader(*i, it, origRequest, response, false))
         {
            it->second.state = Failed;   
            return false;
         }
      }
   }
   if (response.exists(h_ProxyAuthenticates))
   {      
      for (Auths::const_iterator i = response.header(h_ProxyAuthenticates).begin();  
           i != response.header(h_ProxyAuthenticates).end(); ++i)                    
      {    
         if (!handleAuthHeader(*i, it, origRequest, response, true))
         {
            it->second.state = Failed;   
            return false;
         }
      }
   }
   assert(origRequest.header(h_Vias).size() == 1);
   origRequest.header(h_CSeq).sequence()++;
   return true;
}

bool ClientAuthManager::handleAuthHeader(const Auth& auth, AttemptedAuthMap::iterator authState,
                                         SipMessage& origRequest, 
                                         const SipMessage& response, bool proxy)
{
   const Data& realm = auth.param(p_realm);                   
   
   //!dcm! -- icky, expose static empty soon...ptr instead of reference?
   Profile::DigestCredential credential = mProfile.getDigestCredential(realm);
   if ( credential.password.empty() )                       
   {                                        
      credential = mProfile.getDigestCredential(response);
      if ( credential.password.empty() )                       
      {                                        
         InfoLog( << "Got a 401 or 407 but could not find credentials for realm: " << realm);
         DebugLog (<< auth);
         DebugLog (<< response);
         return false;                                        
      }
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

void ClientAuthManager::addAuthentication(SipMessage& request)
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
      
      for (AuthState::CredentialMap::iterator it = authState.wwwCredentials.begin(); 
           it != authState.wwwCredentials.end(); it++)
      {
         authState.cnonceCountString.clear();         
         request.header(h_Authorizations).push_back(Helper::makeChallengeResponseAuth(request,
                                                                                      it->second.user,
                                                                                      it->second.password,
                                                                                      it->first,
                                                                                      authState.cnonce, 
                                                                                      authState.cnonceCount,
                                                                                      authState.cnonceCountString));
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
   cnonceCount(0),
   cnonce(Random::getCryptoRandomHex(8)) //weak, should have ntp or something
{}            
