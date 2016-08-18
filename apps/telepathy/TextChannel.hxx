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

#ifndef TEXTCHANNEL_HXX
#define TEXTCHANNEL_HXX

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <TelepathyQt/BaseChannel>

#include "Connection.hxx"

using namespace std;
using namespace resip;

namespace tr
{

class TextChannel;
class Connection;
   
typedef Tp::SharedPtr<TextChannel> TextChannelPtr;

class TextChannel : public Tp::BaseChannelTextType
{
   Q_OBJECT
public:
   static TextChannelPtr create(tr::MyUserAgent* userAgent, resip::SharedPtr<MyInstantMessage> instantMessage, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID);
   void processReceivedMessage(const resip::SipMessage &message, uint senderHandle, const QString& senderIdentifier);
   
protected:
   TextChannel(tr::MyUserAgent* userAgent, resip::SharedPtr<MyInstantMessage> instantMessage, Tp::BaseChannel *baseChannel, uint selfHandle, QString selfID);
   QString sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error);
   void messageAcknowledged(const QString &messageId);

protected:
   Tp::BaseChannelMessagesInterfacePtr mMessagesInterface;
   Tp::BaseChannelChatStateInterfacePtr mChatStateInterface;
   
   uint mTargetHandle;
   QString mTargetID;
   uint mSelfHandle;
   QString mSelfID;

private:
   tr::MyUserAgent* ua;
   resip::SharedPtr<MyInstantMessage> mInstantMessage;
};
   
}

#endif
