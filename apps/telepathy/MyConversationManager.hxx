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
#include <resip/recon/ConversationManager.hxx>

#include "Connection.hxx"

namespace tr
{

class Connection;

class MyConversationManager : public QObject, public recon::ConversationManager
{
   Q_OBJECT

public:

   MyConversationManager(bool localAudioEnabled, recon::ConversationManager::MediaInterfaceMode mediaInterfaceMode, int defaultSampleRate, int maxSampleRate, bool autoAnswerEnabled, Connection *connection);
   virtual ~MyConversationManager() {};

   virtual void startup();
   
   virtual recon::ConversationHandle createConversation();
   virtual recon::ParticipantHandle createRemoteParticipant(recon::ConversationHandle convHandle, resip::NameAddr& destination, recon::ConversationManager::ParticipantForkSelectMode forkSelectMode = recon::ConversationManager::ForkSelectAutomatic);
   virtual recon::ParticipantHandle createMediaResourceParticipant(recon::ConversationHandle convHandle, const resip::Uri& mediaUrl);
   virtual recon::ParticipantHandle createLocalParticipant();
   virtual void onConversationDestroyed(recon::ConversationHandle convHandle);
   virtual void onParticipantDestroyed(recon::ParticipantHandle partHandle);
   virtual void onDtmfEvent(recon::ParticipantHandle partHandle, int dtmf, int duration, bool up);
   virtual void onIncomingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, bool autoAnswer, recon::ConversationProfile& conversationProfile);
   virtual void onRequestOutgoingParticipant(recon::ParticipantHandle partHandle, const resip::SipMessage& msg, recon::ConversationProfile& conversationProfile);
   virtual void onParticipantTerminated(recon::ParticipantHandle partHandle, unsigned int statusCode);
   virtual void onParticipantProceeding(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onRelatedConversation(recon::ConversationHandle relatedConvHandle, recon::ParticipantHandle relatedPartHandle, 
                                      recon::ConversationHandle origConvHandle, recon::ParticipantHandle origPartHandle);
   virtual void onParticipantAlerting(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantConnected(recon::ParticipantHandle partHandle, const resip::SipMessage& msg);
   virtual void onParticipantRedirectSuccess(recon::ParticipantHandle partHandle);
   virtual void onParticipantRedirectFailure(recon::ParticipantHandle partHandle, unsigned int statusCode);
   virtual void onParticipantRequestedHold(recon::ParticipantHandle partHandle, bool held);
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

