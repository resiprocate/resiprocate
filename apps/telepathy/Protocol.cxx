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

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/Types>
#include <TelepathyQt/ProtocolParameterList>
#include <TelepathyQt/Utils>

#include "Protocol.hxx"
#include "TelepathyParameters.hxx"
#include "Connection.hxx"
#include "Common.hxx"

using namespace tr;

tr::Protocol::Protocol(const QDBusConnection &dbusConnection, const QString &name)
   : Tp::BaseProtocol(dbusConnection, name)
{
   setRequestableChannelClasses(Tp::RequestableChannelClassSpecList() << Tp::RequestableChannelClassSpec::textChat() << Tp::RequestableChannelClassSpec::audioCall());
   setEnglishName(QLatin1String("SIP"));
   setIconName(QLatin1String("im-sip"));
   setVCardField(QLatin1String("x-sip"));

   setCreateConnectionCallback(Tp::memFun(this, &Protocol::createConnection));
   setIdentifyAccountCallback(Tp::memFun(this, &Protocol::identifyAccount));
   setNormalizeContactCallback(Tp::memFun(this, &Protocol::normalizeContact));

   setParameters(TelepathyParameters::getParameterList());

   mAddressingInterface = Tp::BaseProtocolAddressingInterface::create();
   mAddressingInterface->setAddressableVCardFields(QStringList() << QLatin1String("x-sip"));
   mAddressingInterface->setAddressableUriSchemes(QStringList() << QLatin1String("sip"));
   mAddressingInterface->setNormalizeVCardAddressCallback(Tp::memFun(this, &Protocol::normalizeVCardAddress));
   mAddressingInterface->setNormalizeContactUriCallback(Tp::memFun(this, &Protocol::normalizeContactUri));
   plugInterface(Tp::AbstractProtocolInterfacePtr::dynamicCast(mAddressingInterface));
}

Tp::BaseConnectionPtr tr::Protocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error)
{
   Tp::BaseConnectionPtr newConnection = Tp::BaseConnection::create<tr::Connection>(QLatin1String("resiprocate"), name(), parameters);

   return newConnection;
}

QString tr::Protocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error)
{
   return Tp::escapeAsIdentifier(parameters[QLatin1String("account")].toString());
}

QString tr::Protocol::normalizeContact(const QString &contactId, Tp::DBusError *error)
{
   // FIXME
   return contactId;
}

QString tr::Protocol::normalizeVCardAddress(const QString &vcardField, const QString vcardAddress,
        Tp::DBusError *error)
{
   // FIXME
   error->set(QLatin1String("NormalizeVCardAddress.Error.Test"), QLatin1String(""));
   return QString();
}

QString tr::Protocol::normalizeContactUri(const QString &uri, Tp::DBusError *error)
{
   // FIXME
   error->set(QLatin1String("NormalizeContactUri.Error.Test"), QLatin1String(""));
   return QString();
}


