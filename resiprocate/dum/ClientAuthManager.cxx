#include <cassert>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"

#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"

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
   if ( mAttemptedAuths.find(id) != mAttemptedAuths.end())
   {
      mAttemptedAuths.erase(id);
      return false;
   }
      
   // is this a 401 or 407  
   const int& code = response.header(h_StatusLine).statusCode();
   if (! (  code == 401 || code == 407 ))
   {
      return false;
   }
   mAttemptedAuths.insert(id);

   InfoLog (<< "Doing client auth");
   // !ah! TODO : check ALL appropriate auth headers.
   if (response.exists(h_WWWAuthenticates))
   {      
      for (Auths::const_iterator i = response.header(h_WWWAuthenticates).begin();  
           i != response.header(h_WWWAuthenticates).end(); ++i)                    
      {    
         handleAuthHeader(*i, origRequest, response);         
      }
   }
   if (response.exists(h_ProxyAuthenticates))
   {      
      for (Auths::const_iterator i = response.header(h_ProxyAuthenticates).begin();  
           i != response.header(h_ProxyAuthenticates).end(); ++i)                    
      {    
         if (!handleAuthHeader(*i, origRequest, response))
         {
            return false;
         }
      }
   }
   assert(origRequest.header(h_Vias).size() == 1);
   //new transaction
   origRequest.header(h_Vias).front().param(p_branch).reset();
   return true;
}

bool ClientAuthManager::handleAuthHeader(const Auth& auth, SipMessage& origRequest, const SipMessage& response)
{
   const Data& realm = auth.param(p_realm);                   
   
   //!dcm! -- icky, expose static empty soon...ptr instead of reference?
   Profile::DigestCredential credential =            
      mProfile.getDigestCredential(realm);
   if ( credential.password.empty() )                       
   {                                        
      credential = mProfile.getDigestCredential(response);
      if ( credential.password.empty() )                       
      {                                        
         InfoLog( << "Got a 401 or 407 but could not find credentials for realm: " << realm);
         return false;                                        
      }
   }                                                        
   
   const Data cnonce = Data::Empty;                         
   unsigned int nonceCount=0;                               
   InfoLog (<< "Adding authorization: " << credential.user);
   
   Helper::addAuthorization(origRequest,response,           
                            credential.user,credential.password, 
                            cnonce,nonceCount);
   origRequest.header(h_CSeq).sequence()++;
   return true;
}

