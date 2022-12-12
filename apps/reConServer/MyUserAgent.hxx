#ifndef MYUSERAGENT_HXX
#define MYUSERAGENT_HXX

#ifdef USE_SIPXTAPI
#include <os/OsIntTypes.h>
#endif

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <rutil/ConfigParse.hxx>
#include <rutil/Data.hxx>
#include <resip/stack/Dispatcher.hxx>
#include <resip/recon/UserAgent.hxx>

#include "B2BCallManager.hxx"
#include "RegistrationForwarder.hxx"
#include "SubscriptionForwarder.hxx"

#include <memory>
#include <utility>

namespace reconserver
{

class MyUserAgent : public recon::UserAgent
{
public:
   MyUserAgent(reconserver::ReConServerConfig& configParse, recon::ConversationManager* conversationManager, std::shared_ptr<recon::UserAgentMasterProfile> profile);
   virtual void onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq);
   virtual void onSubscriptionTerminated(recon::SubscriptionHandle handle, unsigned int statusCode);
   virtual void onSubscriptionNotify(recon::SubscriptionHandle handle, const resip::Data& notifyData);
   virtual std::shared_ptr<recon::ConversationProfile> getIncomingConversationProfile(const resip::SipMessage& msg);
   virtual std::shared_ptr<recon::ConversationProfile> getConversationProfileForRefer(const resip::SipMessage& msg);
   virtual void process(int timeoutMs);

   virtual void addIncomingFeature(std::shared_ptr<resip::DumFeature> f) { getDialogUsageManager().addIncomingFeature(std::move(f)); };

   virtual std::shared_ptr<resip::Dispatcher> initDispatcher(std::unique_ptr<resip::Worker> prototype,
                  int workers=2,
                  bool startImmediately=true);

protected:
   friend class MyConversationManager;

private:
   friend class B2BCallManager;

   unsigned int mMaxRegLoops;
   std::shared_ptr<RegistrationForwarder> mRegistrationForwarder;
   std::shared_ptr<SubscriptionForwarder> mSubscriptionForwarder;

   B2BCallManager *getB2BCallManager();
};

}

#endif

/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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

