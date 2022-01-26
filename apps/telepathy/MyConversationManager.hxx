/*
 * Copyright (C) 2015 Daniel Pocock http://danielpocock.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MYCONVERSATIONMANAGER_HXX
#define MYCONVERSATIONMANAGER_HXX

#include <QObject>

#include <os/OsIntTypes.h>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <rutil/Data.hxx>
#include <resip/recon/SipXConversationManager.hxx>

#include "Connection.hxx"

namespace tr
{

class Connection;

class MyConversationManager : public QObject, public recon::SipXConversationManager
{
   Q_OBJECT

public:

   MyConversationManager(bool localAudioEnabled, recon::SipXConversationManager::MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled, Connection *connection);
   virtual ~MyConversationManager() {};

   virtual void startup();
   
   virtual recon::ConversationHandle createConversation(AutoHoldMode autoHoldMode = AutoHoldEnabled) override;
   virtual recon::ParticipantHandle createRemoteParticipant(recon::ConversationHandle convHandle, const resip::NameAddr& destination, recon::ConversationManager::ParticipantForkSelectMode forkSelectMode = recon::ConversationManager::ForkSelectAutomatic) override;
   virtual recon::ParticipantHandle createMediaResourceParticipant(recon::ConversationHandle convHandle, const resip::Uri& mediaUrl) override;
   virtual recon::ParticipantHandle createLocalParticipant() override;
   virtual void onConversationDestroyed(recon::ConversationHandle convHandle) override;
   virtual void onParticipantDestroyed(recon::ParticipantHandle partHandle) override;
   virtual void onDtmfEvent(recon::ParticipantHandle partHandle, int dtmf, int duration, bool up) override;
   virtual void onIncomingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, recon::ConversationProfile& conversationProfile) override;
   virtual void onRequestOutgoingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, recon::ConversationProfile& conversationProfile) override;
   virtual void onParticipantTerminated(recon::ParticipantHandle partHandle, unsigned int statusCode) override;
   virtual void onParticipantProceeding(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onRelatedConversation(recon::ConversationHandle relatedConvHandle, recon::ParticipantHandle relatedPartHandle, 
                                      recon::ConversationHandle origConvHandle, recon::ParticipantHandle origPartHandle) override;
   virtual void onParticipantAlerting(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onParticipantConnected(recon::ParticipantHandle partHandle, const resip::SipMessage& msg) override;
   virtual void onParticipantRedirectSuccess(recon::ParticipantHandle partHandle) override;
   virtual void onParticipantRedirectFailure(recon::ParticipantHandle partHandle, unsigned int statusCode) override;
   virtual void onParticipantRequestedHold(recon::ParticipantHandle partHandle, bool held) override;
   virtual void displayInfo();

protected:
   std::list<recon::ConversationHandle> mConversationHandles;
   std::list<recon::ParticipantHandle> mLocalParticipantHandles;
   std::list<recon::ParticipantHandle> mRemoteParticipantHandles;
   std::list<recon::ParticipantHandle> mMediaParticipantHandles;
   bool mLocalAudioEnabled;
   bool mAutoAnswerEnabled;
   Connection *mConnection;
};

}

#endif

