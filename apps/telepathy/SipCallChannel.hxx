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

#ifndef SipCallChannel_hxx
#define SipCallChannel_hxx

#include <QObject>
#include <QString>

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/BaseCall>
#include <TelepathyQt/Types>

#include <resip/recon/ConversationManager.hxx>

#include "Connection.hxx"

namespace tr
{

class Connection;

class SipCallChannel : public QObject
{
   Q_OBJECT
public:
   SipCallChannel(bool incoming, Connection* connection, QString peer, uint targetHandle, recon::ParticipantHandle participantHandle);
   Tp::BaseChannelPtr baseChannel();

   void onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError* error);
   void onAccept(Tp::DBusError*);
   void onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error);
   void onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error);

private Q_SLOTS:
   void setCallState(const QString &state);
   void init();
   void onAnswerComplete(bool success);
   void onHangupComplete(bool success);


private:
   bool mIncoming;
   Connection* mConnection;
   QString mPeer;
   uint mTargetHandle;
   recon::ParticipantHandle mParticipantHandle;
   QString mObjPath;
   Tp::BaseChannelPtr mBaseChannel;
   Tp::BaseChannelCallTypePtr mCallChannel;
   Tp::BaseChannelHoldInterfacePtr mHoldIface;
   Tp::BaseCallMuteInterfacePtr mMuteIface;
   Tp::BaseCallContentPtr mCallContent;

};

}

#endif


