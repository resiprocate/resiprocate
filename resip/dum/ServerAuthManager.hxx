#if !defined(RESIP_SERVERAUTHMANAGER_HXX)
#define RESIP_SERVERAUTHMANAGER_HXX

#include <map>

#include "resip/stack/Auth.hxx"
#include "resip/stack/SipMessage.hxx"
#include "DumFeature.hxx"

namespace resip
{
class UserAuthInfo;
class DialogUsageManager;


class ServerAuthManager : public DumFeature
{
   public:
      enum Result
      {
         //Authorized,
         RequestedInfo,
         RequestedCredentials,
         Challenged,
         Skipped,
         Rejected
      };

      ServerAuthManager(DialogUsageManager& dum, TargetCommand::Target& target, bool challengeThirdParties = true, const resip::Data& staticRealm = "");
      virtual ~ServerAuthManager();

      virtual ProcessingResult process(Message* msg);      
      
      // can return Authorized, Rejected or Skipped
      //Result handleUserAuthInfo(Message* msg);

      // returns the SipMessage that was authorized if succeeded or returns 0 if
      // rejected. 
      virtual SipMessage* handleUserAuthInfo(UserAuthInfo* auth);

      // can return Challenged, RequestedCredentials, Rejected, Skipped
      virtual Result handle(SipMessage* sipMsg);
      
   protected:

      enum AsyncBool
      {
           True,  // response is true
           False, // response is false
           Async  // response will be sent asynchronously
      };

      enum AuthFailureReason
      {
         InvalidRequest,   // some aspect of the request (e.g. nonce)
                           // is not valid/tampered with
         BadCredentials,   // credentials didn't match
         Error             // processing/network error
      };

      // this call back should async cause a post of UserAuthInfo
      virtual void requestCredential(const Data& user, 
                                     const Data& realm, 
                                     const SipMessage& msg,
                                     const Auth& auth, // the auth line we have chosen to authenticate against
                                     const Data& transactionToken ) = 0;
      
      virtual bool useAuthInt() const;
      virtual bool proxyAuthenticationMode() const;
      virtual bool rejectBadNonces() const;
      
      typedef std::map<Data, SipMessage*> MessageMap;
      MessageMap mMessages;

      /// should return true if the request must be challenged
      /// The default is to challenge all requests - override this class to change this beviour
      virtual AsyncBool requiresChallenge(const SipMessage& msg);

      /// should return true if the passed in user is authorized for the provided uri
      virtual bool authorizedForThisIdentity(const resip::Data &user, 
                                             const resip::Data &realm, 
                                             resip::Uri &fromUri);

      /// returns the realm to be used for the challenge
      virtual const Data& getChallengeRealm(const SipMessage& msg);   

      /// should return true if realm passed in is ours and we can challenge
      virtual bool isMyRealm(const Data& realm);

      // Either
      //  a) issues a challenge if necessary and returns `Challenged'
      //  b) returns `Skipped' if no challenge necessary
      //  c) waits asynchronously to find out if challenge required,
      //      and returns `RequestedInfo'
      Result issueChallengeIfRequired(SipMessage *sipMsg);

      // sends a 407 challenge to the UAC who sent sipMsg
      void issueChallenge(SipMessage *sipMsg);

      virtual void onAuthSuccess(const SipMessage& msg);
      virtual void onAuthFailure(AuthFailureReason reason, const SipMessage& msg);

      bool mChallengeThirdParties;
      resip::Data mStaticRealm;
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
