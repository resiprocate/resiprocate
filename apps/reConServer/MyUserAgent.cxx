#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>

#include <resip/stack/MessageFilterRule.hxx>
#include "AppSubsystem.hxx"

#include "B2BCallManager.hxx"
#include "MyUserAgent.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace resip;
using namespace recon;
using namespace reconserver;
using namespace std;


MyUserAgent::MyUserAgent(ReConServerConfig& configParse, ConversationManager* conversationManager, std::shared_ptr<UserAgentMasterProfile> profile) :
   UserAgent(conversationManager, std::move(profile)),
   mMaxRegLoops(1000)
{
   ReConServerConfig::Application application = configParse.getConfigApplication("Application", ReConServerConfig::None);
   if(application == ReConServerConfig::B2BUA)
   {
      mRegistrationForwarder.reset(new RegistrationForwarder(configParse, getSipStack()));
      mSubscriptionForwarder.reset(new SubscriptionForwarder(configParse, getSipStack()));
   }
}

void
MyUserAgent::onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq)
{
   InfoLog(<< "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);
}

void
MyUserAgent::onSubscriptionTerminated(SubscriptionHandle handle, unsigned int statusCode)
{
   InfoLog(<< "onSubscriptionTerminated: handle=" << handle << " statusCode=" << statusCode);
}

void
MyUserAgent::onSubscriptionNotify(SubscriptionHandle handle, const Data& notifyData)
{
   InfoLog(<< "onSubscriptionNotify: handle=" << handle << " data=" << endl << notifyData);
}

std::shared_ptr<ConversationProfile>
MyUserAgent::getIncomingConversationProfile(const resip::SipMessage& msg)
{
   B2BCallManager *b2bcm = getB2BCallManager();
   auto defaultProfile = UserAgent::getIncomingConversationProfile(msg);
   if(b2bcm)
   {
      return b2bcm->getIncomingConversationProfile(msg, std::move(defaultProfile));
   }
   return defaultProfile;
}

std::shared_ptr<ConversationProfile>
MyUserAgent::getConversationProfileForRefer(const resip::SipMessage& msg)
{
   // FIXME 2022-01-23 - this isn't called any more since the commit
   //     a5e1f059f08e47d08efe42dd4bcca64157f5f26d changed the UserAgent API
   // For the moment,
   // - we only handle a REFER from the external zone
   // - we assume the INVITE goes to the internal zone
   B2BCallManager *b2bcm = getB2BCallManager();
   if(b2bcm)
   {
      auto p = b2bcm->getExternalConversationProfile();
      // The re-INVITE needs to have a Route header to ensure it
      // is sent back to reConServer over the same interface where
      // the REFER was received.
      // This hack takes the REFER's request URI, strips the user part and
      // adds the lr parameter to create a suitable Route header.
      NameAddr loop(msg.header(h_RequestLine).uri());
      loop.uri().user() = "";
      loop.uri().param(p_lr);
      DebugLog(<<"using Route " << loop.uri() << " for re-INVITE, REFER was received on " << msg.getReceivedTransportTuple());
      NameAddrs route;
      route.push_back(loop);
      p->setServiceRoute(route);
      return p;
   }

   // fall through to superclass if not handled by B2BUA
   return UserAgent::getDefaultOutgoingConversationProfile();
}

void
MyUserAgent::process(int timeoutMs)
{
   // Keep calling process() as long as there appear to be messages
   // available from the stack
   if(mRegistrationForwarder)
   {
      for(unsigned int i = 0; i < mMaxRegLoops && mRegistrationForwarder->process() ; i++);
   }
   if(mSubscriptionForwarder)
   {
      for(unsigned int i = 0; i < mMaxRegLoops && mSubscriptionForwarder->process() ; i++);
   }

   UserAgent::process(timeoutMs);
}

std::shared_ptr<Dispatcher>
MyUserAgent::initDispatcher(std::unique_ptr<Worker> prototype,
                  int workers,
                  bool startImmediately)
{
   DebugLog(<< "initializing Dispatcher for " << workers << " worker(s)");
   return std::make_shared<Dispatcher>(std::move(prototype), &getSipStack(), workers, startImmediately);
}

B2BCallManager*
MyUserAgent::getB2BCallManager()
{
   return dynamic_cast<B2BCallManager*>(getConversationManager());
}

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

