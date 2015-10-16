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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/stack/SipMessage.hxx>
#include <resip/stack/Tuple.hxx>
#include <resip/recon/ReconSubsystem.hxx>

#include "SipCallChannel.hxx"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace tr;
using namespace resip;
using namespace recon;

SipCallChannel::SipCallChannel(bool incoming, Connection* connection, QString peer, uint targetHandle, ParticipantHandle participantHandle)
   : mIncoming(incoming),
     mConnection(connection),
     mPeer(peer),
     mTargetHandle(targetHandle),
     mParticipantHandle(participantHandle)
{
   mBaseChannel = Tp::BaseChannel::create(mConnection, TP_QT_IFACE_CHANNEL_TYPE_CALL, Tp::HandleTypeContact, targetHandle);
   mBaseChannel->setTargetID(peer);

   Tp::BaseChannelCallTypePtr callType = Tp::BaseChannelCallType::create(mBaseChannel.data(),
                                                                          true,
                                                                          Tp::StreamTransportTypeUnknown,
                                                                          true,
                                                                          false, "audio","");
   mBaseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(callType));

   mHoldIface = Tp::BaseChannelHoldInterface::create();
   mHoldIface->setSetHoldStateCallback(Tp::memFun(this,&SipCallChannel::onHoldStateChanged));

   mMuteIface = Tp::BaseCallMuteInterface::create();
   mMuteIface->setSetMuteStateCallback(Tp::memFun(this,&SipCallChannel::onMuteStateChanged));

   mCallChannel = Tp::BaseChannelCallTypePtr::dynamicCast(mBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_CALL));
   mCallChannel->setHangupCallback(Tp::memFun(this,&SipCallChannel::onHangup));
   mCallChannel->setAcceptCallback(Tp::memFun(this,&SipCallChannel::onAccept));

   QObject::connect(this, SIGNAL(hangupComplete(bool)), this, SLOT(onHangupComplete(bool)));
   QObject::connect(this, SIGNAL(answerComplete(bool)), this, SLOT(onAnswerComplete(bool)));
   QTimer::singleShot(0, this, SLOT(init()));
}

void
SipCallChannel::init()
{
   InfoLog(<<"SipCallChannel::init for participant " << mParticipantHandle);
   mObjPath = mBaseChannel->objectPath();
   Tp::CallMemberMap memberFlags;
   Tp::HandleIdentifierMap identifiers;
   QVariantMap stateDetails;
   Tp::CallStateReason reason;

   identifiers[mTargetHandle] = mPeer;
   reason.actor =  0;
   reason.reason = Tp::CallStateChangeReasonProgressMade;
   reason.message = "";
   reason.DBusReason = "";
   if (mIncoming) {
      memberFlags[mTargetHandle] = 0;
   } else {
      memberFlags[mTargetHandle] = Tp::CallMemberFlagRinging;
   }

   mCallChannel->setCallState(Tp::CallStateInitialising, 0, reason, stateDetails);

   mCallContent = Tp::BaseCallContent::create(baseChannel()->dbusConnection(), baseChannel().data(), "audio", Tp::MediaStreamTypeAudio, Tp::MediaStreamDirectionNone);
   mCallChannel->addContent(mCallContent);

   mCallChannel->setMembersFlags(memberFlags, identifiers, Tp::UIntList(), reason);

   mCallChannel->setCallState(Tp::CallStateInitialised, 0, reason, stateDetails);
}

Tp::BaseChannelPtr
SipCallChannel::baseChannel()
{
   return mBaseChannel;
}

void
SipCallChannel::onHangupComplete(bool status)
{
   if (!status) {
      InfoLog(<<"onHangupComplete, status = false");
   }
}

void
SipCallChannel::onAnswerComplete(bool status)
{
   if (!status) {
      InfoLog(<<"onAnswerComplete, status = false");
   }
}

void
SipCallChannel::onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError* error)
{
   mConnection->getConversationManager().destroyParticipant(mParticipantHandle);
}

void
SipCallChannel::onAccept(Tp::DBusError*)
{
   QVariantMap stateDetails;
   Tp::CallStateReason reason;
   reason.actor =  0;
   reason.reason = Tp::CallStateChangeReasonUserRequested;
   reason.message = "";
   reason.DBusReason = "";
   mCallChannel->setCallState(Tp::CallStateAccepted, 0, reason, stateDetails);

   mConnection->getConversationManager().answerParticipant(mParticipantHandle);
}

void
SipCallChannel::onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error)
{
   StackLog(<<"onHoldStateChanged");
}

void
SipCallChannel::onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error)
{
   StackLog(<<"onMuteStateChanged");
}

void
SipCallChannel::setCallState(const QString &state)
{
   StackLog(<<"setCallState");
}
