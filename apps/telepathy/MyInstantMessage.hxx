/*
 * Copyright (C) 2016 Mateus Bellomo https://mateusbellomo.wordpress.com/
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

#ifndef MYINSTANTMESSAGE_HXX
#define MYINSTANTMESSAGE_HXX

#include <QObject>

#include <os/OsIntTypes.h>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <rutil/Data.hxx>
#include <resip/recon/InstantMessage.hxx>

#include "Connection.hxx"

namespace tr
{

class Connection;

class MyInstantMessage : public QObject, public recon::InstantMessage
{
   Q_OBJECT

public:
   MyInstantMessage();
   virtual ~MyInstantMessage() {};

   virtual void onMessageArrived(resip::ServerPagerMessageHandle handle, const resip::SipMessage& message);
   virtual void onSuccess(resip::ClientPagerMessageHandle handle, const resip::SipMessage& status);
   virtual void onFailure(resip::ClientPagerMessageHandle handle, const resip::SipMessage& status, std::auto_ptr<resip::Contents> contents);

signals:
   void onMessageReceived(const resip::SipMessage& message);
};

}

#endif

