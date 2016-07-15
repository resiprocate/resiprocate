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
#include <resip/recon/ReconSubsystem.hxx>


#include "MyUserAgent.hxx"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

tr::MyUserAgent::MyUserAgent(ConversationManager* conversationManager, SharedPtr<UserAgentMasterProfile> profile, tr::Connection& connection) :
   UserAgent(conversationManager, profile), mConnection(connection)
{
}

void
tr::MyUserAgent::onApplicationTimer(unsigned int id, unsigned int durationMs, unsigned int seq)
{
   InfoLog(<< "onApplicationTimeout: id=" << id << " dur=" << durationMs << " seq=" << seq);
}

void
tr::MyUserAgent::onSubscriptionTerminated(SubscriptionHandle handle, unsigned int statusCode)
{
   InfoLog(<< "onSubscriptionTerminated: handle=" << handle << " statusCode=" << statusCode);
}

void
tr::MyUserAgent::onSubscriptionNotify(SubscriptionHandle handle, const Data& notifyData)
{
   InfoLog(<< "onSubscriptionNotify: handle=" << handle << " data=" << endl << notifyData);
}

void
tr::MyUserAgent::onSuccess(ClientRegistrationHandle h, const SipMessage& response)
{
   InfoLog( << "ClientHandler::onSuccess: " << endl );
   QMetaObject::invokeMethod(&mConnection, "onConnected", Qt::QueuedConnection);
}

void
tr::MyUserAgent::onRemoved(ClientRegistrationHandle, const SipMessage& response)
{
   ErrLog ( << "ClientHandler::onRemoved ");
   setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNetworkError);
}

void
tr::MyUserAgent::onFailure(ClientRegistrationHandle, const SipMessage& response)
{
   ErrLog ( << "ClientHandler::onFailure: " << response );
   setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNetworkError);
}

int
tr::MyUserAgent::onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)
{
   WarningLog ( << "ClientHandler:onRequestRetry, want to retry immediately");
   return 0;
}

void
tr::MyUserAgent::thread()
{
   mStopping = false;
   while(!mStopping)
   {
      process(50);
   }
   InfoLog(<<"cleaning up thread");
   UserAgent::shutdown();
   setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonRequested);
   InfoLog(<<"thread done");
}

void
tr::MyUserAgent::stop()
{
   mStopping = true;
}

void
tr::MyUserAgent::setStatus(uint newStatus, uint reason)
{
   QMetaObject::invokeMethod(&mConnection, "setStatusSlot", Qt::QueuedConnection, Q_ARG(uint, newStatus), Q_ARG(uint, reason));
}


