

#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "rutil/Data.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/SharedPtr.hxx"

#include "DialerConfiguration.hxx"
#include "DialInstance.hxx"
#include "MyInviteSessionHandler.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

#define REFER_TIMEOUT 10

DialInstance::DialInstance(const DialerConfiguration& dialerConfiguration, const resip::Uri& targetUri) :
   mDialerConfiguration(dialerConfiguration),
   mTargetUri(targetUri),
   mResult(Error)
{
}

DialInstance::DialResult DialInstance::execute()
{

   prepareAddress();

   Security* security = 0;

   Data certPath(mDialerConfiguration.getCertPath());
   if(certPath.size() == 0)
   {
      certPath = getenv("HOME");
      certPath += "/.sipdial/certs";
   }
   security = new Security(certPath);

   if(mDialerConfiguration.getCADirectory().size() > 0)
      security->addCADirectory(mDialerConfiguration.getCADirectory());

   mSipStack = new SipStack(security);
   mDum = new DialogUsageManager(*mSipStack);
   //mDum->addTransport(UDP, 5067, V4);
   mDum->addTransport(TLS, 5067, V4);
   SharedPtr<MasterProfile> masterProfile = SharedPtr<MasterProfile>(new MasterProfile);
   mDum->setMasterProfile(masterProfile);
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager);
   mDum->setClientAuthManager(clientAuth);
   MyInviteSessionHandler *ish = new MyInviteSessionHandler(*this);
   mDum->setInviteSessionHandler(ish);

   sendInvite();

   while(mSipStack != 0) 
   {
      FdSet fdset;
      mSipStack->buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(resipMin((int)mSipStack->getTimeTillNextProcessMS(), 50));
      if(err == -1) {
         if(errno != EINTR) {
            //B2BUA_LOG_ERR("fdset.select returned error code %d", err);
            resip_assert(0);  // FIXME
         }
      }
      // Process all SIP stack activity
      mSipStack->process(fdset);
      while(mDum->process());

      // FIXME - we should wait a little and make sure it really worked
      if(mProgress == ReferSent)
      {
         time_t now;
         time(&now);
         if(mReferSentTime + REFER_TIMEOUT < now)
         {
            ErrLog(<< "REFER timeout");
            mProgress = Done;
         }
      }

      if(mProgress == Connected && mClient->isConnected()) 
      {
         InfoLog(<< "Sending the REFER");
         mClient->refer(NameAddr(mFullTarget));
         InfoLog(<< "Done sending the REFER");
         mProgress = ReferSent;
         time(&mReferSentTime);
      }
      
      if(mProgress == Done)
      {
         delete mDum;
         delete ish;
         delete mSipStack;
         mSipStack = 0;
      }
   }

   return mResult;

}

void DialInstance::prepareAddress() 
{
   if(mTargetUri.scheme() == Symbols::Sip ||
      mTargetUri.scheme() == Symbols::Sips) {
      mFullTarget = mTargetUri;
      return;
   }

   if(mTargetUri.scheme() == Symbols::Tel) {
      Data num = processNumber(mTargetUri.user());
      if(num.size() < 1)
      {
         // FIXME - check size
         resip_assert(0);
      }
      if(num[0] == '+')
      {
         // E.164
         if(mDialerConfiguration.getTargetPrefix().size() > 0)
            mFullTarget = Uri("sip:" + mDialerConfiguration.getTargetPrefix() + num.substr(1, num.size() - 1) + "@" + mDialerConfiguration.getTargetDomain());
         else
            mFullTarget = Uri("sip:" + num + "@" + mDialerConfiguration.getTargetDomain());
         return;
      }
      mFullTarget = Uri("sip:" + num + "@" + mDialerConfiguration.getTargetDomain());
      return;
   }

   // FIXME Unsupported scheme 
   resip_assert(0);
}

void DialInstance::sendInvite() 
{
   SharedPtr<UserProfile> outboundUserProfile(mDum->getMasterUserProfile());
   outboundUserProfile->setDefaultFrom(mDialerConfiguration.getDialerIdentity());
   outboundUserProfile->setDigestCredential(mDialerConfiguration.getAuthRealm(), mDialerConfiguration.getAuthUser(), mDialerConfiguration.getAuthPassword());
   SharedPtr<SipMessage> msg = mDum->makeInviteSession(NameAddr(mDialerConfiguration.getCallerUserAgentAddress()), outboundUserProfile, 0);
   HeaderFieldValue *hfv = 0;
   switch(mDialerConfiguration.getCallerUserAgentVariety())
   {
   case DialerConfiguration::Generic:
      break;
   case DialerConfiguration::LinksysSPA941:
      hfv = new HeaderFieldValue("\\;answer-after=0", 16);
      msg->header(h_CallInfos).push_back(GenericUri(*hfv, Headers::CallInfo));
      break;
   case DialerConfiguration::AlertInfo:
      hfv = new HeaderFieldValue("AA", 2);
      msg->header(h_AlertInfos).push_back(GenericUri(*hfv, Headers::AlertInfo));
      break;
   case DialerConfiguration::Cisco7940:
      break;
   default:
      break;
   }
   mDum->send(msg);
   if(hfv != 0)
      delete hfv;
}

// Get rid of punctuation like `.' and `-'
// Keep a leading `+' if present
// assert if not a real number
Data DialInstance::processNumber(const Data& verboseNumber)
{
   Data num = Data("");
   int len = verboseNumber.size();
   for(int i = 0; i < len; i++)
   {
      char c = verboseNumber[i];
      switch(c)
      {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         num.append(&c, 1);
         break;
      case '+':
         resip_assert(i == 0);   // FIXME - better error handling needed
         num.append(&c, 1);
         break;
      case '.':
      case '-':
         // just ignore those characters
         break;
      default:
         // any other character is garbage
         resip_assert(0);
      }
   }
   return num;
}

void DialInstance::onFailure()
{
   mResult = ReferUnsuccessful;
   mProgress = Done;
}

void DialInstance::onConnected(ClientInviteSessionHandle cis) 
{
   mClient = cis;
   mProgress = Connected;
}

void DialInstance::onReferSuccess()
{
   InfoLog(<< "Refer was successful");
   mResult = ReferSuccessful;
   mProgress = Done;
}

void DialInstance::onReferFailed()
{
   ErrLog(<< "Refer failed");
   mResult = ReferUnsuccessful;
   mProgress = Done;
}

void DialInstance::onTerminated()
{
   InfoLog(<< "onTerminated()");
   mProgress = Done;
}

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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

