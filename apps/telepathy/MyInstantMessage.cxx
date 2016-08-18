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

#include "MyInstantMessage.hxx"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/recon/ReconSubsystem.hxx>
#include <resip/dum/ClientPagerMessage.hxx>
#include <resip/dum/ServerPagerMessage.hxx>

using namespace tr;
using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

MyInstantMessage::MyInstantMessage()
{

}

void
MyInstantMessage::onMessageArrived(ServerPagerMessageHandle handle, const SipMessage& message)
{
   SharedPtr<SipMessage> ok = handle->accept();
   handle->send(ok);
   InfoLog(<<"MyInstantMessage::onMessageArrived " << message.brief());
   emit onMessageReceived(message);
}

void
MyInstantMessage::onSuccess(ClientPagerMessageHandle handle, const resip::SipMessage& status)
{
   InfoLog(<<"ClientMessageHandler::onSuccess\n");
}

void
MyInstantMessage::onFailure(ClientPagerMessageHandle handle, const SipMessage& status, auto_ptr<Contents> contents)
{
   ErrLog(<<"ClientMessageHandler::onFailure\n");
   
   ErrLog(<< "Message rcv: "  << *contents << "\n");
}
