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

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/BaseException.hxx>

#include <resip/recon/UserAgent.hxx>
#include <resip/recon/ReconSubsystem.hxx>
#include "resip/stack/Pidf.hxx"
#include "resip/stack/GenericPidfContents.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "rutil/Random.hxx"

#include "MyConversationManager.hxx"

#include "Connection.hxx"
#include "Common.hxx"
#include "SipCallChannel.hxx"

#define CRLF "\r\n"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

static const QString c_fileWithContacts = QLatin1String("data.txt");

tr::Connection::Connection(const QDBusConnection &dbusConnection, const QString &cmName, const QString &protocolName, const QVariantMap &parameters)
   : Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters),
     mUAProfile(new TelepathyMasterProfile(parameters)),
     mConversationProfile(new TelepathyConversationProfile(mUAProfile, parameters)),
     ua(0),
     mProfile(new TelepathyMasterProfile(parameters)),
     nextHandleId(1)
{
   std::vector<unsigned int> _codecIds;
   _codecIds.push_back(SdpCodec::SDP_CODEC_G722);           // 9 - G.722
   _codecIds.push_back(SdpCodec::SDP_CODEC_OPUS);           // Opus
   _codecIds.push_back(SdpCodec::SDP_CODEC_PCMU);           // 0 - pcmu
   _codecIds.push_back(SdpCodec::SDP_CODEC_PCMA);           // 8 - pcma
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX);          // 96 - speex NB 8,000bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX_15);       // 98 - speex NB 15,000bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX_24);       // 99 - speex NB 24,600bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_L16_44100_MONO); // PCM 16 bit/sample 44100 samples/sec.
   _codecIds.push_back(SdpCodec::SDP_CODEC_ILBC);           // 108 - iLBC
   _codecIds.push_back(SdpCodec::SDP_CODEC_ILBC_20MS);      // 109 - Internet Low Bit Rate Codec, 20ms (RFC3951)
   _codecIds.push_back(SdpCodec::SDP_CODEC_SPEEX_5);        // 97 - speex NB 5,950bps
   _codecIds.push_back(SdpCodec::SDP_CODEC_GSM);            // 3 - GSM
   _codecIds.push_back(SdpCodec::SDP_CODEC_TONES);          // 110 - telephone-event
   unsigned int *codecIds = &_codecIds[0];
   unsigned int numCodecIds = _codecIds.size();

   //////////////////////////////////////////////////////////////////////////////
   // Create ConverationManager and UserAgent
   //////////////////////////////////////////////////////////////////////////////
   bool localAudioEnabled = true;
   ConversationManager::MediaInterfaceMode mediaInterfaceMode = ConversationManager::sipXGlobalMediaInterfaceMode;
   unsigned int defaultSampleRate = 16000;
   unsigned int maximumSampleRate = 16000;
   bool autoAnswerEnabled = false;
   myConversationManager.reset(new MyConversationManager(localAudioEnabled, mediaInterfaceMode, defaultSampleRate, maximumSampleRate, autoAnswerEnabled, this));
   ua = new MyUserAgent(myConversationManager.get(), mUAProfile, *this);
   InfoLog(<< "mbellomo Connection() ua = " << ua);
   myConversationManager->buildSessionCapabilities(mConversationProfile->getDefaultAddress(), numCodecIds, codecIds, mConversationProfile->sessionCaps());
   ua->addConversationProfile(mConversationProfile);

   /* Connection.Interface.Contacts */
   mContactsInterface = Tp::BaseConnectionContactsInterface::create();
   mContactsInterface->setGetContactAttributesCallback(Tp::memFun(this, &Connection::getContactAttributes));
   mContactsInterface->setContactAttributeInterfaces(QStringList()
						     << TP_QT_IFACE_CONNECTION
						     << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST
						     << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE
						     << TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING
						     << TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS
						     //<< TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS
      );
   plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(mContactsInterface));

   /* Connection.Interface.SimplePresence */
   mSimplePresenceInterface = Tp::BaseConnectionSimplePresenceInterface::create();
   mSimplePresenceInterface->setStatuses(tr::Common::getSimpleStatusSpecMap());
   mSimplePresenceInterface->setSetPresenceCallback(Tp::memFun(this, &Connection::setPresence));
   plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(mSimplePresenceInterface));

   /* Connection.Interface.ContactList */
   mContactListInterface = Tp::BaseConnectionContactListInterface::create();
   mContactListInterface->setContactListPersists(true);
   mContactListInterface->setCanChangeContactList(true);
   mContactListInterface->setDownloadAtConnection(true);
   mContactListInterface->setGetContactListAttributesCallback(Tp::memFun(this, &Connection::getContactListAttributes));
   mContactListInterface->setRequestSubscriptionCallback(Tp::memFun(this, &Connection::requestSubscription));
   // mContactListInterface->setAuthorizePublicationCallback(Tp::memFun(this, &Connection::authorizePublication));
   mContactListInterface->setRemoveContactsCallback(Tp::memFun(this, &Connection::removeContacts));
   //mContactListInterface->setUnsubscribeCallback(Tp::memFun(this, &Connection::unsubscribe));
   //mContactListInterface->setUnpublishCallback(Tp::memFun(this, &Connection::unpublish));
   plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(mContactListInterface));

   /* Connection.Interface.Aliasing */
   mAliasingInterface = Tp::BaseConnectionAliasingInterface::create();
   mAliasingInterface->setGetAliasesCallback(Tp::memFun(this, &Connection::getAliases));
   mAliasingInterface->setSetAliasesCallback(Tp::memFun(this, &Connection::setAliases));
   plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(mAliasingInterface));
    
   /* Connection.Interface.Requests */
   mRequestsInterface = Tp::BaseConnectionRequestsInterface::create(this);
   /* Fill requestableChannelClasses */
   Tp::RequestableChannelClass text;
   text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
   text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeContact;
   text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
   text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));

   Tp::RequestableChannelClass call;
   call.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_CALL;
   call.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeContact;
   call.fixedProperties[TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialAudio"]  = true;
   //call.fixedProperties[TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialVideo"]  = false;
   call.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
   call.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));
   call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialAudio");
   //call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialVideo");
   call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialAudioName");
   //call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialVideoName");
   call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".InitialTransport");
   call.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL+".HardwareStreaming");
   //call.allowedProperties.append(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE + QLatin1String(".InitialChannels"));

   mRequestsInterface->requestableChannelClasses << text << call;
   plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(mRequestsInterface));

   //setSelfContact(m_uniqueHandleMap[myJid], myJid);
   const QString& account = parameters.value(QLatin1String("account")).toString();
   uint _self = ensureHandle(account);
   setSelfContact(_self, account);

   setConnectCallback(Tp::memFun(this, &Connection::doConnect));
   setInspectHandlesCallback(Tp::memFun(this, &Connection::inspectHandles));
   setCreateChannelCallback(Tp::memFun(this, &Connection::createChannel));
   setRequestHandlesCallback(Tp::memFun(this, &Connection::requestHandles));
   connect(this, SIGNAL(disconnected()), SLOT(doDisconnect()));
}

void
tr::Connection::getContactsFromFile(Tp::DBusError *error)
{
   if (error->isValid())
   {
      return;
   }

   QFile file(c_fileWithContacts);

   if ( file.open(QFile::ReadOnly) )
   {
      QTextStream in(&file);
      QString line = in.readLine();
      Tp::AliasMap aliases;
      while ( !line.isNull() )
      {
	 QString identifier = line.split(" ").at(0);
	 uint handle = ensureHandle(identifier);
	 aliases[handle] = line.split(" ").at(1);

	 string strIdentifier = identifier.toUtf8().constData();
	 string uri = "sip:" + strIdentifier;
	 InfoLog(<< "mbellomo getContactsFromFile() ua = "<< ua);
	 ua->createSubscription(Data("presence"), NameAddr(uri.c_str()), 3600, Mime("application", "pidf+xml"));
	 
	 line = in.readLine();
      }
      setAliases(aliases, error);
   }
   
   else
   {
      file.open(QFile::WriteOnly);
   }

   mContactListInterface->setContactListState(Tp::ContactListStateSuccess);
}

void
tr::Connection::setContactsInFile()
{
   QFile file(c_fileWithContacts);
   
   if( file.open(QIODevice::ReadWrite | QIODevice::Truncate) )
   {
      QTextStream stream(&file);
      QMap<uint, QString>::iterator it;
      for ( it = mHandles.begin(); it != mHandles.end(); it++ )
      {
	 if ( it.key() != selfHandle() )
	 {
	    stream << it.value() << " " << mAliases[it.key()] << endl;
	 }
      }
      file.close();
   }
   
   else
   {
      ErrLog(<<"couldn't write contacts to file" << endl);
   }
}

Tp::AliasMap
tr::Connection::getAliases(const Tp::UIntList& handles, Tp::DBusError *error)
{
   if ( error->isValid() )
   {
      return Tp::AliasMap();
   }

   qDebug() << Q_FUNC_INFO << handles;

   Tp::AliasMap aliases;
   Q_FOREACH ( uint handle, handles )
   {
      if ( mAliases.find(handle) == mAliases.end() )
      {
	 error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Invalid handle(s)"));
      }
      aliases[handle] = mAliases[handle];
   }

   return aliases;
}

void
tr::Connection::setAliases(const Tp::AliasMap &aliases, Tp::DBusError *error)
{
   if ( error->isValid() )
   {
      return;
   }
   
   qDebug() << Q_FUNC_INFO << aliases;
   
   for ( Tp::AliasMap::const_iterator it = aliases.begin(); it != aliases.end(); it++ )
   {
      mAliases[it.key()] = it.value();
   }
   
}

void
tr::Connection::deleteContacts(const QStringList& contacts)
{

   Tp::ContactSubscriptionMap changes;
   Tp::HandleIdentifierMap identifiers;
   Tp::HandleIdentifierMap removals;
   Q_FOREACH ( const QString &contact, contacts )
   {
      uint id = mIdentifiers[contact];
      removals[id] = contact;
   }
   mContactListInterface->contactsChangedWithID(changes, identifiers, removals);

   setContactsInFile();
}

Tp::ContactAttributesMap
tr::Connection::getContactListAttributes(const QStringList &interfaces, bool hold, Tp::DBusError *error)
{
   Tp::UIntList handles = mHandles.keys();
   handles.removeOne(selfHandle());

   StackLog(<<"getContactListAttributes()");
   qDebug() << handles;

   return getContactAttributes(handles, interfaces, error);
}

void
tr::Connection::requestSubscription(const Tp::UIntList &handles, const QString &message, Tp::DBusError *error)
{

   QStringList contacts = inspectHandles(Tp::HandleTypeContact, handles, error);

   if ( error->isValid() )
   {
      return;
   }

   if ( contacts.isEmpty() )
   {
      error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Invalid handle(s)"));
   }

   Tp::ContactSubscriptionMap changes;
   Tp::HandleIdentifierMap identifiers;
   Tp::HandleIdentifierMap removals;
   Q_FOREACH ( const QString &contact, contacts )
   {
      uint handle = ensureHandle(contact);

      Tp::ContactSubscriptions change;
      change.publish = Tp::SubscriptionStateYes;
      change.publishRequest = QString();
      change.subscribe = Tp::SubscriptionStateUnknown;

      changes[handle] = change;
      identifiers[handle] = contact;
   }
   mContactListInterface->contactsChangedWithID(changes, identifiers, removals);
    
   setContactsInFile();
}

void
tr::Connection::removeContacts(const Tp::UIntList &handles, Tp::DBusError *error)
{
   QStringList contacts = inspectHandles(Tp::HandleTypeContact, handles, error);

   if ( error->isValid() )
   {
      return;
   }

   if ( contacts.isEmpty() )
   {
      error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Invalid handle(s)"));
   }

   deleteContacts(contacts);
}

void
tr::Connection::doConnect(Tp::DBusError *error)
{
   setStatus(Tp::ConnectionStatusConnecting, Tp::ConnectionStatusReasonRequested);

   InfoLog(<<"Trying to connect");
   ua->startup();
   myConversationManager->startup();
   ua->run();
   statusMap = tr::Common::getSimpleStatusSpecMap();

   mContactListInterface->setContactListState(Tp::ContactListStateWaiting);

   getContactsFromFile(error);
}

void
tr::Connection::onConnected()
{
   setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);

   Tp::SimpleContactPresences presences;
   Tp::DBusError error;
   setPresence(QLatin1String("available"), QLatin1String(""), &error);

   presences[selfHandle()] = mSelfPresence;
   // TODO: get presence from contacts
   mSimplePresenceInterface->setPresences(presences);
}

class ClientPubHandler : public ClientPublicationHandler {
public:
   ClientPubHandler() {}
   virtual void onSuccess(ClientPublicationHandle cph, const SipMessage& status)
   {
      handle = cph;
      InfoLog(<<"ClientPubHandler::onSuccess\n");
   }
   virtual void onRemove(ClientPublicationHandle cph, const SipMessage& status)
   {
      InfoLog(<<"ClientPubHandler::onRemove\n");
      handle = ClientPublicationHandle();
   }
   virtual int onRequestRetry(ClientPublicationHandle cph, int retrySeconds, const SipMessage& status)
   {
      handle = cph;
      InfoLog(<<"ClientPubHandler::onRequestRetry\n");
      return 30;
   }
   virtual void onFailure(ClientPublicationHandle cph, const SipMessage& status)
   {
      InfoLog(<<"ClientPubHandler::onFailure\n");
      handle = ClientPublicationHandle();
   }
   ClientPublicationHandle handle;
};


void
tr::Connection::sendPresence(const QString &status)
{

   // bool first = true;
   // string aor(argv[1]);
   // string user(argv[2]);
   // string passwd(argv[3]);
   // string realm(argv[4]);
   int port = 5060;

   Data eventName("presence");
   
   // sip logic
   // SharedPtr<MasterProfile> profile(new MasterProfile);   
   auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager());   

   SipStack clientStack;
   DialogUsageManager clientDum(clientStack);
   clientDum.addTransport(UDP, port);
   clientDum.setMasterProfile(mProfile);

   clientDum.setClientAuthManager(clientAuth);
   clientDum.getMasterProfile()->addSupportedMethod(PUBLISH);
   clientDum.getMasterProfile()->addSupportedMimeType(PUBLISH,Pidf::getStaticType());

   ClientPubHandler cph;
   clientDum.addClientPublicationHandler(eventName,&cph);

   /////
   // NameAddr naAor(aor.c_str());
   string aor = mHandles[selfHandle()].toUtf8().constData();
   aor = "sip:" + aor;
   NameAddr naAor(aor.c_str());
   // profile->setDefaultFrom(naAor);
   // profile->setDigestCredential(realm.c_str(), user.c_str(), passwd.c_str());
   
   Pidf pidf;
   // pidf.setSimpleStatus(true);
   // pidf.setEntity(naAor.uri());
   // pidf.setSimpleId(Random::getRandomHex(3));

   Pidf::Tuple tuple;
   tuple.status = true;
   // tuple.id = "test id";
   tuple.id = Random::getRandomHex(3);
   tuple.contact = Data::from(aor);
   tuple.contactPriority = (int)0;
   // tuple.note = "Away";
   tuple.note = status.toUtf8().constData();
   tuple.attributes["displayname"] = "displayName";
   tuple.attributes["status"] = "1";
   pidf.getTuples().push_back(tuple);
   InfoLog( << "Generated tuple: " << endl << tuple );


   // Data txt(
   //    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" CRLF
   //    "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" " CRLF
   //    "          xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" " CRLF
   //    "          xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" " CRLF
   //    "        entity=\"sip:mateus1@ws.sip5060.net\">" CRLF
   //    "  <dm:person id=\"p719\">" CRLF
   //    "    <rpid:activities/>" CRLF
   //    "  </dm:person>" CRLF
   //    "  <tuple id=\"t8642\">" CRLF
   //    "    <status>" CRLF
   //    "      <basic>open</basic>" CRLF
   //    "    </status>" CRLF
   //    "    <contact>sip:mateus1@ws.sip5060.net</contact>" CRLF
   //    "    <note>Online</note>" CRLF
   //    "  </tuple>" CRLF
   //    "</presence>" CRLF
   //    );
   // HeaderFieldValue hfv(txt.data(), txt.size());
   // GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType()); 

   {
      SharedPtr<SipMessage> pubMessage = clientDum.makePublication(naAor, mProfile, pidf, eventName, 120);
      InfoLog( << "Generated publish: " << endl << *pubMessage );
      clientDum.send( pubMessage );
   }

}

uint
tr::Connection::setPresence(const QString &status, const QString &message, Tp::DBusError *error)
{
   // TODO: find out why message is always getting here empty

   StackLog(<<"setPresence()");

   mSelfPresence.type = statusMap[status].type;
   mSelfPresence.status = status;
   if ( statusMap[status].canHaveMessage )
   {
      mSelfPresence.statusMessage = message;
   }

   qDebug() << "status = " << mSelfPresence.status << " type = " << mSelfPresence.type << " message = " << message;
   sendPresence(status);

   return selfHandle();
}

Tp::SimpleContactPresences
tr::Connection::getPresences(const Tp::UIntList &handles)
{
   StackLog(<<"getPresences()");

   Tp::SimpleContactPresences presences;
   Q_FOREACH ( uint handle, handles )
   {
      presences[handle] = getPresence(handle);
   }

   return presences;
}

Tp::SimplePresence
tr::Connection::getPresence(uint handle)
{
   StackLog(<<"getPresence()");
   if ( !mPresences.contains(handle) )
   {
      return Tp::SimplePresence();
   }
   qDebug() << "presence = " << mPresences.value(handle).status;

   return mPresences.value(handle);
}

void
tr::Connection::doDisconnect()
{
   InfoLog(<<"doDisconnect()");
   ua->stop();
   setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonRequested);
}

void
tr::Connection::setStatusSlot(uint newStatus, uint reason)
{
   InfoLog(<<"setStatusSlot");
   setStatus(newStatus, reason);
}

uint
tr::Connection::ensureHandle(const QString& identifier)
{
   if ( !mIdentifiers.contains(identifier) )
   {
      long id = nextHandleId++;
      mHandles[id] = identifier;
      mIdentifiers[identifier] = id;
   }
   return mIdentifiers[identifier];
}

void
tr::Connection::onIncomingCall(const QString & caller, uint callHandle)
{
   InfoLog(<<"onIncomingCall: " << caller.toUtf8().constData());

   uint handle = ensureHandle("sip:" + caller);
   uint initiatorHandle = handle;

   QVariantMap request;
   request[TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")] = false;
   request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_CALL;
   request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")] = Tp::HandleTypeContact;
   request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")] = handle;
   request[TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")] = initiatorHandle;
   request["participantHandle"] = callHandle;

   bool yours;
   Tp::DBusError error;
   Tp::BaseChannelPtr channel = ensureChannel(request, yours, false, &error);

   if ( error.isValid() || channel.isNull() )
   {
      qWarning() << "error creating the channel " << error.name() << error.message();
      return;
   }
}

QStringList
tr::Connection::inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error)
{
   StackLog(<<"inspectHandles()");

   if ( handleType != Tp::HandleTypeContact )
   {
      error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unsupported handle type"));
      return QStringList();
   }

   QStringList result;

   Q_FOREACH ( uint handle, handles )
   {
      if ( !mHandles.contains(handle) )
      {
	 return QStringList();
      }

      result.append(mHandles.value(handle));
   }
   return result;
}

Tp::UIntList
tr::Connection::requestHandles(uint handleType, const QStringList &identifiers, Tp::DBusError *error)
{
   DebugLog(<<"requestHandles() ");
   Tp::UIntList result;

   if ( handleType != Tp::HandleTypeContact )
   {
      ErrLog(<<"requestHandles() unsupported handleType == " << handleType);
      error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Connection::requestHandles - Handle Type unknown"));
      return result;
   }

   Q_FOREACH ( const QString &identifier,  identifiers )
   {
      result.append(ensureHandle(identifier));
   }

   return result;
}

Tp::BaseChannelPtr
tr::Connection::createChannel(const QVariantMap &request, Tp::DBusError *error)
{
   StackLog(<<"createChannel");
   const QString channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();

   uint targetHandleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
   uint targetHandle = 0;
   QString targetID;

   switch ( targetHandleType )
   {
      case Tp::HandleTypeContact:
      {
	 if ( request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")) )
	 {
            targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
            targetID = mHandles[targetHandle];
	 }

	 else if ( request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")) )
	 {
            targetID = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
            targetHandle = ensureHandle(targetID);
	 }
	 break;
      }
      
      default:
      {
	 if ( error )
	 {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unknown target handle type"));
	 }
	 return Tp::BaseChannelPtr();
	 break;
      }
   }

   if ( targetID.isEmpty() )
   {
      if ( error )
      {
	 error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Target handle is empty."));
      }
      return Tp::BaseChannelPtr();
   }

   uint initiatorHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")).toUInt();

   /*Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, channelType, Tp::HandleType(targetHandleType), targetHandle);
     baseChannel->setTargetID(targetID);
     baseChannel->setInitiatorHandle(initiatorHandle); */

   ParticipantHandle participantHandle = -1;
   bool incoming = false;
   StackLog(<<"createChannel - channelType = " << channelType.toUtf8().constData() << " and contact = " << targetID.toUtf8().constData());
   if ( channelType == TP_QT_IFACE_CHANNEL_TYPE_CALL )
   {
      recon::ConversationHandle cHandle = 1;   // FIXME - hardcoded default value, should create new Conversation
      //recon::ConversationHandle cHandle = myConversationManager->createConversation();
      //myConversationManager->createLocalParticipant();
      if ( !request.contains("participantHandle") )
      {
	 // Outgoing call
	 DebugLog(<<"outoing call");
	 NameAddr callee(targetID.toUtf8().constData());
	 participantHandle = myConversationManager->createRemoteParticipant(cHandle, callee);
	 myConversationManager->addParticipant(cHandle, participantHandle);
      }
      
      else
      {
	 // Incoming call
	 DebugLog(<<"incoming call");
	 incoming = true;
	 participantHandle = (ParticipantHandle)request["participantHandle"].toUInt();
      }
   }

   //return baseChannel;

   SipCallChannel *channel = new SipCallChannel(incoming, this, targetID, targetHandle, participantHandle);
   channel->baseChannel()->setInitiatorHandle(initiatorHandle);
   return channel->baseChannel();
}

Tp::ContactAttributesMap
tr::Connection::getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error)
{
   StackLog(<<"getContactAttributes");
   Tp::ContactAttributesMap attributesMap;
   Q_FOREACH ( uint handle, handles )
   {
      QVariantMap attributes;
      QStringList inspectedHandle = inspectHandles(Tp::HandleTypeContact, Tp::UIntList() << handle, error);
      if ( inspectedHandle.size() > 0 )
      {
	 attributes[TP_QT_IFACE_CONNECTION+"/contact-id"] = inspectedHandle.at(0);
      }

      else
      {
	 continue;
      }
      qDebug() << "handle = " << handle << " inspectedHandle = " << inspectedHandle;

      if ( ifaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE) )
      {
	 attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE+"/presence"] = QVariant::fromValue(getPresence(handle));
      }

      if ( ifaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING) )
      {
	 attributes[TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING + QLatin1String("/alias")] = QVariant::fromValue(mAliases[handle]);
      }

      attributesMap[handle] = attributes;
   }

   return attributesMap;
}
