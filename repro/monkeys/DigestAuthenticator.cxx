
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/DnsUtil.hxx"
#include "resip/stack/Message.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Auth.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#include "repro/monkeys/DigestAuthenticator.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/UserInfoMessage.hxx"
#include "repro/UserStore.hxx"
#include "repro/Dispatcher.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

DigestAuthenticator::DigestAuthenticator(ProxyConfig& config,
                                         Dispatcher* authRequestDispatcher,
                                         const Data& staticRealm) :
   Processor("DigestAuthenticator"),
   mAuthRequestDispatcher(authRequestDispatcher),
   mStaticRealm(staticRealm),
   mNoIdentityHeaders(config.getConfigBool("DisableIdentity", false)),
   mHttpHostname(config.getConfigData("HttpHostname", "")),
   mHttpPort(config.getConfigInt("HttpPort", 5080)),
   mUseAuthInt(!config.getConfigBool("DisableAuthInt", false)),
   mRejectBadNonces(config.getConfigBool("RejectBadNonces", false))
{
}

DigestAuthenticator::~DigestAuthenticator()
{
}

repro::Processor::processor_action_t
DigestAuthenticator::process(repro::RequestContext &rc)
{
   DebugLog(<< "Monkey handling request: " << *this << "; reqcontext = " << rc);
   
   Message *message = rc.getCurrentEvent();
   
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   UserInfoMessage *userInfo = dynamic_cast<UserInfoMessage*>(message);
   Proxy &proxy = rc.getProxy();
   
   if (sipMessage)
   {
      if (sipMessage->method() == ACK ||
          sipMessage->method() == BYE)
      {
         // Don't challenge ACK and BYE requests
         return Continue;
      }

      if (sipMessage->exists(h_ProxyAuthorizations))
      {
         Auths &authHeaders = sipMessage->header(h_ProxyAuthorizations);
         
         // if we find a Proxy-Authorization header for a realm we handle, 
         // asynchronously fetch the relevant userAuthInfo from the database
         for (Auths::iterator i = authHeaders.begin() ; i != authHeaders.end() ; ++i)
         {
            if (isMyRealm(rc, i->param(p_realm)))
            {
               return requestUserAuthInfo(rc, i->param(p_realm));
            }
         }
      }
      
      // if there was no Proxy-Auth header already, and the request is purportedly From
      // one of our domains, send a challenge, unless this is from a trusted node in one
      // of "our" domains (ex: from a gateway).
      //
      // Note that other monkeys can still challenge the request later if needed 
      // for other reasons (for example, the StaticRoute monkey)
      if(!sipMessage->header(h_From).isWellFormed() ||
         sipMessage->header(h_From).isAllContacts() )
      {
         InfoLog(<<"Malformed From header: cannot get realm to challenge with. Rejecting.");
         rc.sendResponse(*auto_ptr<SipMessage>
                         (Helper::makeResponse(*sipMessage, 400, "Malformed From header")));
         return SkipAllChains;         
      }
      
      if (proxy.isMyDomain(sipMessage->header(h_From).uri().host()))
      {
         if (!rc.getKeyValueStore().getBoolValue(IsTrustedNode::mFromTrustedNodeKey))
         {
            challengeRequest(rc, false);
            return SkipAllChains;
         }
      }
   }
   else if (userInfo)
   {
      // Handle response from user authentication database
      sipMessage = &rc.getOriginalRequest();
      const Data& realm = userInfo->realm();
      const Data& user = userInfo->user();
      InfoLog (<< "Received user auth info for " << user << " at realm " << realm);
      Helper::AuthResult authResult = Helper::Failed;
      switch(userInfo->getMode())
      {
         case UserAuthInfo::UserUnknown:
            authResult = Helper::Failed;
            break;

         case UserAuthInfo::RetrievedA1:
            {
               const Data& a1 = userInfo->A1();
               StackLog (<< "Received user auth info for " << user << " at realm " << realm 
                         <<  " a1 is " << a1);

               pair<Helper::AuthResult,Data> result =
                  Helper::advancedAuthenticateRequest(*sipMessage, realm, a1, 3000); // was 15
               authResult = result.first;
            }
            break;

         case UserAuthInfo::Stale:
            authResult = Helper::Expired;
            break;

         case UserAuthInfo::DigestAccepted:
            authResult = Helper::Authenticated;
            break;

         case UserAuthInfo::DigestNotAccepted:
            authResult = Helper::Failed;
            break;

         case UserAuthInfo::Error:
            authResult = Helper::Failed;
            WarningLog(<<"UserInfoMessage mode == ERROR");
            break;

         default:
            authResult = Helper::Failed;
            ErrLog(<<"Unrecognised UserInfoMessage mode value: " << userInfo->getMode());
      }

      switch (authResult)
      {
         case Helper::Failed:
            InfoLog (<< "Authentication failed for " << user << " at realm " << realm << ". Sending 403");
            rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Authentication Failed")));
            return SkipAllChains;
        
            // !abr! Eventually, this should just append a counter to
            // the nonce, and increment it on each challenge. 
            // If this count is smaller than some reasonable limit,
            // then we re-challenge; otherwise, we send a 403 instead.

         case Helper::Authenticated:
            InfoLog (<< "Authentication ok for " << user);
            
            if(!sipMessage->header(h_From).isWellFormed() ||
               sipMessage->header(h_From).isAllContacts())
            {
               InfoLog(<<"From header is malformed in"
                              " digest response.");
               rc.sendResponse(*auto_ptr<SipMessage>
                               (Helper::makeResponse(*sipMessage, 400, "Malformed From header")));
               return SkipAllChains;               
            }
            
            if (authorizedForThisIdentity(user, realm, sipMessage->header(h_From).uri()))
            {
               rc.setDigestIdentity(user);

               if(rc.getProxy().isPAssertedIdentityProcessingEnabled())
               {
                  if (sipMessage->exists(h_PPreferredIdentities))
                  {
                     // Ensure any P-AssertedIdentities present are removed (note: this is an illegal condidition)
                     sipMessage->remove(h_PAssertedIdentities);

                     // TODO - when we have a concept of multiple identities per user
                     // find the first sip or sips P-Preferred-Identity header  and the first tel
                     // bool haveSip = false;
                     // bool haveTel = false;
                     // for (;;)
                     // {
                     //    if ((i->uri().scheme() == Symbols::SIP) || (i->uri().scheme() == Symbols::SIPS))
                     //    {
                     //       if (haveSip)
                     //       {
                     //          continue;   // skip all but the first sip: or sips: URL
                     //       }
                     //       haveSip = true;
                     //
                     //       if (knownSipIdentity( user, realm, i->uri() )  // should be NameAddr?
                     //       {
                     //          sipMessage->header(h_PAssertedIdentities).push_back( i->uri() );
                     //       }
                     //       else
                     //       {
                     //          sipMessage->header(h_PAssertedIdentities).push_back(getDefaultIdentity(user, realm));
                     //       }
                     //    }
                     //    else if ((i->uri().scheme() == Symbols::TEL))
                     //    {
                     //       if (haveTel)
                     //       {
                     //          continue;  // skip all but the first tel: URL
                     //       }
                     //       haveTel = true;
                     //
                     //       if (knownTelIdentity( user, realm, i->uri() ))
                     //       {
                     //          sipMessage->header(h_PAssertedIdentities).push_back( i->uri() );
                     //       }
                     //    }
                     // }

                     // We currently don't do anything special with the P-Peferred-Identity hint - just
                     // add default identity
                     sipMessage->header(h_PAssertedIdentities).push_back(getDefaultIdentity(user, realm, sipMessage->header(h_From)));

                     // Remove the P-Preferered-Identity header
                     sipMessage->remove(h_PPreferredIdentities);
                  }
                  else
                  {
                     if (!sipMessage->exists(h_PAssertedIdentities))
                     {
                        sipMessage->header(h_PAssertedIdentities).push_back(getDefaultIdentity(user, realm, sipMessage->header(h_From)));
                     }
                     // else  TODO
                     //  - should implement guidlines in RFC5876 4.5 - whereby the proxy should remove 
                     //        ignored URI's (ie. a 2nd SIP, SIPS or TEL URI, unknown scheme)
                  }
               }            
            
#if defined(USE_SSL)
               if(!mNoIdentityHeaders)
               {
                  static Data http("http://" + mHttpHostname + ":" + Data(mHttpPort) + "/cert?domain=");
                  // .bwc. Leave pre-existing Identity headers alone.
                  if(!sipMessage->exists(h_Identity))
                  {
                     sipMessage->header(h_Identity).value() = Data::Empty;  // This is a signal to have the TransportSelector fill in the identity header
                     if(sipMessage->exists(h_IdentityInfo))
                     {
                        InfoLog(<<"Somebody sent us a"
                              " request with an Identity-Info, but no Identity"
                              " header. Removing it.");
                        if(!sipMessage->header(h_IdentityInfo).isWellFormed())
                        {
                           InfoLog(<<"...and this "
                              "Identity-Info header was malformed!");
                        }

                        sipMessage->remove(h_IdentityInfo);
                     }
                     
                     sipMessage->header(h_IdentityInfo).uri() = http + realm;
                     InfoLog (<< "Identity-Info=" << sipMessage->header(h_IdentityInfo).uri());
                  }
               }
#endif
            }
            else
            {
               // !rwm! The user is trying to forge a request.  Respond with a 403
               InfoLog (<< "User: " << user << " at realm: " << realm << 
                           " trying to forge request from: " << sipMessage->header(h_From).uri());
               rc.sendResponse(*auto_ptr<SipMessage>
                               (Helper::makeResponse(*sipMessage, 403)));
               return SkipAllChains;               
            }
            
            return Continue;

         case Helper::Expired:
            InfoLog (<< "Authentication expired for " << user);
            challengeRequest(rc, true);
            return SkipAllChains;

         case Helper::BadlyFormed:
            InfoLog (<< "Authentication nonce badly formed for " << user);
            if(mRejectBadNonces)
            {
               rc.sendResponse(*auto_ptr<SipMessage>
                            (Helper::makeResponse(*sipMessage, 403, "Where on earth did you get that nonce?")));
            }
            else
            {
               challengeRequest(rc, true);
            }
            return SkipAllChains;
      }
   }

   return Continue;
}

bool
DigestAuthenticator::authorizedForThisIdentity(const resip::Data &user, const resip::Data &realm, 
                                                resip::Uri &fromUri)
{
   // !rwm! good enough for now.  TODO eventually consult a database to see what
   // combinations of user/realm combos are authorized for an identity
   if (fromUri.host() == realm)
   {
      if ((fromUri.user() == user) || (fromUri.user() == "anonymous"))
      {
         return true;
      }
   }

   // Now try the form where the username parameter in the auth
   // header is the full fromUri, e.g.
   //    Proxy-Authorization: Digest username="user@domain" ...
   //
   if (fromUri.getAorNoPort() == user)
   {
      return true;
   }

   // catch-all: access denied
   return false;
}

NameAddr
DigestAuthenticator::getDefaultIdentity(const resip::Data &user, const resip::Data &realm, resip::NameAddr &from)
{
   NameAddr defaultIdentity;
   defaultIdentity.displayName() = from.displayName();
   defaultIdentity.uri().scheme() = from.uri().scheme();
   defaultIdentity.uri().user() = user;
   defaultIdentity.uri().host() = realm;
   return defaultIdentity;
}

void
DigestAuthenticator::challengeRequest(repro::RequestContext &rc, bool stale)
{
   SipMessage &sipMessage = rc.getOriginalRequest();
   Data realm = getRealm(rc);

   SipMessage *challenge = Helper::makeProxyChallenge(sipMessage, realm, 
                                                      mUseAuthInt /*auth-int*/, stale);
   rc.sendResponse(*challenge);
   delete challenge;
}

repro::Processor::processor_action_t
DigestAuthenticator::requestUserAuthInfo(repro::RequestContext &rc, resip::Data &realm)
{
   Message *message = rc.getCurrentEvent();
   SipMessage *sipMessage = dynamic_cast<SipMessage*>(message);
   resip_assert(sipMessage);

   // Extract the user from the appropriate Proxy-Authorization header
   Auths &authorizationHeaders = sipMessage->header(h_ProxyAuthorizations); 
   Auths::iterator i;
   Data user;

   for (i = authorizationHeaders.begin(); i != authorizationHeaders.end(); i++)
   {
      if (i->exists(p_realm) && 
          i->param(p_realm) == realm
          &&  i->exists(p_username))
      {
         user = i->param(p_username);

         InfoLog (<< "Request user auth info for "  << user
                  << " at realm " << realm);
         break;
      }
   }

   if (!user.empty())
   {
      UserInfoMessage* async = new UserInfoMessage(*this, rc.getTransactionId(), &(rc.getProxy()));
      async->user()=user;
      async->realm()=realm;
      if(sipMessage->header(h_From).isWellFormed())
      {
         async->domain()=sipMessage->header(h_From).uri().host();
      }
      else
      {
         async->domain()=realm;
      }
      Auth& auth = *i;
      return requestUserAuthInfo(rc, auth, async);
   }
   else
   {
      challengeRequest(rc, false);
      return SkipAllChains;
   }
}

Processor::processor_action_t
DigestAuthenticator::requestUserAuthInfo(RequestContext &rc, const Auth& auth, UserInfoMessage *userInfo)
{
   std::auto_ptr<ApplicationMessage> app(userInfo);
   mAuthRequestDispatcher->post(app);
   return WaitingForEvent;
}

resip::Data
DigestAuthenticator::getRealm(RequestContext &rc)
{
   if(!mStaticRealm.empty())
   {
      return mStaticRealm;
   }
   return rc.getDigestRealm();
}

bool
DigestAuthenticator::isMyRealm(RequestContext &rc, const resip::Data& realm)
{
   if(!mStaticRealm.empty())
   {
      return mStaticRealm == realm;
   }
   Proxy &proxy = rc.getProxy();
   return proxy.isMyDomain(realm);
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
