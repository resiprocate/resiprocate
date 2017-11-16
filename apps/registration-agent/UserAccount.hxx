#ifndef USERACCOUNT_HXX
#define USERACCOUNT_HXX

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/UserProfile.hxx"

#include "KeyedFile.hxx"

namespace registrationagent {

class UserRegistrationClient;

class UserAccountFileRowHandler : public KeyedFileRowHandler
{
public:
   UserAccountFileRowHandler(resip::DialogUsageManager& dum);
   virtual ~UserAccountFileRowHandler() {};
   void setUserRegistrationClient(resip::SharedPtr<UserRegistrationClient> userRegistrationClient);

   virtual resip::SharedPtr<KeyedFileLine> onNewLine(resip::SharedPtr<KeyedFile> keyedFile, const resip::Data& key, const std::vector<resip::Data>& columns);

private:
   resip::DialogUsageManager& mDum;
   resip::SharedPtr<UserRegistrationClient> mUserRegistrationClient;
};

class UserAccount : public BasicKeyedFileLine
{

public:
   UserAccount(resip::SharedPtr<KeyedFile> keyedFile, const resip::Uri& aor, const std::vector<resip::Data>& columns, resip::DialogUsageManager& dum, resip::SharedPtr<UserRegistrationClient> userRegistrationClient);
   virtual ~UserAccount();

   void activate();
   void deactivate();

   void setContact(const resip::Data& newContact, const time_t expires = 0, const std::vector<resip::Data>& route = std::vector<resip::Data>());
   void unSetContact();

   virtual void onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
   virtual void onRemoved(resip::ClientRegistrationHandle, const resip::SipMessage& response);
   virtual void onFailure(resip::ClientRegistrationHandle, const resip::SipMessage& response);
   virtual int onRequestRetry(resip::ClientRegistrationHandle, int retrySeconds, const resip::SipMessage& response);
   virtual bool onRefreshRequired(resip::ClientRegistrationHandle, const resip::SipMessage& lastRequest);

   virtual void onLineRemoved(resip::SharedPtr<KeyedFileLine> sp);
   virtual void onLineChanged();

private:
   void readColumns();
   void doRegistration();
   void removeAllActive();
   void doCleanup();

   resip::DialogUsageManager& mDum;
   resip::SharedPtr<UserRegistrationClient> mUserRegistrationClient;
   resip::NameAddr mAor;
   bool mContactOverride;   // when set, a changed Contact in the file is ignored
                            // and it can only be changed through set/unset methods
   time_t mExpires;         // if 0, keep refreshing
   resip::Data mContact;
   resip::Uri mContactUri;
   resip::Data mSecret;
   resip::Data mAuthUser;
   resip::Data mExpiry;
   resip::Data mOutboundProxy;
   resip::Data mRegId;
   resip::Data mInstanceId;

   typedef enum {
      Inactive,      // initialized, not yet asked to register
      Active,        // should be registered or trying to register
      Ending,        // try to remove all registrations
      Done           // all registrations ended
   } State;
   State mState;

   resip::SharedPtr<resip::UserProfile> mProfile;
   std::vector<resip::Data> mRouteRaw;
   resip::NameAddrs mRoute;

   std::vector<resip::ClientRegistrationHandle> mHandles;
};

} // namespace

#endif

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */
