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

#include "TelepathyParameters.hxx"

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

using namespace tr;
using namespace resip;

Tp::ProtocolParameterList
TelepathyParameters::getParameterList()
{
   Tp::ProtocolParameterList params;

   params << Tp::ProtocolParameter(QLatin1String("alias"), QDBusSignature(QLatin1String("s")), Tp::ConnMgrParamFlagRequired)
          << Tp::ProtocolParameter(QLatin1String("account"), QDBusSignature(QLatin1String("s")), Tp::ConnMgrParamFlagRequired)
          << Tp::ProtocolParameter(QLatin1String("password"), QDBusSignature(QLatin1String("s")), Tp::ConnMgrParamFlagSecret | Tp::ConnMgrParamFlagRequired)
          << Tp::ProtocolParameter(QLatin1String("turn-server"), QDBusSignature(QLatin1String("s")), 0)
          //<< Tp::ProtocolParameter(QLatin1String("turn-port"), QDBusSignature(QLatin1String("q")), 0)
          << Tp::ProtocolParameter(QLatin1String("root-cert-path"), QDBusSignature(QLatin1String("s")), 0, "/etc/ssl/certs")  // FIXME - differs on some platforms
       ;

   return params;
}

TelepathyParameters::TelepathyParameters(const QVariantMap &parameters)
   : mParameters(parameters)
{
   mAccount = getString("account");

   mContact.uri() = Uri("sip:" + mAccount);
   mContact.displayName() = getString("alias");

   mRealm = mContact.uri().host();
   // FIXME: support username only too
   mAuthUser = mContact.uri().user(); //this is for using test.sip5060.net server
   // mAuthUser = mContact.uri().getAorNoPort();
   mPassword = getString("password");
}

const char*
TelepathyParameters::getString(const char* parameter)
{
   return (const char *)mParameters.value(QLatin1String(parameter)).toString().toUtf8().constData();
}



