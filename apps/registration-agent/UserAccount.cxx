
#include <algorithm>

#include "rutil/Logger.hxx"
#include "AppSubsystem.hxx"
#include "UserAccount.hxx"
#include "UserRegistrationClient.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONAGENT

using namespace registrationagent;
using namespace resip;
using namespace std;

UserAccountFileRowHandler::UserAccountFileRowHandler(DialogUsageManager& dum)
   : mDum(dum)
{
}

SharedPtr<KeyedFileLine>
UserAccountFileRowHandler::onNewLine(SharedPtr<KeyedFile> keyedFile, const Data& key, const vector<Data>& columns)
{
   Uri aor(key);
   SharedPtr<UserAccount> userReg(new UserAccount(keyedFile, aor, columns, mDum, mUserRegistrationClient));
   mUserRegistrationClient->addUserAccount(aor, userReg);
   userReg->activate();
   return SharedPtr<KeyedFileLine>(userReg, dynamic_cast_tag());
}

void
UserAccountFileRowHandler::setUserRegistrationClient(SharedPtr<UserRegistrationClient> userRegistrationClient)
{
   mUserRegistrationClient = userRegistrationClient;
}

UserAccount::UserAccount(SharedPtr<KeyedFile> keyedFile, const Uri& aor, const vector<Data>& columns, DialogUsageManager& dum, resip::SharedPtr<UserRegistrationClient> userRegistrationClient) :
   BasicKeyedFileLine(keyedFile, aor.getAor(), columns),
   mDum(dum),
   mUserRegistrationClient(userRegistrationClient),
   mAor(aor),
   mContactOverride(false),
   mExpires(0),
   mState(UserAccount::Inactive)
{
   readColumns();
}

UserAccount::~UserAccount()
{
   if(mState != Done)
   {
      WarningLog(<<" AoR '" << mAor << "' deleted before all registrations removed");
   }
   StackLog(<<" AoR '" << mAor << "' UserAccount deleted");
}

void
UserAccount::readColumns()
{
   if(!mContactOverride)
   {
      mContact = getParam(0);
      if(!mContact.empty())
      {
         mContactUri = Uri(mContact);
      }
   }
   mSecret = paramCount() > 1 ? getParam(1) : Data::Empty;
   mAuthUser = paramCount() > 2 ? getParam(2) : Data::Empty;
   if(mAuthUser.empty())
   {
      mAuthUser = mAor.uri().user();
   }
   mExpiry = paramCount() > 3 ? getParam(3) : Data::Empty;
   mOutboundProxy = paramCount() > 4 ? getParam(4) : Data::Empty;
   mRegId = paramCount() > 5 ? getParam(5) : Data::Empty;
   mInstanceId = paramCount() > 6 ? getParam(6) : Data::Empty;

   // Build a UserProfile for this registration
   mProfile.reset(new UserProfile(mDum.getMasterUserProfile()));
   mProfile->setDefaultFrom(mAor);
   if(!mSecret.empty())
   {
      mProfile->setDigestCredential(mAor.uri().host(), mAuthUser, mSecret);
   }
   if(!mExpiry.empty())
   {
      mProfile->setDefaultRegistrationTime(mExpiry.convertInt());
   }
   if(!mOutboundProxy.empty())
   {
      const Uri outboundProxy(mOutboundProxy);
      mProfile->setOutboundProxy(outboundProxy);
   }
}

void
UserAccount::activate()
{
   if(mState == Inactive)
   {
      if(mContact.empty())
      {
         StackLog(<<"AoR '" << mAor << "' has no Contact, not activating");
         return;
      }
      mState = Active;
      doRegistration();
   }
}

void
UserAccount::deactivate()
{
   if(mState == Active)
   {
      mState = Inactive;
      removeAllActive();
   }
}

void
UserAccount::setContact(const Data& newContact, const time_t expires, const std::vector<resip::Data>& route)
{
   if(mState == Active && newContact == mContact && route == mRouteRaw && expires > mExpires)
   {
      // only the expiry changes, so no need to de-register/re-register
      mExpires = expires;
      return;
   }
   deactivate();
   mContactOverride = true;
   mContact = newContact;
   mContactUri = Uri(mContact);
   mExpires = expires;
   mRouteRaw = route;
   mRoute.clear();
   for(std::vector<resip::Data>::const_iterator it = route.begin(); it != route.end(); it++)
   {
      mRoute.push_back(NameAddr(*it));
   }
   activate();
}

void
UserAccount::unSetContact()
{
   mContactOverride = true;
   deactivate();
   mContact.clear();
}

void
UserAccount::doRegistration()
{
   SharedPtr<SipMessage> regMessage = mDum.makeRegistration(mAor, mProfile);
   NameAddr contact(mContactUri);
   if(!mRegId.empty())
   {
      contact.param(p_regid) = mRegId.convertInt();
   }
   if(!mInstanceId.empty())
   {
      contact.param(p_Instance) = mInstanceId;
   }
   regMessage->header(h_Contacts).clear();
   regMessage->header(h_Contacts).push_back(contact);
   regMessage->header(h_Routes) = mRoute;

   mDum.sendCommand(regMessage);
}

void
UserAccount::onSuccess(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog( << "ClientHandler::onSuccess: " << endl );
   if(find(mHandles.begin(), mHandles.end(), h) == mHandles.end())
   {
      mHandles.insert(mHandles.begin(), h);
   }
}

void
UserAccount::onRemoved(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog ( << "ClientHandler::onRemoved ");
   vector<ClientRegistrationHandle>::iterator it = find(mHandles.begin(), mHandles.end(), h);
   if(it != mHandles.end())
   {
      mHandles.erase(it);
   }
   if(mState == Ending && mHandles.size() == 0)
   {
      doCleanup();
   }
}

void
UserAccount::doCleanup()
{
   DebugLog(<<"doCleanup(): all handles removed");
   mState = Done;
   mUserRegistrationClient->removeUserAccount(mAor.uri());
   // Delete ourselves
   readyForDeletion();
}

void
UserAccount::onFailure(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog ( << "ClientHandler::onFailure - check the configuration.  Peer response: " << response );
}

/// From resip/dum/RegistrationHandler.hxx
/// call on Retry-After failure.
/// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
int
UserAccount::onRequestRetry(ClientRegistrationHandle h, int retrySeconds, const SipMessage& response)
{
   WarningLog ( << "ClientHandler:onRequestRetry, want to retry");
   return 30;
}

bool
UserAccount::onRefreshRequired(resip::ClientRegistrationHandle h, const resip::SipMessage& lastRequest)
{
   StackLog(<<"UserAccount::onRefreshRequired mExpires == " << mExpires);
   if(mExpires == 0)
   {
      return true;
   }
   UInt64 now = Timer::getTimeSecs();
   if(now > mExpires)
   {
      DebugLog(<<"now = " << now << " and contact expired at " << mExpires);
      if(mState == Active)
      {
         mState = Inactive;
      }
      return false;
   }
   return true;
}

void
UserAccount::removeAllActive()
{
   for(vector<ClientRegistrationHandle>::iterator it = mHandles.begin();
          it != mHandles.end(); it++)
   {
      (*it)->endCommand();
   }
}

void
UserAccount::onLineRemoved(SharedPtr<KeyedFileLine> sp)
{
   BasicKeyedFileLine::onLineRemoved(sp);
   InfoLog( << "Removing registration(s)");
   if(mHandles.size() > 0)
   {
      mState = Ending;
      removeAllActive();
   }
   else
   {
      doCleanup();
   }
}

void
UserAccount::onLineChanged()
{
   InfoLog( << "Updating registration");
   readColumns();
   if(mState == Active)
   {
      deactivate();
      activate();
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
