#include <cassert>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"

#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

bool 
ClientAuthManager::handle(SipMessage& origRequest, const SipMessage& response)
{
   assert( response.isResponse() );
   assert( origRequest.isRequest() );
   
   // is this a 401 or 407  
   const int& code = response.header(h_StatusLine).statusCode();
   if ( (code!=401) && (code!=407) )
   {
      return false;
   }
   
   for (Auths::const_iterator i = response.header(h_Authorizations).begin();
        i != response.header(h_Authorizations).end(); ++i)
   {
      const Data& realm = i->param(p_realm);

      const Profile::DigestCredential& credential = mProfile.getDigestCredential(realm);
      if ( credential.password.empty() )
      {
         InfoLog( << "Got a 401 or 407 but could not find credentials for realm ans user" );
         return false;
      }

      const Data cnonce = Data::Empty;
      unsigned int nonceCount=0;
      // construct the new headers and modify origRequest 
      Helper::addAuthorization(origRequest,response,
                               credential.user,credential.password,
                               cnonce,nonceCount);
   }

   return true;
}
