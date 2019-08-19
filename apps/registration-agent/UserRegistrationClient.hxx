#ifndef USERREGISTRATIONAGENT_HXX
#define USERREGISTRATIONAGENT_HXX

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/UserProfile.hxx"

#include "KeyedFile.hxx"
#include "UserAccount.hxx"

namespace registrationagent {

class UserRegistrationClient : public resip::ClientRegistrationHandler
{

public:
   UserRegistrationClient(resip::SharedPtr<KeyedFile> keyedFile);
   virtual ~UserRegistrationClient();

   void addUserAccount(const resip::Uri& aor, resip::SharedPtr<UserAccount> userAccount);
   void removeUserAccount(const resip::Uri& aor);

   void setContact(const resip::Uri& aor, const resip::Data& newContact, const time_t expires = 0, const std::vector<resip::Data>& route = std::vector<resip::Data>());
   void unSetContact(const resip::Uri& aor);

   virtual void onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
   virtual void onRemoved(resip::ClientRegistrationHandle, const resip::SipMessage& response);
   virtual void onFailure(resip::ClientRegistrationHandle, const resip::SipMessage& response);
   virtual int onRequestRetry(resip::ClientRegistrationHandle, int retrySeconds, const resip::SipMessage& response);
   virtual bool onRefreshRequired(resip::ClientRegistrationHandle, const resip::SipMessage& lastRequest);

   virtual std::size_t getAccountsTotal() const { return mKeyedFile->getLineCount(); };
   virtual std::size_t getAccountsFailed() const { return mFailedAccounts.size(); };

protected:
   resip::SharedPtr<UserAccount> userAccountForMessage(const resip::SipMessage& m);
   resip::SharedPtr<UserAccount> userAccountForAoR(const resip::Uri& aor);

private:
   resip::SharedPtr<KeyedFile> mKeyedFile;
   std::map<resip::Uri, resip::SharedPtr<UserAccount> > mAccounts;
   std::set<resip::SharedPtr<UserAccount> > mFailedAccounts;
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
