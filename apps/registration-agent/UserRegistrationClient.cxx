
#include "rutil/Logger.hxx"
#include "AppSubsystem.hxx"
#include "UserRegistrationClient.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONAGENT

using namespace registrationagent;
using namespace resip;
using namespace std;

UserRegistrationClient::UserRegistrationClient(resip::SharedPtr<KeyedFile> keyedFile) :
   mKeyedFile(keyedFile)
{
}

UserRegistrationClient::~UserRegistrationClient()
{
}

void
UserRegistrationClient::addUserAccount(const Uri& aor, SharedPtr<UserAccount> userAccount)
{
   mAccounts[aor] = userAccount;
}

void
UserRegistrationClient::removeUserAccount(const Uri& aor)
{
   StackLog(<<"Removing UserAccount " << aor);
   mAccounts.erase(aor);
}

void
UserRegistrationClient::setContact(const Uri& aor, const Data& newContact, const time_t expires, const vector<Data>& route)
{
   SharedPtr<UserAccount> userAccount = userAccountForAoR(aor);
   if(userAccount.get())
   {
      userAccount->setContact(newContact, expires, route);
   }
}

void
UserRegistrationClient::unSetContact(const Uri& aor)
{
   SharedPtr<UserAccount> userAccount = userAccountForAoR(aor);
   if(userAccount.get())
   {
      userAccount->unSetContact();
   }
}

void
UserRegistrationClient::onSuccess(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog( << "ClientHandler::onSuccess: " << endl );
   SharedPtr<UserAccount> userAccount = userAccountForMessage(response);
   if(userAccount.get())
   {
      mFailedAccounts.erase(userAccount);
      userAccount->onSuccess(h, response);
   }
}

void
UserRegistrationClient::onRemoved(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog ( << "ClientHandler::onRemoved ");
   SharedPtr<UserAccount> userAccount = userAccountForMessage(response);
   if(userAccount.get())
   {
      mFailedAccounts.erase(userAccount);
      userAccount->onRemoved(h, response);
   }
}

void
UserRegistrationClient::onFailure(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog ( << "ClientHandler::onFailure - check the configuration.  Peer response: " << response );
   SharedPtr<UserAccount> userAccount = userAccountForMessage(response);
   if(userAccount.get())
   {
      mFailedAccounts.insert(userAccount);
      userAccount->onFailure(h, response);
   }
}

/// From resip/dum/RegistrationHandler.hxx
/// call on Retry-After failure.
/// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
int
UserRegistrationClient::onRequestRetry(ClientRegistrationHandle h, int retrySeconds, const SipMessage& response)
{
   SharedPtr<UserAccount> userAccount = userAccountForMessage(response);
   if(userAccount.get())
   {
      mFailedAccounts.insert(userAccount);
      return userAccount->onRequestRetry(h, retrySeconds, response);
   }
   return 30;
}

bool
UserRegistrationClient::onRefreshRequired(ClientRegistrationHandle h, const resip::SipMessage& lastRequest)
{
   SharedPtr<UserAccount> userAccount = userAccountForMessage(lastRequest);
   if(userAccount.get())
   {
      return userAccount->onRefreshRequired(h, lastRequest);
   }
   return true;
}

SharedPtr<UserAccount>
UserRegistrationClient::userAccountForMessage(const resip::SipMessage& m)
{
   Uri aor = m.header(h_To).uri();
   return userAccountForAoR(aor);
}

SharedPtr<UserAccount>
UserRegistrationClient::userAccountForAoR(const Uri& aor)
{
   map<Uri, SharedPtr<UserAccount> >::iterator it = mAccounts.find(aor);
   if(it != mAccounts.end())
   {
      return it->second;
   }
   else
   {
      WarningLog(<<"couldn't find UserAccount for " << aor);
      return SharedPtr<UserAccount>();
   }
}

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
