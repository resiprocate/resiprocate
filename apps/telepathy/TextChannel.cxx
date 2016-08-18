/*
 * Copyright (C) 2015 Mateus Bellomo https://mateusbellomo.wordpress.com/
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

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"

#include "TextChannel.hxx"
#include "Connection.hxx"

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

typedef Tp::SharedPtr<tr::TextChannel> TextChannelPtr;

tr::TextChannel::TextChannel(tr::MyUserAgent* userAgent, SharedPtr<MyInstantMessage> instantMessage, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID)
   : Tp::BaseChannelTextType(baseChannel),
     mTargetHandle(baseChannel->targetHandle()),
     mTargetID(baseChannel->targetID()),
     mSelfHandle(selfHandle),
     mSelfID(selfID),
     ua(userAgent),
     mInstantMessage(instantMessage)
{
   QStringList supportedContentTypes = QStringList() << QLatin1String("text/plain");
   Tp::UIntList messageTypes = Tp::UIntList() << Tp::ChannelTextMessageTypeNormal
					      << Tp::ChannelTextMessageTypeDeliveryReport;

   uint messagePartSupportFlags = Tp::MessageSendingFlagReportDelivery | Tp::MessageSendingFlagReportRead;
   uint deliveryReportingSupport = Tp::DeliveryReportingSupportFlagReceiveSuccesses | Tp::DeliveryReportingSupportFlagReceiveRead;

   setMessageAcknowledgedCallback(Tp::memFun(this, &TextChannel::messageAcknowledged));

   mMessagesInterface = Tp::BaseChannelMessagesInterface::create(this,
								 supportedContentTypes,
								 messageTypes,
								 messagePartSupportFlags,
								 deliveryReportingSupport);

   baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mMessagesInterface));
   mMessagesInterface->setSendMessageCallback(Tp::memFun(this, &TextChannel::sendMessage));

   mChatStateInterface = Tp::BaseChannelChatStateInterface::create();
   // mChatStateInterface->setSetChatStateCallback(Tp::memFun(this, &TextChannel::setChatState));
   baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mChatStateInterface));
}

TextChannelPtr
tr::TextChannel::create(tr::MyUserAgent* userAgent, SharedPtr<MyInstantMessage> instantMessage, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID)
{
   return tr::TextChannelPtr(new tr::TextChannel(userAgent, instantMessage, baseChannel, selfHandle, selfID));
}

QString
tr::TextChannel::sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error)
{
   QString content;
   foreach ( const Tp::MessagePart& part, messageParts )
   {
      if ( part.count(QLatin1String("content-type")) &&
	   part.value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain") &&
	   part.count(QLatin1String("content")) )
      {
	 content = part.value(QLatin1String("content")).variant().toString();
	 break;
      }
   }
   Data messageBody(content.toUtf8().data());

   string to = mTargetID.toUtf8().constData();
   to = "sip:" + to;
   NameAddr naTo(to.c_str());

   const char* callId = ua->sendMessage(naTo, messageBody, Mime("text", "plain"));

   return QString(callId);
}

void
tr::TextChannel::processReceivedMessage(const resip::SipMessage& message, uint senderHandle, const QString& senderID)
{
   Tp::MessagePartList body;
   Tp::MessagePart text;
   text[QLatin1String("content-type")] = QDBusVariant(QLatin1String("text/plain"));

   QString content = message.getContents()->getBodyData().c_str();
   text[QLatin1String("content")] = QDBusVariant(content);
   body << text;
   qDebug() << "TextChannel::processReceivedMessage() senderHandle = " << senderHandle << " senderID = " << senderID;
   
   Tp::MessagePartList partList;
   Tp::MessagePart header;

   const QString token = message.header(h_CallId).value().c_str();
   header[QLatin1String("message-token")] = QDBusVariant(token);
   header[QLatin1String("message-type")] = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
   header[QLatin1String("message-sent")] = QDBusVariant(message.getCreatedTimeMicroSec());

   uint currentTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000;
   header[QLatin1String("message-received")] = QDBusVariant(currentTimestamp);
   header[QLatin1String("message-sender")] = QDBusVariant(senderHandle);
   header[QLatin1String("message-sender-id")] = QDBusVariant(senderID);
   
   partList << header << body;
   addReceivedMessage(partList);
}

void
tr::TextChannel::messageAcknowledged(const QString &messageId)
{
   qDebug() << "TextChannel::messageAcknowledged() not implemented" << endl;
}
