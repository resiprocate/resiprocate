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

#include <QCoreApplication>

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>

#include <os/OsSysLog.h>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <resip/recon/SipXHelper.hxx>


#include "Protocol.hxx"

using namespace tr;

int main(int argc, char *argv[])
{
   QCoreApplication app(argc, argv);
   app.setApplicationName(QLatin1String("telepathy-resiprocate"));

   resip::Log::initialize(resip::Log::Cout, resip::Log::Stack, "telepathy-resiprocate");
   //Log::setMaxLineCount(loggingFileMaxLineCount);

   // Setup logging for the sipX media stack
   // It is bridged to the reSIProcate logger
   recon::SipXHelper::setupLoggingBridge("telepathy-resiprocate");
   //UserAgent::setLogLevel(Log::Warning, UserAgent::SubsystemAll);
   //UserAgent::setLogLevel(Log::Info, UserAgent::SubsystemRecon);

   //initNetwork();     // needed for reSIProcate on some platforms

   Tp::registerTypes();
   Tp::enableDebug(true);
   Tp::enableWarnings(true);

   Tp::BaseProtocolPtr proto = Tp::BaseProtocol::create<Protocol>(QLatin1String("sip"));
   Tp::BaseConnectionManagerPtr cm = Tp::BaseConnectionManager::create(QLatin1String("resiprocate"));

   cm->addProtocol(proto);
   cm->registerObject();

   return app.exec();
}


