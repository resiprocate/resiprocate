

#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/SharedPtr.hxx"

#include "DialerConfiguration.hxx"
#include "DialInstance.hxx"
#include "MyInviteSessionHandler.hxx"

using namespace resip;
using namespace std;

DialInstance::DialInstance(const DialerConfiguration& dialerConfiguration, const resip::Uri& targetUri) :
   mDialerConfiguration(dialerConfiguration),
   mTargetUri(targetUri),
   mResult(Error)
{
}

DialInstance::DialResult DialInstance::execute()
{
  
   prepareAddress();

   mSipStack = new SipStack();
   mDum = new DialogUsageManager(*mSipStack);
   mDum->addTransport(UDP, 5067, V4);
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
            assert(0);  // FIXME
         }
      }
      // Process all SIP stack activity
      mSipStack->process(fdset);
      while(mDum->process());
      if(mProgress == Connected && mClient->isConnected()) 
      {
         mClient->refer(NameAddr(mFullTarget));
         mProgress = ReferSent;
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
   if(mTargetUri.scheme() == Symbols::Sip) {
      mFullTarget = mTargetUri;
      return;
   }

   if(mTargetUri.scheme() == Symbols::Tel) {
      Data num = processNumber(mTargetUri.user());
      if(num.size() < 1)
      {
         // FIXME - check size
         assert(0);
      }
      if(num[0] == '+')
      {
         // E.164
         mFullTarget = Uri("sip:" + mDialerConfiguration.getTargetPrefix() + num.substr(1, num.size() - 1) + "@" + mDialerConfiguration.getTargetDomain());
         return;
      }
      mFullTarget = Uri("sip:" + num + "@" + mDialerConfiguration.getTargetDomain());
      return;
   }

   // FIXME Unsupported scheme 
   assert(0);
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
      msg->header(h_CallInfos).push_back(GenericUri(hfv, Headers::CallInfo));
      break;
   case DialerConfiguration::PolycomIP501:
      hfv = new HeaderFieldValue("AA", 2);
      msg->header(h_AlertInfos).push_back(GenericUri(hfv, Headers::AlertInfo));
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
         assert(i == 0);   // FIXME - better error handling needed
         num.append(&c, 1);
         break;
      case '.':
      case '-':
         // just ignore those characters
         break;
      default:
         // any other character is garbage
         assert(0);
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
   mResult = ReferSuccessful;
   mProgress = Done;
}

void DialInstance::onReferFailed()
{
   mResult = ReferUnsuccessful;
   mProgress = Done;
}

void DialInstance::onTerminated()
{
   mProgress = Done;
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

