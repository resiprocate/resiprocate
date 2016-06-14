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


#include "MyConversationManager.hxx"

#include "Connection.hxx"
#include "Common.hxx"
#include "SipCallChannel.hxx"

using namespace recon;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM ReconSubsystem::RECON

tr::Connection::Connection(const QDBusConnection &dbusConnection, const QString &cmName, const QString &protocolName, const QVariantMap &parameters)
   : Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters),
     mUAProfile(new TelepathyMasterProfile(parameters)),
     mConversationProfile(new TelepathyConversationProfile(mUAProfile, parameters)),
     ua(0),
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
   myConversationManager->buildSessionCapabilities(mConversationProfile->getDefaultAddress(), numCodecIds, codecIds, mConversationProfile->sessionCaps());
   ua->addConversationProfile(mConversationProfile);

   /* Connection.Interface.Contacts */
   mContactsInterface = Tp::BaseConnectionContactsInterface::create();
   mContactsInterface->setGetContactAttributesCallback(Tp::memFun(this, &Connection::getContactAttributes));
   mContactsInterface->setContactAttributeInterfaces(QStringList()
                                                   << TP_QT_IFACE_CONNECTION
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE
                                                   //<< TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING
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
    //mContactListInterface->setGetContactListAttributesCallback(Tp::memFun(this, &Connection::getContactListAttributes));
    //mContactListInterface->setRequestSubscriptionCallback(Tp::memFun(this, &Connection::requestSubscription));
    //mContactListInterface->setAuthorizePublicationCallback(Tp::memFun(this, &Connection::authorizePublication));
    //mContactListInterface->setRemoveContactsCallback(Tp::memFun(this, &Connection::removeContacts));
    //mContactListInterface->setUnsubscribeCallback(Tp::memFun(this, &Connection::unsubscribe));
    //mContactListInterface->setUnpublishCallback(Tp::memFun(this, &Connection::unpublish));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(mContactListInterface));

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
tr::Connection::doConnect(Tp::DBusError *error)
{
   setStatus(Tp::ConnectionStatusConnecting, Tp::ConnectionStatusReasonRequested);

   InfoLog(<<"Trying to connect");
   ua->startup();
   myConversationManager->startup();
   ua->run();

   mContactListInterface->setContactListState(Tp::ContactListStateWaiting);
}

void
tr::Connection::onConnected()
{
   setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);

   Tp::SimpleContactPresences presences;
   mSelfPresence.type = Tp::ConnectionPresenceTypeAvailable;
   mSelfPresence.status = QLatin1String("available");
   presences[selfHandle()] = mSelfPresence;
   mSimplePresenceInterface->setPresences(presences);
}

uint
tr::Connection::setPresence(const QString &status, const QString &message, Tp::DBusError *error)
{
   //FIXME
   return selfHandle();
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
   if(!mIdentifiers.contains(identifier)) {
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

   if (error.isValid() || channel.isNull()) {
       qWarning() << "error creating the channel " << error.name() << error.message();
       return;
   }
}

QStringList
tr::Connection::inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error)
{
   StackLog(<<"inspectHandles()");

   if(handleType != Tp::HandleTypeContact) {
      error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unsupported handle type"));
      return QStringList();
   }

   QStringList result;

   foreach (uint handle, handles) {
      if(!mHandles.contains(handle)) {
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

   if(handleType != Tp::HandleTypeContact) {
      ErrLog(<<"requestHandles() unsupported handleType == " << handleType);
      error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Connection::requestHandles - Handle Type unknown"));
      return result;
   }

   Q_FOREACH(const QString &identifier,  identifiers) {
      ensureHandle(identifier);
      result.append(mIdentifiers[identifier]);
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

   switch (targetHandleType) {
   case Tp::HandleTypeContact:
      if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))) {
         targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
         targetID = mHandles[targetHandle];
      } else if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"))) {
         targetID = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
         targetHandle = ensureHandle(targetID);
      }
      break;
   default:
      if (error) {
         error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unknown target handle type"));
      }
      return Tp::BaseChannelPtr();
      break;
   }

   if (targetID.isEmpty()) {
      if (error) {
         error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Target handle is empty."));
      }
      return Tp::BaseChannelPtr();
   }

   uint initiatorHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")).toUInt();

   /*Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, channelType, Tp::HandleType(targetHandleType), targetHandle);
   baseChannel->setTargetID(targetID);
   baseChannel->setInitiatorHandle(initiatorHandle); */

   ParticipantHandle participantHandle = -1 ;
   recon::ConversationHandle cHandle = 1;   // FIXME - hardcoded default value, should create new Conversation
   bool incoming = false;
   StackLog(<<"createChannel - channelType = " << channelType.toUtf8().constData() << " and contact = " << targetID.toUtf8().constData());
   if(channelType == TP_QT_IFACE_CHANNEL_TYPE_CALL) {
      cHandle = myConversationManager->createConversation();
      myConversationManager->createLocalParticipant();
      if(!request.contains("participantHandle"))
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

   SipCallChannel *channel = new SipCallChannel(incoming, this, targetID, targetHandle, cHandle, participantHandle);
   channel->baseChannel()->setInitiatorHandle(initiatorHandle);
   return channel->baseChannel();
}

Tp::ContactAttributesMap
tr::Connection::getContactAttributes(const Tp::UIntList &handles, const QStringList &ifaces, Tp::DBusError *error)
{
    StackLog(<<"getContactAttributes");
    qDebug() << "getContactAttributes" << handles << ifaces;
    Tp::ContactAttributesMap attributesMap;
    Q_FOREACH(uint handle, handles) {
        QVariantMap attributes;
        QStringList inspectedHandles = inspectHandles(Tp::HandleTypeContact, Tp::UIntList() << handle, error);
        if (inspectedHandles.size() > 0) {
            attributes[TP_QT_IFACE_CONNECTION+"/contact-id"] = inspectedHandles.at(0);
        } else {
            continue;
        }
        if (ifaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE+"/presence"] = QVariant::fromValue(mSelfPresence);
        }
        attributesMap[handle] = attributes;
    }
    return attributesMap;
}
