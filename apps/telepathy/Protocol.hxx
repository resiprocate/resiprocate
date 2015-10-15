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

#ifndef PROTOCOL_HXX
#define PROTOCOL_HXX

#include <TelepathyQt/BaseProtocol>

namespace tr
{

class Protocol : public Tp::BaseProtocol
{
   Q_OBJECT
   Q_DISABLE_COPY(Protocol)

public:
   Protocol(const QDBusConnection &dbusConnection, const QString &name);

private:
   Tp::BaseConnectionPtr createConnection(const QVariantMap &parameters, Tp::DBusError *error);
   QString identifyAccount(const QVariantMap &parameters, Tp::DBusError *error);
   QString normalizeContact(const QString &contactId, Tp::DBusError *error);

   /* Protocol.I.Addressing */
   QString normalizeVCardAddress(const QString &vCardField, const QString vCardAddress,
            Tp::DBusError *error);
   QString normalizeContactUri(const QString &uri, Tp::DBusError *error);

private:
   Tp::BaseProtocolAddressingInterfacePtr mAddressingInterface;

};

}

#endif




