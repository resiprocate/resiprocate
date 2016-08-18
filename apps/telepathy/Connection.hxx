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

#ifndef CONNECTION_HXX
#define CONNECTION_HXX

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif


#include <memory>

#include <rutil/ThreadIf.hxx>
#include <resip/recon/UserAgent.hxx>

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseChannel>

#include "TelepathyConversationProfile.hxx"
#include "TelepathyMasterProfile.hxx"
#include "MyConversationManager.hxx"
#include "MyUserAgent.hxx"
#include "MyInstantMessage.hxx"

using namespace recon;
using namespace resip;

namespace tr {

class MyConversationManager;
class MyUserAgent;
class MyInstantMessage;

class Connection : public Tp::BaseConnection
{
   Q_OBJECT
public:
   Connection(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters);

   MyConversationManager& getConversationManager() { return *myConversationManager.get(); };
   Tp::ContactAttributesMap getContactListAttributes(const QStringList &interfaces, bool hold, Tp::DBusError *error);
   void requestSubscription(const Tp::UIntList &handles, const QString &message, Tp::DBusError *error);
   void removeContacts(const Tp::UIntList &handles, Tp::DBusError *error);
   Tp::AliasMap getAliases(const Tp::UIntList& handles, Tp::DBusError *error);
   void setAliases(const Tp::AliasMap &aliases, Tp::DBusError *error);
   void getContactsFromFile(Tp::DBusError *error);
   void setContactsInFile();
   void deleteContacts(const QStringList& contacts);
   Tp::SimpleStatusSpecMap getSimpleStatusSpecMap();
   Tp::SimpleContactPresences getPresences(const Tp::UIntList &handles);
   Tp::SimplePresence getPresence(uint handle);

   tr::MyUserAgent* ua;

private:
   uint setPresence(const QString &status, const QString &message, Tp::DBusError *error);
       QStringList inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error);
   Tp::UIntList requestHandles(uint handleType, const QStringList &identifiers, Tp::DBusError *error);
   Tp::BaseChannelPtr createChannel(const QVariantMap &request, Tp::DBusError *error);
   Tp::ContactAttributesMap getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error);
   uint ensureHandle(const QString& identifier);


private slots:
   void doConnect(Tp::DBusError *error);
   void onConnected();
   void doDisconnect();
   void setStatusSlot(uint newStatus, uint reason);
   void onIncomingCall(const QString & caller, uint callHandle);

   void setContactStatus(const QString& identifier, const QString& status);
   void onMessageReceived(const resip::SipMessage& message);

private:
   resip::SharedPtr<TelepathyMasterProfile> mUAProfile;
   resip::SharedPtr<TelepathyConversationProfile> mConversationProfile;
   resip::SharedPtr<MyInstantMessage> mInstantMessage;
   std::auto_ptr<MyConversationManager> myConversationManager;

   Tp::BaseConnectionContactsInterfacePtr mContactsInterface;
   Tp::BaseConnectionAliasingInterfacePtr mAliasingInterface;
   Tp::BaseConnectionSimplePresenceInterfacePtr mSimplePresenceInterface;
   Tp::BaseConnectionContactListInterfacePtr mContactListInterface;
   Tp::BaseConnectionRequestsInterfacePtr mRequestsInterface;

   long nextHandleId;
   QMap<uint, QString> mHandles;
   QMap<QString, uint> mIdentifiers;
   Tp::AliasMap mAliases;

   Tp::SimplePresence mSelfPresence;
   Tp::SimpleStatusSpecMap statusMap;
   Tp::SimpleContactPresences mPresences;

};
}

#endif



