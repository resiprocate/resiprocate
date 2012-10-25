#if !defined(RESIP_CLIENTAUTHMANAGER_HXX)
#define RESIP_CLIENTAUTHMANAGER_HXX

#include "resip/dum/DialogSetId.hxx"
#include "resip/dum/UserProfile.hxx"
#include "rutil/SharedPtr.hxx"

#include <map>
#include <functional>

namespace resip
{

class Auth;
class SipMessage;
class ClientAuthExtension;


class ClientAuthManager
{
   public:
      ClientAuthManager();
      virtual ~ClientAuthManager() {}
      
      // For any response received by the UAC, handle will be
      // called. origRequest is the request that generated the 401/407.
      // return true if the challenge can be handled with an updated request. 
      // This will increment the CSeq on origRequest
      virtual bool handle(UserProfile& userProfile, SipMessage& origRequest, const SipMessage& response);

      //
      virtual void addAuthentication(SipMessage& origRequest);
      virtual void clearAuthenticationState(const DialogSetId& dsId);
      
   private:
      friend class DialogSet;
      virtual void dialogSetDestroyed(const DialogSetId& dsId);      

//       class CompareAuth  : public std::binary_function<const Auth&, const Auth&, bool>
//       {
//          public:
//             bool operator()(const Auth& lhs, const Auth& rhs) const;
//       };      
         
      class RealmState
      {
         public:     
            RealmState();
            
            void clear();

            bool handleAuth(UserProfile& userProfile, const Auth& auth, bool isProxyCredential);
            void authSucceeded();

            void addAuthentication(SipMessage& origRequest);            
         private:
            typedef enum
            {
               Invalid,
               Cached,
               Current,
               TryOnce, 
               Failed
            } State;      

            void transition(State s);
            static const Data& getStateString(State s);
            bool findCredential(UserProfile& userProfile, const Auth& auth);  
            UserProfile::DigestCredential mCredential;
            bool mIsProxyCredential;
            
            State mState;            
            unsigned int mNonceCount;
            Auth mAuth;            

            // FH add the realm state so it can change
            Auth *mAuthPtr;
            
            // .dcm. only one credential per realm per challenge supported
            // typedef std::map<Auth, UserProfile::DigestCredential, CompareAuth > CredentialMap;            
            // CredentialMap proxyCredentials;
            // CredentialMap wwwCredentials;  
      };      

      class AuthState
      {
         public:
            AuthState();
            bool handleChallenge(UserProfile& userProfile, const SipMessage& challenge);
            void addAuthentication(SipMessage& origRequest);
            void authSucceeded();
            
         private:
            typedef std::map<Data, RealmState> RealmStates;
            RealmStates mRealms;
            bool mFailed;
            unsigned long mCacheUseLimit;
            unsigned long mCacheUseCount;
      };

      typedef std::map<DialogSetId, AuthState> AttemptedAuthMap;
      AttemptedAuthMap mAttemptedAuths;      
};
 
}

#endif

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
