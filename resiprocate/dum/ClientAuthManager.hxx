#if !defined(RESIP_CLIENTAUTHMANAGER_HXX)
#define RESIP_CLIENTAUTHMANAGER_HXX

#include "resiprocate/dum/DialogSetId.hxx"
#include "resiprocate/dum/Profile.hxx"

#include <map>
#include <functional>

namespace resip
{

class Auth;
class SipMessage;

class ClientAuthManager
{
   public:
      ClientAuthManager(Profile& profile);

      // return true if request is authorized
      bool handle(SipMessage& origRequest, const SipMessage& response);
      void addAuthentication(SipMessage& origRequest);
      
   private:
      friend class DialogSet;
      void dialogSetDestroyed(const DialogSetId& dsId);      

      class CompareAuth  : public std::binary_function<const Auth&, const Auth&, bool>
      {
         public:
            bool operator()(const Auth& lhs, const Auth& rhs) const;
      };      
         
      typedef enum
      {
         Invalid,
         Cached,
         Current,
         Failed
      } State;      

      class AuthState
      {
         public:     
            AuthState();
            
            void clear() 
            {
               proxyCredentials.clear();
               wwwCredentials.clear();
            }
            //could work to iterator pointing to digest credential
            typedef std::map<Auth, Profile::DigestCredential, CompareAuth > CredentialMap;            
            CredentialMap proxyCredentials;
            CredentialMap wwwCredentials;            

            State state;            
            unsigned int cnonceCount;
            Data cnonce;
            Data cnonceCountString;            
      };      
         

      typedef std::map<DialogSetId, AuthState> AttemptedAuthMap;

      Profile& mProfile;
      bool handleAuthHeader(const Auth& auth, AttemptedAuthMap::iterator authState, SipMessage& origRequest, 
                            const SipMessage& response, bool proxy);      

      AttemptedAuthMap mAttemptedAuths;      
};
 
}

#endif
