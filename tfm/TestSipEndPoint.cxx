#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cppunit/TestCase.h"

#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/CpimContents.hxx" // vk
#include "resip/stack/Pidf.hxx" // vk
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/SipFrag.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TcpTransport.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/TlsTransport.hxx"
#endif // USE_SSL

#include "resip/stack/UdpTransport.hxx"
#include "resip/stack/UnknownHeaderType.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/DnsUtil.hxx"

#include "tfm/DialogSet.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/Resolver.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SequenceSet.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/SipRawMessage.hxx"
#include "tfm/TestSipEndPoint.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/TestUser.hxx"

#include <boost/version.hpp>

using namespace resip;
using namespace boost;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

boost::shared_ptr<resip::SipMessage> TestSipEndPoint::nil;
resip::Uri TestSipEndPoint::NoOutboundProxy;

TestSipEndPoint::IdentityMessageConditioner TestSipEndPoint::identity;
TestSipEndPoint::IdentityRawConditioner TestSipEndPoint::raw_identity;

TestSipEndPoint::TestSipEndPoint(const Uri& addressOfRecord,
                                 const Uri& contactUrl,
                                 const Uri& outboundProxy,
                                 bool hasStack,
                                 const Data& interfaceObj,
                                 Security* security)
   : mAor(addressOfRecord),
     mContact(contactUrl),
     mOutboundProxy(outboundProxy),
     mTransport(0),
     mSecurity(security)
{
#ifdef RTP_ON
   mRtpSession = 0;
#endif
   
   nwIf=interfaceObj;
   
   if(nwIf.empty())
   {
      nwIf=mContact.uri().host();
   }
   
   if (hasStack)
   {
      resip::IpVersion version=resip::V4;
      
      if(resip::DnsUtil::isIpV6Address(interfaceObj))
      {
#ifdef USE_IPV6
         version=resip::V6;
#else
         return;
#endif
      }
      else if(resip::DnsUtil::isIpV4Address(interfaceObj))
      {
         version=resip::V4;
      }

      if (!mContact.uri().exists(p_transport) ||
          (isEqualNoCase(mContact.uri().param(p_transport), Tuple::toData(UDP))))
      {
         InfoLog(<< "TestSipEndPoint[" << addressOfRecord << "]transport is UDP " << nwIf);
         mTransport = new UdpTransport(mIncoming, mContact.uri().port(), version, StunDisabled, nwIf,0,resip::Compression::Disabled);
      }
      else if (isEqualNoCase(mContact.uri().param(p_transport), Tuple::toData(TCP)))
      {
         InfoLog(<< "TestSipEndPoint[" << addressOfRecord << "]transport is TCP " << nwIf);
         mTransport = new TcpTransport(mIncoming, mContact.uri().port(), version, nwIf, 0, resip::Compression::Disabled, 0);
      }
      else if (isEqualNoCase(mContact.uri().param(p_transport), Tuple::toData(SCTP)))
      {
         InfoLog(<< "TestSipEndPoint[" << addressOfRecord << "]transport is SCTP  INADDR_ANY ("<< nwIf <<" will go in Vias)");
         // .bwc. For SCTP, attach to INADDR_ANY, so we can test multihoming.
         // Also recall that we've added a bool param that switches the behavior 
         // of TcpTransport to instead use SCTP.
         mTransport = new TcpTransport(mIncoming, mContact.uri().port(), version, "0.0.0.0", 0, resip::Compression::Disabled, 0);
      }
#ifdef USE_SSL
      else if (isEqualNoCase(mContact.uri().param(p_transport), Tuple::toData(TLS)))
      {
         InfoLog(<< "TestSipEndPoint[" << addressOfRecord << "]transport is TLS " << nwIf);
         mTransport = new TlsTransport(mIncoming, mContact.uri().port(), version, nwIf, 
                                       *mSecurity, resip::Data::Empty, SecurityTypes::TLSv1, 0, resip::Compression::Disabled, 0);
      }
#endif
      else
      {
         resip_assert(0);
      }

      resip_assert(mTransport);
      registerWithTransportDriver();
   }
   else
   {
      InfoLog (<< "Creating sip endpoint with no transport: " << addressOfRecord);
   }
   mContactSet.insert(mContact);
}

TestSipEndPoint::TestSipEndPoint(const Uri& contactUrl,
                                 const Uri& outboundProxy,
                                 bool hasStack,
                                 const Data& interfaceObj,
                                 Security* security)
   : mAor(contactUrl),
     mContact(contactUrl),
     mOutboundProxy(outboundProxy),
     mTransport(0),
     mSecurity(security)
{
#ifdef RTP_ON
   mRtpSession = 0;
#endif

   DebugLog(<< "TestSipEndPoint::TestSipEndPoint contact: " << mContact);
   if (hasStack)
   {
      resip::IpVersion version = (DnsUtil::isIpV6Address(interfaceObj) ? V6 : V4);

      if (!contactUrl.exists(p_transport) ||
          (contactUrl.param(p_transport) == Tuple::toData(UDP)))
      {
         //CerrLog(<< "transport is UDP " << interfaceObj);
         mTransport = new UdpTransport(mIncoming, mContact.uri().port(), version, StunDisabled, interfaceObj);
      }
      else if (contactUrl.param(p_transport) == Tuple::toData(TCP))
      {
         //CerrLog(<< "transport is TCP " << interfaceObj);
         mTransport = new TcpTransport(mIncoming, mContact.uri().port(), version, interfaceObj);
      }
/*
      else if (contactUrl.param(p_transport) == Tuple::toData(Transport::TLS))
      {
         mTransport = new TlsTransport(interfaceObj, mContact.uri().port(), "", mIncoming);
      }
*/

      resip_assert(mTransport);
      registerWithTransportDriver();
   }
   mContactSet.insert(mContact);
}

TestSipEndPoint::~TestSipEndPoint() 
{
   DebugLog (<< "Deleting TestSipEndPoint " << getName());
   
   unregisterFromTransportDriver();
   delete mTransport;
}

void
TestSipEndPoint::clean()
{
   DebugLog(<< *this << " Clearing out requests.");
   mInvitesSent.clear();
   mInvitesReceived.clear();
   mUpdatesSent.clear();
   mUpdatesReceived.clear();
   mSubscribesSent.clear();
   mSubscribesReceived.clear();
   mRequests.clear();
   
   for (DialogList::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
   {
      delete (*it);
   }
   mDialogs.clear();
   mLastMessage.reset();
#ifdef RTP_ON
   delete mRtpSession;
   mRtpSession = 0;
#endif

   TestEndPoint::clear();
}

void
TestSipEndPoint::setTransport(Transport* transport)
{
   resip_assert(transport && !mTransport);
   if (transport && !mTransport)
   {
      mTransport = transport;
      registerWithTransportDriver();
   }
}

void 
TestSipEndPoint::send(boost::shared_ptr<SipMessage>& msg, RawConditionerFn func)
{
   const Tuple* useTuple = 0;

   Uri uri;
   if (msg->isRequest())
   {
      resip_assert(!msg->header(h_Vias).empty());

      if( !msg->header(h_Vias).front().exists(p_branch) ||
         msg->header(h_Vias).front().param(p_branch).getTransactionId().empty())
      {
         // .bwc. Hide 2543 tid in topmost via where receiver won't notice
         // it. We will see this over in X when responses come back.
         static ExtensionParameter p_brunch("brunch");
         msg->header(h_Vias).front().param(p_brunch)=msg->getTransactionId();
      }

      if (msg->header(h_RequestLine).getMethod() != ACK)
      {
         mRequests[msg->getTransactionId()] = msg;
      }
      
      //DebugLog (<< "Sending: " << Inserter(mRequests));
      
      if (msg->exists(h_Routes) && !msg->header(h_Routes).empty())
      {
         uri = msg->header(h_Routes).front().uri();
         DebugLog(<< "sending based on routes: " << uri);
      }
      else if (mOutboundProxy != TestSipEndPoint::NoOutboundProxy)
      {
         uri = mOutboundProxy;
      }
      else
      {
         uri = msg->header(h_RequestLine).uri();
      }
   }
   else if (msg->isResponse())
   {
      resip_assert (!msg->header(h_Vias).empty());
      Via& via = msg->header(h_Vias).front();

      // use tuple from request if it exists
      if (msg->header(h_Vias).front().exists(p_branch))
      {
         RequestMap::const_iterator i = 
            mRequests.find(msg->header(h_Vias).front().param(p_branch).getTransactionId());
         if (i != mRequests.end())
         {
            useTuple = &i->second->getSource();
         }
      }
      
      resip::Data& target = via.exists(p_maddr) ? via.param(p_maddr) : via.sentHost();
      if (via.exists(p_received))
      {
         if (via.exists(p_rport))
         {
            uri.host() = via.param(p_received);
            uri.port() = via.param(p_rport).port();
         }
         else
         {
            if (via.sentPort())
            {
               uri.host() = via.param(p_received);
               uri.port() = via.sentPort();
            }
            else
            {
               uri.host() = via.param(p_received);
               uri.port() = 5060; // !jf! bad
            }
         }
      }
      else if (via.exists(p_rport))
      {
         uri.host() = target;
         uri.port() = via.param(p_rport).port();
      }
      else if (via.sentPort())
      {
         uri.host() = target;
         uri.port() = via.sentPort();
      }
      else
      {
         uri.host() = target;
         uri.port() = 5060;
      }
   }
   else
   {
      // not request and not response
      resip_assert(0);
   }
   
   DebugLog(<<"Target uri: " << uri);

   // modifying the via
   if (msg->isRequest())
   {
      resip_assert(!msg->header(h_Vias).empty());
      msg->header(h_Vias).front().remove(p_maddr);
      msg->header(h_Vias).front().transport() = Tuple::toData(mTransport->transport());

      //Sufficient, and also necessary if we are sending on TLS.
      if(msg->header(h_Vias).front().transport().lowercase() == "tls")
      {
         msg->header(h_Vias).front().sentHost() = mTransport->tlsDomain();
      }
      else if(mTransport->interfaceName()!="0.0.0.0" && 
               !mTransport->interfaceName().empty())
      {
         msg->header(h_Vias).front().sentHost() = mTransport->interfaceName();
      }
      else
      {
         msg->header(h_Vias).front().sentHost() = nwIf;
      }
      msg->header(h_Vias).front().sentPort() = mTransport->port();
   }
   
   resip::Data encoded;
   {
      DataStream encodeStream(encoded);
      msg->encode(encodeStream);
      encodeStream.flush();
   }
   DebugLog (<< "encoded=" << encoded.c_str());

   resip::Data toWrite = func(encoded);
   
   if (useTuple)
   {
      // Find Transport via TransportDriver
      Transport* transport = TransportDriver::instance().getClientTransport(useTuple->mTransportKey);
      resip_assert(transport);
      std::auto_ptr<SendData> toSend(transport->makeSendData(*useTuple, toWrite, "bogus"));
      transport->send(toSend);
   }
   else
   {
      //The transport we are sending on had better match the transport we are sending to
      uri.param(p_transport)=msg->header(h_Vias).front().transport();
      DebugLog(<<"Trying to resolve...");
      // send it over the transport
      Resolver r(uri);
      resip_assert (!r.mNextHops.empty());
      DebugLog(<<"Resolved successfully.");
      r.mNextHops.front().setTargetDomain(uri.host());

      //Default TLS port
      if(r.mNextHops.front().getType()==TLS && r.mNextHops.front().getPort()==5060)
      {
         r.mNextHops.front().setPort(5061);
      }
      std::auto_ptr<SendData> toSend(mTransport->makeSendData(r.mNextHops.front(), toWrite, "bogus"));
      mTransport->send(toSend);
   }
}

void TestSipEndPoint::storeSentInvite(const boost::shared_ptr<SipMessage>& invite)
{
   resip_assert(invite->isRequest());
   resip_assert(invite->header(h_RequestLine).getMethod() == INVITE);
   DebugLog (<< "Storing invite: " << invite->header(h_CallId) << " in " << this);
   
   for(InviteList::iterator it = mInvitesSent.begin();
       it != mInvitesSent.end(); it++)
   {
      if ( (*it)->header(h_CallId) == invite->header(h_CallId))
      {
         (*it) = invite;
         return;
      }
   }
   mInvitesSent.push_back(invite);
}

void TestSipEndPoint::storeReceivedInvite(const boost::shared_ptr<SipMessage>& invite)
{
   for(InviteList::iterator it = mInvitesReceived.begin();
       it != mInvitesReceived.end(); it++)
   {
      if ( (*it)->header(h_CallId) == invite->header(h_CallId))
      {
         (*it) = invite;
         return;
      }
   }
   mInvitesReceived.push_back(invite);
}


void TestSipEndPoint::storeSentUpdate(const boost::shared_ptr<SipMessage>& update)
{
   resip_assert(update->isRequest());
   resip_assert(update->header(h_RequestLine).getMethod() == UPDATE);
   DebugLog (<< "Storing update: " << update->header(h_CallId) << " in " << this);
   
   for(UpdateList::iterator it = mUpdatesSent.begin();
       it != mUpdatesSent.end(); it++)
   {
      if ( (*it)->header(h_CallId) == update->header(h_CallId))
      {
         (*it) = update;
         return;
      }
   }
   mUpdatesSent.push_back(update);
}

void TestSipEndPoint::storeReceivedUpdate(const boost::shared_ptr<SipMessage>& update)
{
   for(UpdateList::iterator it = mUpdatesReceived.begin();
       it != mUpdatesReceived.end(); it++)
   {
      if ( (*it)->header(h_CallId) == update->header(h_CallId))
      {
         (*it) = update;
         return;
      }
   }
   mUpdatesReceived.push_back(update);
}

boost::shared_ptr<SipMessage> 
TestSipEndPoint::getSentInvite(const CallId& callId)
{
   for(InviteList::iterator it = mInvitesSent.begin();
       it != mInvitesSent.end(); it++)
   {
      if ( (*it)->header(h_CallId) == callId)
      {
         return *it;
      }
   }
   InfoLog (<< "No stored invites (probably retranmissions of failed test)");
   InfoLog (<< Inserter(mInvitesSent));
   throw AssertException("No stored invites (likely retranmissions from failed test)", __FILE__, __LINE__);
}
 
boost::shared_ptr<SipMessage> 
TestSipEndPoint::getReceivedInvite(const CallId& callId)
{
   for(InviteList::iterator it = mInvitesReceived.begin();
       it != mInvitesReceived.end(); it++)
   {
      if ( (*it)->header(h_CallId) == callId)
      {
         return *it;
      }
   }
   return boost::shared_ptr<SipMessage>();
}  

 
boost::shared_ptr<SipMessage> 
TestSipEndPoint::getReceivedUpdate(const CallId& callId)
{
   for(UpdateList::iterator it = mUpdatesReceived.begin();
       it != mUpdatesReceived.end(); it++)
   {
      if ( (*it)->header(h_CallId) == callId)
      {
         return *it;
      }
   }
   return boost::shared_ptr<SipMessage>();
}  

void TestSipEndPoint::storeSentSubscribe(const boost::shared_ptr<SipMessage>& subscribe)
{
   InfoLog(<< "storeSentSubscribe " << *subscribe);
   for (SentSubscribeList::iterator it = mSubscribesSent.begin();
       it != mSubscribesSent.end(); ++it)
   {
      if (it->isMatch(subscribe))
      {
         return;
      }
   }
      
   // add a new DialogSet
   mSubscribesSent.push_back(::DialogSet(subscribe, *this));
}

void TestSipEndPoint::storeReceivedSubscribe(const boost::shared_ptr<SipMessage>& subscribe)
{
   for (ReceivedSubscribeList::iterator it = mSubscribesReceived.begin();
       it != mSubscribesReceived.end(); ++it)
   {
      if ( (*it)->header(h_CallId) == subscribe->header(h_CallId))
      {
         (*it) = subscribe;
         return;
      }
   }
   mSubscribesReceived.push_back(subscribe);
}

void TestSipEndPoint::storeReceivedPublish(const boost::shared_ptr<SipMessage>& publish)
{
   for (ReceivedPublishList::iterator it = mPublishReceived.begin();
       it != mPublishReceived.end(); ++it)
   {
      if ( (*it)->header(h_CallId) == publish->header(h_CallId))
      {
         (*it) = publish;
         return;
      }
   }
   mPublishReceived.push_back(publish);
}

boost::shared_ptr<SipMessage> 
TestSipEndPoint::getSentSubscribe(boost::shared_ptr<SipMessage> msg)
{
   InfoLog(<< "getSentSubscribe: " << *msg);
   for (SentSubscribeList::iterator it = mSubscribesSent.begin();
        it != mSubscribesSent.end(); ++it)
   {
      InfoLog(<< "getSentSubscribe trying " << *it->getMessage());
      if (it->isMatch(msg))
      {
         return it->getMessage();
      }
   }
   ErrLog(<< "getSentSubscribe failed");
   return boost::shared_ptr<SipMessage>();
}
 
boost::shared_ptr<SipMessage> 
TestSipEndPoint::getReceivedSubscribe(const CallId& callId)
{
  for(ReceivedSubscribeList::iterator it = mSubscribesReceived.begin();
       it != mSubscribesReceived.end(); it++)
   {
      if ( (*it)->header(h_CallId) == callId)
      {
         return *it;
      }
   }
   return boost::shared_ptr<SipMessage>();
}  

boost::shared_ptr<SipMessage> 
TestSipEndPoint::getReceivedPublish(const CallId& callId)
{
  for(ReceivedSubscribeList::iterator it = mPublishReceived.begin();
       it != mPublishReceived.end(); it++)
   {
      if ( (*it)->header(h_CallId) == callId)
      {
         return *it;
      }
   }
   return boost::shared_ptr<SipMessage>();
}  

DeprecatedDialog*
TestSipEndPoint::getDialog()
{
   // An unfortunate "ONLY 1 dialog per endpoint" limitation
   resip_assert(mDialogs.size() <= 1); 

   if (mDialogs.size())
   {
      return *(mDialogs.begin());
   }
   else
   {
      return 0;
   }
}

DeprecatedDialog*
TestSipEndPoint::getDialog(const CallId& callId,
                           const resip::Data& remoteTag)
{
   DebugLog(<< "searching for callid: " << callId << " tag:" << remoteTag);
   for(DialogList::iterator it = mDialogs.begin();
       it != mDialogs.end(); it++)
   {
      DebugLog (<< "Comparing to dialog: " << (*it));
      if ( (*it)->getCallId() == callId && (remoteTag.empty() || (*it)->getRemoteTag()==remoteTag))
      {
         DebugLog(<< "Dialog: " << *it << "  " << (*it)->getCallId() << " "
         << (*it)->getRemoteTag() << " matched.");
         return *it;
      }
      DebugLog(<< "Dialog: " << *it << "  " << (*it)->getCallId() << " "
         << (*it)->getRemoteTag() <<  "did not match.");
   }
   return 0;
}

DeprecatedDialog* 
TestSipEndPoint::getDialog(const Uri& target)
{
   // DebugLog(<< this->getContact() << " has " << mDialogs.size() << " dialogs.");
   for(DialogList::iterator it = mDialogs.begin();
       it != mDialogs.end(); it++)
   {
      // DebugLog(<< (*it)->getRemoteTarget().uri().getAor());
      if ((*it)->getRemoteTarget().uri().getAor() == target.getAor())
      {
         // DebugLog(<< "Matched in getDialogByTarget");
         return *it;
      }
   }
   return 0;
}

DeprecatedDialog* 
TestSipEndPoint::getDialog(const Data& user)
{
   DebugLog(<< this->getContact() << " has " << mDialogs.size() << " dialogs.");
   for(DialogList::iterator it = mDialogs.begin();
       it != mDialogs.end(); it++)
   {
      DebugLog(<< (*it)->getRemoteTarget().uri().user());
      if ((*it)->getRemoteTarget().uri().user() == user)
      {
         DebugLog(<< "Matched in getDialog");
         return *it;
      }
   }
   return 0;
}

DeprecatedDialog* 
TestSipEndPoint::getDialog(const NameAddr& target)
{
   return getDialog(target.uri());
}

// needs to deal with INVITE SDP contents
boost::shared_ptr<SipMessage>
TestSipEndPoint::makeResponse(SipMessage& request, int responseCode)
{
   resip_assert(request.isRequest());
   if ( responseCode < 300 && responseCode > 100 &&
        (request.header(h_RequestLine).getMethod() == INVITE ||
         request.header(h_RequestLine).getMethod() == SUBSCRIBE ||
         request.header(h_RequestLine).getMethod() == PUBLISH) )
   {
      DeprecatedDialog* dialog = getDialog(request.header(h_CallId),
                                                request.header(h_From).param(p_tag));
      if (!dialog)
      {
         DebugLog(<< "making a dialog, contact: " << getContact());
         dialog = new DeprecatedDialog(getContact());
         mDialogs.push_back(dialog);
         DebugLog(<< "made a dialog (" << dialog << "), contact: " << getContact());
      }
      DebugLog(<< "Creating response using dialog: " << dialog);
      boost::shared_ptr<SipMessage> response(dialog->makeResponse(request, responseCode));

      return response;
   }
   else
   {
      DeprecatedDialog* dialog = getDialog(request.header(h_CallId),
                                                request.header(h_From).param(p_tag));
      if (!dialog)
      {
         DebugLog(<<"making response outside of dialog, contact: " << getContact());
         boost::shared_ptr<SipMessage> response(Helper::makeResponse(request, responseCode, getContact()));
         return response;
      }
      else
      {
         DebugLog(<< "Creating response using dialog: " << dialog);
         boost::shared_ptr<SipMessage> response(dialog->makeResponse(request, responseCode));
         return response;
      }
   }
}
 
boost::shared_ptr<SipMessage>
TestSipEndPoint::Dump::go(boost::shared_ptr<SipMessage> msg)
{
   InfoLog(<<"##Dump: " << mEndPoint.getName() << " " << *msg);
   // don't send it
   static const boost::shared_ptr<SipMessage> nul;
   return nul;
}
     
boost::shared_ptr<SipMessage>
TestSipEndPoint::ByeTo::go(boost::shared_ptr<SipMessage> msg, const Uri& target)
{
   DeprecatedDialog* dialog = mEndPoint.getDialog(target);
   resip_assert(dialog);
   boost::shared_ptr<SipMessage> bye(dialog->makeBye());
   return bye;
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Bye::go(boost::shared_ptr<SipMessage> msg)
{
   resip::Data remoteTag(msg->isRequest() ? msg->header(h_From).param(p_tag) :
                                            msg->header(h_To).param(p_tag) );
   DeprecatedDialog* dialog = mEndPoint.getDialog(msg->header(h_CallId),remoteTag);
   if(!dialog)
   {
      resip::Data localTag(!msg->isRequest() ? msg->header(h_From).param(p_tag):
                                               msg->header(h_To).param(p_tag) );
      dialog=mEndPoint.getDialog(msg->header(h_CallId),localTag);
   }
   resip_assert(dialog);
   boost::shared_ptr<SipMessage> bye(dialog->makeBye());
   return bye;
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Notify200To::go(boost::shared_ptr<SipMessage> msg, const Uri& target)
{
   DeprecatedDialog* dialog = mEndPoint.getDialog(target);
   resip_assert(dialog);
   boost::shared_ptr<SipMessage> notify(dialog->makeNotify());
   notify->header(h_Event).value() = "refer";
   SipFrag frag;
   frag.message().header(h_StatusLine).responseCode() = 200;
   frag.message().header(h_StatusLine).reason() = "OK";
   notify->setContents(&frag);
   return notify;
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Notify200::go(boost::shared_ptr<SipMessage> msg)
{
   resip::Data remoteTag(msg->isRequest() ? msg->header(h_From).param(p_tag) :
                                            msg->header(h_To).param(p_tag) );
   DeprecatedDialog* dialog = mEndPoint.getDialog(msg->header(h_CallId),remoteTag);
   resip_assert(dialog);
   boost::shared_ptr<SipMessage> notify(dialog->makeNotify());
   notify->header(h_Event).value() = "refer";
   SipFrag frag;
   frag.message().header(h_StatusLine).responseCode() = 200;
   frag.message().header(h_StatusLine).reason() = "OK";
   notify->setContents(&frag);
   return notify;
}

TestSipEndPoint::SipEndPointAction::SipEndPointAction(TestSipEndPoint* endPoint)
   : mEndPoint(endPoint)
{
}           

TestSipEndPoint::SipEndPointAction::~SipEndPointAction() 
{
}

void 
TestSipEndPoint::SipEndPointAction::operator()()
{
   (*this)(*mEndPoint);
}

TestSipEndPoint::Refer::Refer(TestSipEndPoint* endPoint, 
                              const resip::Uri& who, 
                              const resip::Uri& to,
                              bool replaces)
   : SipEndPointAction(endPoint), 
     mTo(to),
     mWho(who),
     mReplaces(replaces)
{
}

void
TestSipEndPoint::Refer::operator()(TestSipEndPoint& endPoint)
{  
   DeprecatedDialog* dialog = endPoint.getDialog(mWho.user());
   resip_assert(dialog);
   boost::shared_ptr<SipMessage> refer(dialog->makeRefer(NameAddr(mTo)));
   if (mReplaces)
   {
      refer->header(h_Replaces) = dialog->makeReplaces();
   }
   endPoint.send(refer);
}

resip::Data
TestSipEndPoint::Refer::toString() const
{
   return mEndPoint->getName() + ".refer()";
}

TestSipEndPoint::Refer* 
TestSipEndPoint::refer(const resip::Uri& who, 
                       const resip::Uri& to)
{
   return new Refer(this, who, to); 
}

TestSipEndPoint::Refer* 
TestSipEndPoint::referReplaces(const resip::Uri& who, 
                               const resip::Uri& to) 
{
   return new Refer(this, who, to, true); 
}

TestSipEndPoint::ReInvite::ReInvite(TestSipEndPoint* from, 
                                    const resip::Uri& to,
                                    bool matchUserOnly,
                                    boost::shared_ptr<resip::SdpContents> sdp)
   : mEndPoint(*from),
     mTo(to),
     mMatchUserOnly(matchUserOnly),
     mSdp(sdp)
{
}

void 
TestSipEndPoint::ReInvite::operator()() 
{ 
   go(); 
}

void 
TestSipEndPoint::ReInvite::operator()(boost::shared_ptr<Event> event)
{
   go();
}

void
TestSipEndPoint::ReInvite::go()
{
   DebugLog(<< "Re-Inviting to: " << mTo);
   DeprecatedDialog* dialog;
   if( mMatchUserOnly )
   {
      dialog = mEndPoint.getDialog(mTo.uri().user());
   }
   
   else
   {
      dialog = mEndPoint.getDialog(mTo);
   }
   
   if (!dialog) 
   {
      InfoLog (<< "No matching dialog on " << mEndPoint.getName() << " for " << mTo);
      throw AssertException(resip::Data("No matching dialog"), __FILE__, __LINE__);
   }
      
   boost::shared_ptr<SipMessage> invite(dialog->makeInvite());
   if (mSdp.get())
   {
      invite->setContents(mSdp.get());
   }
  
   DebugLog(<< "TestSipEndPoint::ReInvite: " << *invite);
   mEndPoint.storeSentInvite(invite);
   mEndPoint.send(invite);
}

resip::Data
TestSipEndPoint::ReInvite::toString() const
{
   return mEndPoint.getName() + ".reInvite()";
}

TestSipEndPoint::ReInvite* 
TestSipEndPoint::reInvite(const TestSipEndPoint& endPoint)
{
   return new ReInvite(this, endPoint.getContact().uri()); 
}

TestSipEndPoint::ReInvite* 
TestSipEndPoint::reInvite(resip::Uri& url) 
{
   return new ReInvite(this, url); 
}

TestSipEndPoint::ReInvite* 
TestSipEndPoint::reInvite(const Data& user) 
{
   Uri to;
   to.user() = user;
   return new ReInvite(this, to, true); 
}

TestSipEndPoint::ReInvite*
TestSipEndPoint::reInvite(const Data& user, const boost::shared_ptr<resip::SdpContents>& sdp) 
{
   Uri to;
   to.user() = user;
   return new ReInvite(this, to, true, sdp);
}

TestSipEndPoint::Update::Update(TestSipEndPoint* from, 
                                const resip::Uri& to,
                                bool matchUserOnly,
                                boost::shared_ptr<resip::SdpContents> sdp)
   : mEndPoint(*from),
     mTo(to),
     mMatchUserOnly(matchUserOnly),
     mSdp(sdp)
{
}

void 
TestSipEndPoint::Update::operator()() 
{ 
   go(); 
}

void 
TestSipEndPoint::Update::operator()(boost::shared_ptr<Event> event)
{
   go();
}

void
TestSipEndPoint::Update::go()
{
   DebugLog(<< "Update to: " << mTo);
   DeprecatedDialog* dialog;
   if( mMatchUserOnly )
   {
      dialog = mEndPoint.getDialog(mTo.uri().user());
   }
   
   else
   {
      dialog = mEndPoint.getDialog(mTo);
   }
   
   if (!dialog) 
   {
      InfoLog (<< "No matching dialog on " << mEndPoint.getName() << " for " << mTo);
      throw AssertException(resip::Data("No matching dialog"), __FILE__, __LINE__);
   }
      
   boost::shared_ptr<SipMessage> update(dialog->makeUpdate());
   if (mSdp.get())
   {
      update->setContents(mSdp.get());
   }
  
   DebugLog(<< "TestSipEndPoint::Update: " << *update);
   mEndPoint.storeSentUpdate(update);
   mEndPoint.send(update);
}

resip::Data
TestSipEndPoint::Update::toString() const
{
   return mEndPoint.getName() + ".Update()";
}

TestSipEndPoint::Update* 
TestSipEndPoint::update(const TestSipEndPoint& endPoint)
{
   return new Update(this, endPoint.getContact().uri()); 
}

TestSipEndPoint::Update* 
TestSipEndPoint::update(resip::Uri& url) 
{
   return new Update(this, url); 
}

TestSipEndPoint::Update* 
TestSipEndPoint::update(const Data& user) 
{
   Uri to;
   to.user() = user;
   return new Update(this, to, true); 
}

TestSipEndPoint::Update*
TestSipEndPoint::update(const Data& user, const boost::shared_ptr<resip::SdpContents>& sdp) 
{
   Uri to;
   to.user() = user;
   return new Update(this, to, true, sdp);
}

TestSipEndPoint::InviteReferReplaces::InviteReferReplaces(TestSipEndPoint* from, 
                                                          bool replaces) 
   : mEndPoint(*from), mReplaces(replaces) 
{
}

void
TestSipEndPoint::InviteReferReplaces::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   boost::shared_ptr<SipMessage> refer = sipEvent->getMessage();
   resip_assert(refer->isRequest());
   resip_assert(refer->header(h_RequestLine).getMethod() == REFER);
   
   boost::shared_ptr<SipMessage> invite(Helper::makeInvite(refer->header(h_ReferTo),
                                                    NameAddr(mEndPoint.getAddressOfRecord()),
                                                    mEndPoint.getContact()));
   invite->header(h_ReferredBy) = refer->header(h_ReferredBy);
   if (mReplaces)
   {
      invite->header(h_Replaces) = refer->header(h_Replaces);
   }
   mEndPoint.storeSentInvite(invite);
   DebugLog(<< "sending INVITE " << invite->brief());
   mEndPoint.send(invite);
}
resip::Data
TestSipEndPoint::InviteReferReplaces::toString() const
{
   return mEndPoint.getName() + ".inviteReferReplaces()";
}

TestSipEndPoint::InviteReferReplaces* 
TestSipEndPoint::inviteReferReplaces() 
{
   return new InviteReferReplaces(this, true); 
}

TestSipEndPoint::InviteReferReplaces* 
TestSipEndPoint::inviteReferredBy() 
{
   return new InviteReferReplaces(this, false); 
}

boost::shared_ptr<resip::SipMessage> 
TestSipEndPoint::IdentityMessageConditioner::operator()(boost::shared_ptr<resip::SipMessage> msg)
{
   return msg;
}

resip::Data 
TestSipEndPoint::IdentityRawConditioner::operator()(const resip::Data& input)
{
   return input;
}

TestSipEndPoint::ChainConditions::ChainConditions(MessageConditionerFn fn1, 
                                                  MessageConditionerFn fn2)
   : mFn1(fn1),
     mFn2(fn2)
{
}

boost::shared_ptr<resip::SipMessage> 
TestSipEndPoint::ChainConditions::operator()(boost::shared_ptr<resip::SipMessage> msg)
{
   msg = mFn2(msg);
   return mFn1(msg);
}

TestSipEndPoint::ChainRawConditions::ChainRawConditions(RawConditionerFn fn1, 
                                                  RawConditionerFn fn2)
   : mFn1(fn1),
     mFn2(fn2)
{
}

resip::Data
TestSipEndPoint::ChainRawConditions::operator()(const resip::Data& input)
{
   return mFn1(mFn2(input));
}

TestSipEndPoint::SaveMessage::SaveMessage(boost::shared_ptr<resip::SipMessage>& msgPtr)
   : mMsgPtr(msgPtr)
{
}

boost::shared_ptr<resip::SipMessage> 
TestSipEndPoint::SaveMessage::operator()(boost::shared_ptr<resip::SipMessage> msg)
{
   // .dlb. could snapshot here, but would slice (unless clone)
   mMsgPtr = msg;
   return msg;
}

TestSipEndPoint::MessageAction::MessageAction(TestSipEndPoint& from, 
                                              const resip::Uri& to)
   : mEndPoint(from),
     mTo(to),
     mMsg(),
     mConditioner(TestSipEndPoint::identity),
     mRawConditioner(TestSipEndPoint::raw_identity)     
{
}

void 
TestSipEndPoint::MessageAction::setConditioner(MessageConditionerFn conditioner)
{
   mConditioner = ChainConditions(conditioner, mConditioner);
}

void 
TestSipEndPoint::MessageAction::setRawConditioner(RawConditionerFn conditioner)
{
   mRawConditioner = ChainRawConditions(conditioner, mRawConditioner);
}

void
TestSipEndPoint::MessageAction::operator()() 
{ 
   mMsg = go(); 
   if (mMsg.get())
   {
      boost::shared_ptr<SipMessage> conditioned(mConditioner(mMsg));
      DebugLog(<< "sending: " << conditioned->brief());
      mEndPoint.send(conditioned,mRawConditioner);
   }
}

void
TestSipEndPoint::MessageAction::operator()(boost::shared_ptr<Event> event)
{
   Action::operator()(event);
}

TestSipEndPoint::Invite::Invite(TestSipEndPoint* from, 
                                const resip::Uri& to, 
                                bool useOutbound,
                                EndpointReliableProvisionalMode mode,
                                boost::shared_ptr<resip::SdpContents> sdp)
   : MessageAction(*from, to),
     mUseOutbound(useOutbound),
     mRelProvMode(mode),
     mSdp(sdp)
{}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Invite::go()
{
   boost::shared_ptr<SipMessage> invite(Helper::makeInvite(NameAddr(mTo),
                                                    NameAddr(mEndPoint.getAddressOfRecord()),
                                                    mEndPoint.getContact()));
   if (mSdp.get())
      invite->setContents(mSdp.get());

   if(mUseOutbound)
   {
      invite->header(h_Contacts).front().uri().param(p_ob);
   }

   if(mRelProvMode == RelProvModeSupported)
   {
      invite->header(h_Supporteds).push_back(Token(Symbols::C100rel));
   }
   else if(mRelProvMode == RelProvModeRequired)
   {
      invite->header(h_Requires).push_back(Token(Symbols::C100rel));
   }

   // Add allow header
   invite->header(h_Allows).push_back(Token(getMethodName(INVITE)));
   invite->header(h_Allows).push_back(Token(getMethodName(ACK)));
   invite->header(h_Allows).push_back(Token(getMethodName(CANCEL)));
   invite->header(h_Allows).push_back(Token(getMethodName(OPTIONS)));
   invite->header(h_Allows).push_back(Token(getMethodName(BYE)));
   invite->header(h_Allows).push_back(Token(getMethodName(UPDATE)));
   invite->header(h_Allows).push_back(Token(getMethodName(INFO)));
   invite->header(h_Allows).push_back(Token(getMethodName(MESSAGE)));
   invite->header(h_Allows).push_back(Token(getMethodName(REFER)));
   invite->header(h_Allows).push_back(Token(getMethodName(PRACK)));
   invite->header(h_Allows).push_back(Token(getMethodName(NOTIFY)));
   invite->header(h_Allows).push_back(Token(getMethodName(SUBSCRIBE)));

   mEndPoint.storeSentInvite(invite);
   return invite;
}

resip::Data
TestSipEndPoint::Invite::toString() const
{
   return mEndPoint.getName() + ".invite()";
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const TestUser& endPoint, EndpointReliableProvisionalMode mode)
{
   return new Invite(this, endPoint.getAddressOfRecord(), false, mode);
}


TestSipEndPoint::Invite*
TestSipEndPoint::invite(const TestUser& endPoint, const boost::shared_ptr<resip::SdpContents>& sdp, EndpointReliableProvisionalMode mode)
{
   return new Invite(this, endPoint.getAddressOfRecord(), false, mode, sdp);
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const TestSipEndPoint& endPoint, EndpointReliableProvisionalMode mode) 
{
   return new Invite(this, endPoint.mAor, false, mode); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const resip::Uri& url, EndpointReliableProvisionalMode mode) 
{
   return new Invite(this, url, false, mode); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const resip::Uri& url, const boost::shared_ptr<resip::SdpContents>& sdp, EndpointReliableProvisionalMode mode) 
{
   return new Invite(this, url, false, mode, sdp); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const resip::Data& url, EndpointReliableProvisionalMode mode)
{
   return new Invite(this, resip::Uri(url), false, mode); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::inviteWithOutbound(const TestUser& endPoint, EndpointReliableProvisionalMode mode)
{
   return new Invite(this, endPoint.getAddressOfRecord(), true, mode);
}


TestSipEndPoint::Invite*
TestSipEndPoint::inviteWithOutbound(const TestUser& endPoint, const boost::shared_ptr<resip::SdpContents>& sdp, EndpointReliableProvisionalMode mode)
{
   return new Invite(this, endPoint.getAddressOfRecord(), true, mode, sdp);
}

TestSipEndPoint::Invite* 
TestSipEndPoint::inviteWithOutbound(const TestSipEndPoint& endPoint, EndpointReliableProvisionalMode mode) 
{
   return new Invite(this, endPoint.mAor, true, mode); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::inviteWithOutbound(const resip::Uri& url, EndpointReliableProvisionalMode mode) 
{
   return new Invite(this, url, true, mode); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::inviteWithOutbound(const resip::Uri& url, const boost::shared_ptr<resip::SdpContents>& sdp, EndpointReliableProvisionalMode mode) 
{
   return new Invite(this, url, true, mode, sdp); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::inviteWithOutbound(const resip::Data& url, EndpointReliableProvisionalMode mode)
{
   return new Invite(this, resip::Uri(url), true, mode); 
}

TestSipEndPoint::SendSip::SendSip(TestSipEndPoint* from,
                              const resip::Uri& to,
                              boost::shared_ptr<resip::SipMessage>& msg) :
   MessageAction(*from,to),
   mMsgToTransmit(msg)
{
}

Data
TestSipEndPoint::SendSip::toString() const
{
   return Data("TestSipEndPoint::Send");
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::SendSip::go()
{
   return mMsgToTransmit;
}

TestSipEndPoint::SendSip*
TestSipEndPoint::sendSip(boost::shared_ptr<SipMessage>& msg,
                           const Uri& to)
{
   return new SendSip(this,to,msg);
}

TestSipEndPoint::SendSip*
TestSipEndPoint::sendSip(boost::shared_ptr<SipMessage>& msg,
                           const TestUser& endPoint)
{
   return new SendSip(this,endPoint.getAddressOfRecord(),msg);
}

TestSipEndPoint::RawInvite::RawInvite(TestSipEndPoint* from, 
                                      const resip::Uri& to, 
                                      const resip::Data& rawText)
   : MessageAction(*from, to),
     mRawText(rawText)
{}

boost::shared_ptr<SipMessage>
TestSipEndPoint::RawInvite::go()
{
   auto_ptr<SipMessage> msg(Helper::makeInvite(NameAddr(mTo),
                                               NameAddr(mEndPoint.getAddressOfRecord()),
                                               mEndPoint.getContact()));
   boost::shared_ptr<SipMessage> rawInvite(new SipRawMessage(*msg, mRawText));
   mEndPoint.storeSentInvite(rawInvite);
   return rawInvite;
}

resip::Data
TestSipEndPoint::RawInvite::toString() const
{
   static const unsigned int truncate = 15;
   resip::Data buffer;
   {   
      DataStream strm(buffer);
      strm << mEndPoint.getName() << ".rawInvite(\"" << 
         ((mRawText.size() <= truncate)
          ? mRawText
          : resip::Data(mRawText.data(), truncate-2)) << "..\")";
   }
   return buffer;
}

TestSipEndPoint::RawInvite* 
TestSipEndPoint::rawInvite(const TestUser& endPoint,
                           const resip::Data& rawText)
{
   return new RawInvite(this, endPoint.getAddressOfRecord(), rawText);
}

TestSipEndPoint::RawInvite* 
TestSipEndPoint::rawInvite(const TestSipEndPoint* endPoint,
                           const resip::Data& rawText)
{
   return new RawInvite(this, endPoint->mAor, rawText);
}


TestSipEndPoint::RawSend::RawSend(TestSipEndPoint* from, 
                                  const resip::Uri& to, 
                                  const resip::Data& rawText)
   : mEndPoint(*from),
     mTo(to),
     mRawText(rawText),
     mRawConditioner(TestSipEndPoint::raw_identity)
{}

void
TestSipEndPoint::RawSend::operator()()
{
   go();
}

void
TestSipEndPoint::RawSend::operator()(boost::shared_ptr<Event> event)
{
   go();
}

void 
TestSipEndPoint::RawSend::go()
{
   Tuple target(mTo.uri().host(), mTo.uri().port(), TCP);
   mRawText=mRawConditioner(mRawText);
   std::auto_ptr<SendData> toSend(mEndPoint.mTransport->makeSendData(target, mRawText, 0));
   mEndPoint.mTransport->send(toSend);
}

void
TestSipEndPoint::RawSend::setRawConditioner(RawConditionerFn conditioner)
{
   mRawConditioner = ChainRawConditions(conditioner, mRawConditioner);
}

resip::Data
TestSipEndPoint::RawSend::toString() const
{
   static const unsigned int truncate = 15;
   resip::Data buffer;
   {   
      DataStream strm(buffer);
      strm << mEndPoint.getName() << ".rawSend(\"" << 
         ((mRawText.size() <= truncate)
          ? mRawText
          : resip::Data(mRawText.data(), truncate-2)) << "..\")";
   }
   return buffer;
}

TestSipEndPoint::RawSend* 
TestSipEndPoint::rawSend(const TestUser& endPoint,
                         const resip::Data& rawText)
{
   return new RawSend(this, endPoint.getAddressOfRecord(), rawText);
}


TestSipEndPoint::RawSend* 
TestSipEndPoint::rawSend(const TestSipEndPoint* endPoint,
                         const resip::Data& rawText)
{
   return new RawSend(this, endPoint->mAor, rawText);
}

TestSipEndPoint::RawSend* 
TestSipEndPoint::rawSend(const Uri& target, const resip::Data& rawText)
{
   return new RawSend(this, target, rawText);
}

TestSipEndPoint::Subscribe::Subscribe(TestSipEndPoint* from, const Uri& to, const Token& eventPackage, int pExpires, const string PAssertedIdentity, bool pIgnoreExistingDialog)
   : MessageAction(*from, to),
     mEventPackage(eventPackage),
     mAccept(),
     mContents( boost::shared_ptr<resip::Contents>()),
     mExpires(pExpires),
     mIgnoreExistingDialog(pIgnoreExistingDialog)
{
     if (PAssertedIdentity.size() != 0)
      mPAssertedIdentity = PAssertedIdentity;
}

TestSipEndPoint::Subscribe::Subscribe(TestSipEndPoint* from, 
                                      const Uri& to,
                                      const Token& eventPackage,
                                      const Mime& accept,
                                      boost::shared_ptr<resip::Contents> contents)
   : MessageAction(*from, to),
     mEventPackage(eventPackage),
     mAccept(accept),
     mContents(contents),
     mExpires(3600),
     mIgnoreExistingDialog(false)
{
}

TestSipEndPoint::Subscribe::Subscribe(TestSipEndPoint* from, 
                                      const Uri& to,
                                      const Token& eventPackage,
                                      const string allow,
                                      const string supported,
                                      const int mExpires,
                                      const string PAssertedIdentity
                                     )
                                      
   : MessageAction(*from, to),
     mEventPackage(eventPackage),
     mAllow(allow),
     mSupported(supported),
     mExpires(mExpires),
     mPAssertedIdentity(PAssertedIdentity),
     mIgnoreExistingDialog(false)
{
}

resip::Data
TestSipEndPoint::Subscribe::toString() const
{
   return mEndPoint.getName() + ".subscribe()";
}

void
TestSipEndPoint::Subscribe::operator()(boost::shared_ptr<Event> event)
{
   if (! mEndPoint.getDialog())
   {
      // subscribe creating a dialog from an incoming request, as opposed to the normal 1xx, 2xx response to INVITE.
      // The test we want to achieve is:
      // --INV-->, <--SUB--, <--1xx--, <--2xx--, --ACK-->, --200(SUB)-->, etc...
      SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
      if (sipEvent)
      {
         boost::shared_ptr<SipMessage> request = sipEvent->getMessage();
         if (request && request->isRequest())
         {
            // Create the dialog by making a dummy response.
            boost::shared_ptr<SipMessage> dummy = mEndPoint.makeResponse(*request, 180);
         }
      }
   }
   MessageAction::operator()(event);
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Subscribe::go()
{
   boost::shared_ptr<SipMessage> subscribe;

   DeprecatedDialog* dialog = mEndPoint.getDialog();
   if (dialog && !mIgnoreExistingDialog)
   {
      subscribe = boost::shared_ptr<SipMessage>(dialog->makeRequest(SUBSCRIBE));
   }
   else
   {
      subscribe = boost::shared_ptr<SipMessage>(Helper::makeRequest(NameAddr(mTo),
                                                             NameAddr(mEndPoint.getAddressOfRecord()),
                                                             mEndPoint.getContact(),
                                                             SUBSCRIBE));
   }

   // subscribe->header(h_Expires).value() = 3600;
   subscribe->header(h_Expires).value() = mExpires;

   subscribe->header(h_Event) = mEventPackage;
   if( !mAccept.type().empty() )
      subscribe->header(h_Accepts).push_front(mAccept);
   if( mContents.get() )
      subscribe->setContents(mContents.get());

   if (mAllow.length() != 0)
   {
     resip::Data AllowData(mAllow);
     resip::Token AllowToken(AllowData);
     subscribe->header(h_Allows).push_back(AllowToken);
   }
   if (mSupported.length() != 0)
   {
     resip::Data SupportedData(mSupported);
     resip::Token SupportedToken(SupportedData);
     subscribe->header(h_Supporteds).push_back(SupportedToken);
   }
   if (mPAssertedIdentity.length() != 0)
   {
     resip::Data PAssertedId(mPAssertedIdentity);
     NameAddr PAssertedId_NameAddr(PAssertedId);
     subscribe->header(h_PAssertedIdentities).push_back(PAssertedId_NameAddr);
   }

   mEndPoint.storeSentSubscribe(subscribe);
   DebugLog(<< "sending SUBSCRIBE " << subscribe->brief());

   return subscribe;
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const TestSipEndPoint* endPoint, const resip::Token& eventPackage) 
{
   return new Subscribe(this, endPoint->mAor, eventPackage); 
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const TestUser& endPoint, const resip::Token& eventPackage) 
{
   return new Subscribe(this, endPoint.getAddressOfRecord(), eventPackage); 
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const Uri& url, const Token& eventPackage, const Mime& accept, const boost::shared_ptr<resip::Contents>& contents)
{
   return new Subscribe(this, url, eventPackage, accept, contents); 
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const Uri& url, const Token& eventPackage, 
                           const int    pExpires, 
                           const string PAssertedIdentity,
                           bool pIgnoreExistingDialog)
{
   return new Subscribe(this, url, eventPackage, pExpires, PAssertedIdentity, 
                        pIgnoreExistingDialog);
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const Uri& url, const Token& eventPackage, 
                           const string allow, 
                           const string supported, 
                           const int    pExpires, 
                           const string PAssertedIdentity)
{
   return new Subscribe(this, url, eventPackage, 
                        allow, supported, pExpires, PAssertedIdentity);
}

TestSipEndPoint::Request::Request(TestSipEndPoint* from, 
                                  const resip::Uri& to, 
                                  resip::MethodTypes type,
                                  boost::shared_ptr<resip::Contents> contents)
   : MessageAction(*from, to),
     mType(type),
     mContents(contents)
{}

resip::Data
TestSipEndPoint::Request::toString() const
{
   resip::Data buffer;
   {
      DataStream strm(buffer);
      strm << mEndPoint.getName() << "." << getMethodName(mType) << "(" << mTo << ")";
      strm.flush();
   }
   return buffer;
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Request::go()
{
   // !jf! this really should be passed in a dialog-id to identify which dialog
   // to use
   DeprecatedDialog* dialog = mEndPoint.getDialog();
   if (dialog)
   {
      boost::shared_ptr<SipMessage> request(dialog->makeRequest(mType));
      if (mContents.get() != 0) request->setContents(mContents.get());
      request->header(h_Expires).value() = 3600;
      return request;   
   }
   else
   {
      boost::shared_ptr<SipMessage> request(Helper::makeRequest(NameAddr(mTo), 
                                                         NameAddr(mEndPoint.getAddressOfRecord()), 
                                                         mEndPoint.getContact(),
                                                         mType));
      if (mContents != 0) request->setContents(mContents.get());
      request->header(h_Expires).value() = 3600;
      return request;
   }
}

TestSipEndPoint::Request* 
TestSipEndPoint::request(const TestUser& endPoint, resip::MethodTypes method, 
                         boost::shared_ptr<resip::Contents> contents)
{
   return new Request(this, endPoint.getAddressOfRecord(), method, contents);
}

TestSipEndPoint::Request* 
TestSipEndPoint::request(const resip::Uri& to, resip::MethodTypes method, 
                         boost::shared_ptr<resip::Contents> contents)
{
   return new Request(this, to, method, contents);
}

TestSipEndPoint::Request* 
TestSipEndPoint::info(const TestSipEndPoint* endPoint) 
{
   return new Request(this, endPoint->mAor, resip::INFO);
}

TestSipEndPoint::Request* 
TestSipEndPoint::info(const TestUser& endPoint) 
{
   return new Request(this, endPoint.getAddressOfRecord(), resip::INFO);
}

TestSipEndPoint::Request* 
TestSipEndPoint::info(const Uri& url) 
{
   return new Request(this, url, resip::INFO);
}

TestSipEndPoint::Request* 
TestSipEndPoint::info(const Uri& url, const boost::shared_ptr<resip::Contents>& contents) 
{
   return new Request(this, url, resip::INFO, contents);
}

TestSipEndPoint::Request* 
TestSipEndPoint::message(const TestSipEndPoint* endPoint, const Data& text) 
{
   PlainContents* plain = new PlainContents;
   plain->text() = text;
   boost::shared_ptr<resip::Contents> body(plain);   
   return new Request(this, endPoint->mAor, resip::MESSAGE, body);
}

TestSipEndPoint::Request* 
TestSipEndPoint::message(const TestUser& endPoint, const Data& text) 
{
   PlainContents* plain = new PlainContents;
   plain->text() = text;
   boost::shared_ptr<resip::Contents> body(plain);   
   return new Request(this, endPoint.getAddressOfRecord(), resip::MESSAGE, body);
}

// vk
TestSipEndPoint::Request*
TestSipEndPoint::message(const NameAddr& target, const Data& text, const messageType contentType)
{
   if (contentType == messageCpim)
   {
     CpimContents* cpim = new CpimContents;
     cpim->text() = text;
     boost::shared_ptr<resip::Contents> body(cpim);
     return new Request(this, target.uri(), resip::MESSAGE, body);
   }
   else // if (contentType == textPlain)
   {
     PlainContents* plain = new PlainContents;
     plain->text() = text;
     boost::shared_ptr<resip::Contents> body(plain);
     return new Request(this, target.uri(), resip::MESSAGE, body);
   }
}
// end - vk

TestSipEndPoint::Request*
TestSipEndPoint::message(const resip::Uri& target, const Data& text)
{
   PlainContents* plain = new PlainContents;
   plain->text() = text;
   boost::shared_ptr<resip::Contents> body(plain);
   return new Request(this, target, resip::MESSAGE, body);
}

TestSipEndPoint::Request*
TestSipEndPoint::message(const resip::Uri& target, const boost::shared_ptr<resip::Contents>& contents)
{
   return new Request(this, target, resip::MESSAGE, contents);
}

// vk
TestSipEndPoint::Request*
TestSipEndPoint::options(const Uri& url)
{
   return new Request(this, url, resip::OPTIONS);
}

TestSipEndPoint::Publish::Publish(TestSipEndPoint* from, const resip::Uri& to, 
                    const resip::Token& eventPackage, 
                    boost::shared_ptr<resip::Contents> contents,
                    int pExpires, 
                    const std::string PAssertedIdentity,
                    const std::string pSIPIfEtag)
   : MessageAction(*from, to),
     mType(resip::PUBLISH),
     mContents(contents),
     mEventPackage(eventPackage),
     mExpires(pExpires),
     mPAssertedIdentity(PAssertedIdentity),
     mSIPIfEtag(pSIPIfEtag)
{}

TestSipEndPoint::Publish::Publish(TestSipEndPoint* from, 
                                  const resip::Uri& to, 
                                  resip::MethodTypes type,
                                  boost::shared_ptr<resip::Contents> contents)
   : MessageAction(*from, to),
     mType(type),
     mContents(contents)
{}


TestSipEndPoint::Publish*
TestSipEndPoint::publish(const Uri& url, const Token& eventPackage, 
                         const int pExpires, const string PAssertedIdentity, 
                         const string publishBody, const string pSIPIfEtag)
{
   /* 
   WHAT PREVIOUSLY WORKED BUT NOW CORES...
   Data text(publishBody);
   HeaderFieldValue hfv(text.data(), text.size());
   Mime type("application", "pidf+xml");
   Pidf pc(&hfv, type);
   boost::shared_ptr<resip::Contents> body(&pc);
   
   ....
   WHAT NOW NEEDS TO BE DONE INSTEAD..
   */
   Data* text = new Data(publishBody);
   HeaderFieldValue hfv(text->data(), text->size());
   Mime type("application", "pidf+xml");
   Pidf* pc = new Pidf(hfv, type);
   boost::shared_ptr<resip::Pidf> body(pc);
   /*
   */

   Publish* localPublish = new Publish(this, url, eventPackage, 
                                       body, pExpires, PAssertedIdentity,
                                       pSIPIfEtag);

   DebugLog(<< "1. START OF MESSAGE" );
   DebugLog(<< localPublish->toString() );
   DebugLog(<< endl << "1. END OF MESSAGE" );

   return localPublish ; 
}

TestSipEndPoint::Publish*
TestSipEndPoint::publish(const resip::NameAddr& target, const resip::Data& text)
{
   HeaderFieldValue hfv(text.data(), text.size());
   Mime type("application", "pidf+xml");
   Pidf pc(hfv, type);


   boost::shared_ptr<resip::Contents> body(&pc);
   return new Publish(this, target.uri(), resip::PUBLISH, body);
}

resip::Data
TestSipEndPoint::Publish::toString() const
{
   resip::Data buffer;
   {
      DataStream strm(buffer);
      strm << mEndPoint.getName() << "." << getMethodName(mType) << "(" << mTo << ")";
      strm.flush();
   }
   return buffer;
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Publish::go()
{
#if 0
   PUBLISH SHOULD NEVER OCCUR INSIDE A DIALOG. PERIOD END OF STORY.

   // !jf! this really should be passed in a dialog-id to identify which dialog
   // to use
   DeprecatedDialog* dialog = mEndPoint.getDialog();
   if (dialog)
   {
      shared_ptr<SipMessage> request(dialog->makeRequest(mType));
      if (mContents.get() != 0) request->setContents(mContents.get());
      request->header(h_Expires).value() = 3600;

/* 
      TEMPORARILY INTRODUCED AS A PATCH JOB TO INVESTIGATE SOMETHING

      request->header(h_Expires).value() = mExpires;
      request->header(h_Event) = mEventPackage ;
      request->header(h_From).uri() = request->header(h_To).uri();
      request->header(h_Contacts).clear();

      if (mPAssertedIdentity.size() != 0)
      {
        resip::Data PAssertedId(mPAssertedIdentity);
        NameAddr PAssertedId_NameAddr(PAssertedId);
        request->header(h_PAssertedIdentities).push_back(PAssertedId_NameAddr);
      }
      END OF  TEMP INTRODUCITON - DELETE THIS SECTION JUST AS SOON AS YOU CAN
*/
      return request;   
   }
   else
   {
#endif // 0
      boost::shared_ptr<SipMessage> request(Helper::makeRequest(NameAddr(mTo), 
                                                         NameAddr(mEndPoint.getAddressOfRecord()), 
                                                         mEndPoint.getContact(),
                                                         mType));

      request->header(h_Expires).value() = mExpires;
      request->header(h_Event) = mEventPackage ;
      request->header(h_From).uri() = request->header(h_To).uri();
      request->header(h_Contacts).clear();

      if (mPAssertedIdentity.size() != 0)
      {
        resip::Data PAssertedId(mPAssertedIdentity);
        NameAddr PAssertedId_NameAddr(PAssertedId);
        request->header(h_PAssertedIdentities).push_back(PAssertedId_NameAddr);
      }

      DebugLog(<< "2. START OF MESSAGE" );
      DebugLog(<< *request );
      DebugLog(<< "2. END OF MESSAGE" );
      if (mContents != 0) request->setContents(mContents.get());
      DebugLog(<< "3. START OF MESSAGE" );
      DebugLog(<< *request );
      DebugLog(<< "3. END OF MESSAGE" );
      // request->header(h_Expires).value() = 3600;
      return request;
#if 0
   }
#endif // 0
}

/*
TestSipEndPoint::Request*
TestSipEndPoint::publish(const Uri& url, const Token& eventPackage, 
                         const int pExpires, const string PAssertedIdentity, 
                         const string publishBody)
{
   Data text(publishBody);
   HeaderFieldValue hfv(text.data(), text.size());
   Mime type("application", "pidf+xml");
   Pidf pc(&hfv, type);
   boost::shared_ptr<resip::Contents> body(&pc);

   Request* localPublish = new Request(this, url, resip::PUBLISH, body);
   // TODO: Get these in...
   // localPublish->mMsg->header(h_Event) = eventPackage;
   // localPublish->mMsg->header(h_Expires) = pExpires;
   return localPublish ; 
}

TestSipEndPoint::Request*
TestSipEndPoint::publish(const resip::NameAddr& target, const resip::Data& text)
{
   HeaderFieldValue hfv(text.data(), text.size());
   Mime type("application", "pidf+xml");
   Pidf pc(&hfv, type);


   boost::shared_ptr<resip::Contents> body(&pc);
   return new Request(this, target.uri(), resip::PUBLISH, body);
}
*/
// end - vk

TestSipEndPoint::Retransmit::Retransmit(TestSipEndPoint& endPoint, 
                                        boost::shared_ptr<resip::SipMessage>& msg)
   : MessageExpectAction(endPoint),
     mMsgToRetransmit(msg)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Retransmit::go(boost::shared_ptr<resip::SipMessage>)
{
   return mMsgToRetransmit;
}

resip::Data
TestSipEndPoint::Retransmit::toString() const
{
   return mEndPoint.getName() + ".retransmit()";
}

TestSipEndPoint::Retransmit* 
TestSipEndPoint::retransmit(boost::shared_ptr<resip::SipMessage>& msg)
{ 
   return new Retransmit(*this, msg);
}


TestSipEndPoint::CloseTransport::CloseTransport(TestSipEndPoint* endPoint)
   : mEndPoint(endPoint)
{
}
            
void 
TestSipEndPoint::CloseTransport::operator()(boost::shared_ptr<Event> event)
{
   operator()();
}

void
TestSipEndPoint::CloseTransport::operator()()
{
   resip_assert(dynamic_cast<UdpTransport*>(mEndPoint->mTransport) == 0);

   InfoLog(<< mEndPoint->getName() << " closing transport");

   // !dlb! assert(endpoint->hasOwnTransport());
   mEndPoint->unregisterFromTransportDriver();
   delete mEndPoint->mTransport;
   mEndPoint->mTransport = 0;
}

resip::Data
TestSipEndPoint::CloseTransport::toString() const
{
   return mEndPoint->getName() + ".closeTransport()";
}

TestSipEndPoint::CloseTransport* 
TestSipEndPoint::closeTransport()
{ 
   // make sure the transport is not connectionless
   resip_assert(dynamic_cast<UdpTransport*>(mTransport) == 0);
   return new CloseTransport(this);
}


TestSipEndPoint::MessageExpectAction::MessageExpectAction(TestSipEndPoint& from)
   : mEndPoint(from),
     mMsg(),
     mConditioner(TestSipEndPoint::identity),
     mRawConditioner(TestSipEndPoint::raw_identity)
{
}

void
TestSipEndPoint::MessageExpectAction::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());

   if (!sipEvent && !mEndPoint.mLastMessage)
   {
      //!dcm! use last message received if available--may want to have an option
      //to turn off this facility
      ErrLog(<< "a TestSipEndPoint action requires a SipEvent!");
      throw AssertException("requires a SipEvent", __FILE__, __LINE__);
   }
   
   boost::shared_ptr<resip::SipMessage> msg = sipEvent ? sipEvent->getMessage() : mEndPoint.mLastMessage;
      
   mMsg = go(msg); 
   mConditioner(mMsg);  
   if (mMsg != 0)
   {
      DebugLog(<< "sending: " << mMsg->brief());
      mEndPoint.send(mMsg,mRawConditioner);
   }
}

void 
TestSipEndPoint::MessageExpectAction::setConditioner(MessageConditionerFn conditioner)
{
   mConditioner = ChainConditions(conditioner, mConditioner);
}

void 
TestSipEndPoint::MessageExpectAction::setRawConditioner(RawConditionerFn conditioner)
{
   mRawConditioner = ChainRawConditions(conditioner, mRawConditioner);
}

TestSipEndPoint::RawReply::RawReply(TestSipEndPoint& from,
                                    const resip::Data& rawText)
   : MessageExpectAction(from),
     mRawText(rawText)
{
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::RawReply::go(boost::shared_ptr<SipMessage> msg)
{                                                                          
   // set via, from, to off msg?
   boost::shared_ptr<SipMessage> mmsg(SipMessage::make(mRawText));
   return mmsg;
}                                                                       

TestSipEndPoint::Send300::Send300(TestSipEndPoint & endpoint, std::set<resip::NameAddr> alternates)
   : MessageExpectAction(endpoint),
     mAlternates(alternates),
     mEndpoint(endpoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send300::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   boost::shared_ptr<resip::SipMessage> response = mEndpoint.makeResponse(*msg, 300);
   while(!response->header(h_Contacts).empty())
   {
      response->header(h_Contacts).pop_front();
   }
   for(std::set<resip::NameAddr>::const_iterator i=mAlternates.begin();i!=mAlternates.end();++i)
   {
      response->header(h_Contacts).push_back(*i);
   }
   
   return response;
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send300(std::set<resip::NameAddr> alternates)
{
   return new Send300(*this,alternates);
}

// vk

// 301

TestSipEndPoint::Send301::Send301(TestSipEndPoint & endPoint, std::set<resip::NameAddr> alternates)
   : MessageExpectAction(endPoint),
     mAlternates(alternates),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send301::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 301);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send301(std::set<resip::NameAddr> alternates)
{
   return new Send301(*this, alternates);
}

// 400

TestSipEndPoint::Send400::Send400(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send400::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 400);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send400()
{
   return new Send400(*this);
}

// 407

TestSipEndPoint::Send407::Send407(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send407::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 407);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send407()
{
   return new Send407(*this);
}

// 408

TestSipEndPoint::Send408::Send408(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send408::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 408);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send408()
{
   return new Send408(*this);
}

// 410

TestSipEndPoint::Send410::Send410(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send410::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 410);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send410()
{
   return new Send410(*this);
}

// 482

TestSipEndPoint::Send482::Send482(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send482::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 482);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send482()
{
   return new Send482(*this);
}

// 483

TestSipEndPoint::Send483::Send483(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send483::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 483);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send483()
{
   return new Send483(*this);
}


// 500

TestSipEndPoint::Send500WithRetryAfter::Send500WithRetryAfter(TestSipEndPoint & endPoint, int retryAfter)
   : MessageExpectAction(endPoint),
     mRetryAfter(retryAfter),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send500WithRetryAfter::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*msg, 500);
   response->header(h_RetryAfter).value() = mRetryAfter;
   // response->header(h_RetryAfter).comment() = "Service Unavailable";
   return response;
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send500WithRetryAfter(int retryAfter)
{
   return new Send500WithRetryAfter(*this, retryAfter);
}


// 503

TestSipEndPoint::Send503WithRetryAfter::Send503WithRetryAfter(TestSipEndPoint & endPoint, int retryAfter)
   : MessageExpectAction(endPoint),
     mRetryAfter(retryAfter),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send503WithRetryAfter::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*msg, 503);
   response->header(h_RetryAfter).value() = mRetryAfter;
   // response->header(h_RetryAfter).comment() = "Service Unavailable";
   return response;
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send503WithRetryAfter(int retryAfter)
{
   return new Send503WithRetryAfter(*this, retryAfter);
}

// end - vk

TestSipEndPoint::Send302::Send302(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mRedirectTo(0)
{
}

TestSipEndPoint::Send302::Send302(TestSipEndPoint& endPoint, const resip::Uri& redirectTo)
: MessageExpectAction(endPoint),
  mEndPoint(endPoint),
  mRedirectTo(new resip::Uri(redirectTo))
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send302::go(boost::shared_ptr<resip::SipMessage> msg)
{
   resip_assert (msg->isRequest());
   boost::shared_ptr<resip::SipMessage> resp = mEndPoint.makeResponse(*msg, 302);
   if (mRedirectTo.get())
   {
      resp->header(h_Contacts).clear();
      resp->header(h_Contacts).push_back(NameAddr(*mRedirectTo));
   }
   return resp;
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send302()
{
   return new Send302(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send302(const resip::Uri& redirectTarget)
{
   return new Send302(*this, redirectTarget);
}


TestSipEndPoint::Send202ToSubscribe::Send202ToSubscribe(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::Send202ToSubscribe::go(boost::shared_ptr<resip::SipMessage> msg)                      
{
   boost::shared_ptr<resip::SipMessage> subscribe;
   subscribe = mEndPoint.getReceivedSubscribe(msg->header(resip::h_CallId));
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*subscribe, 202);

   if (subscribe->exists(h_Expires))
   {
      response->header(h_Expires).value() = subscribe->header(h_Expires).value();
   }

   return response;
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send202ToSubscribe()
{
   return new Send202ToSubscribe(*this);
}

TestSipEndPoint::Send200ToPublish::Send200ToPublish(TestSipEndPoint& endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send200ToPublish::go(boost::shared_ptr<resip::SipMessage> msg)
{
   boost::shared_ptr<resip::SipMessage> publish;
   publish = mEndPoint.getReceivedPublish(msg->header(resip::h_CallId));
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*publish, 200);

   if (publish->exists(h_Expires))
   {
      response->header(h_Expires).value() = publish->header(h_Expires).value();
   }

   response->header(h_SIPETag).value() = Data(resip::Random::getRandom());

   return response;
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send200ToPublish()
{
   return new Send200ToPublish(*this);
}

TestSipEndPoint::Send423Or200ToPublish::Send423Or200ToPublish(TestSipEndPoint& endPoint, int minExpires)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mMinExpires(minExpires)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send423Or200ToPublish::go(boost::shared_ptr<resip::SipMessage> msg)
{
   boost::shared_ptr<resip::SipMessage> publish;
   publish = mEndPoint.getReceivedPublish(msg->header(resip::h_CallId));
   boost::shared_ptr<resip::SipMessage> response;

   resip_assert(publish->exists(h_Expires));
   
   if (publish->header(h_Expires).value() < mMinExpires)
   {
      response = mEndPoint.makeResponse(*publish, 423);
      response->header(h_MinExpires).value() = mMinExpires;
   }
   else
   {
      response = mEndPoint.makeResponse(*publish, 200);
      response->header(h_SIPETag).value() = Data(resip::Random::getRandom());
      response->header(h_Expires).value() = publish->header(h_Expires).value();
   }

   return response;
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send423Or200ToPublish(int minExpires)
{
   return new Send423Or200ToPublish(*this, minExpires);
}

TestSipEndPoint::Send401::Send401(TestSipEndPoint& endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send401::go(boost::shared_ptr<resip::SipMessage> msg)
{
   boost::shared_ptr<resip::SipMessage> response(mEndPoint.makeResponse(*msg, 401));

   Auth auth;
   auth.scheme() = "Digest";
   auth.param(p_nonce) = resip::Random::getCryptoRandomHex(8);
   auth.param(p_algorithm) = "MD5";
   auth.param(p_realm) = "localhost";
   auth.param(p_qopOptions) = "auth";
   auth.param(p_opaque) = "0000000000000000";

   response->header(h_WWWAuthenticates).push_back(auth);

   return response;
}

TestSipEndPoint::Send200ToRegister::Send200ToRegister(TestSipEndPoint& endPoint, const NameAddr& contact) :
   MessageExpectAction(endPoint),
   mEndPoint(endPoint),
   mUseContact(true),
   mContact(contact)
{}

TestSipEndPoint::Send200ToRegister::Send200ToRegister(TestSipEndPoint& endPoint) :
   MessageExpectAction(endPoint),
   mEndPoint(endPoint),
   mUseContact(false),
   mContact()
{}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send200ToRegister::go(boost::shared_ptr<resip::SipMessage> msg)
{
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*msg, 200);

   response->remove(h_Contacts);

   // check whether we need to remove bindings
   bool bExpires = false;
   if( msg->header(h_Contacts).front().exists(p_expires) && msg->header(h_Contacts).front().param(p_expires) == 0 )
      bExpires = true;
   if( ! bExpires && msg->exists(h_Expires) && msg->header(h_Expires).value() == 0 )
      bExpires = true;

   if( !bExpires )
   {
      response->header(h_Contacts).push_back(msg->header(h_Contacts).front());
      response->header(h_Contacts).front().param(p_expires) = 3600;
   }

   // rport already in Via header, add received
   response->header(h_Vias).front().param(p_received) = msg->header(h_Contacts).front().uri().host();

   // add rport and received to via
   if( mUseContact )
   {
      if (! mContact.uri().host().empty())
         response->header(h_Vias).front().param(p_received) = mContact.uri().host();
      else
         response->header(h_Vias).front().remove(p_received);
      response->header(h_Vias).front().param(p_rport).port() = mContact.uri().port();
   }

   return response;
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send200ToRegister(const NameAddr& contact)
{
   return new Send200ToRegister(*this, contact);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send200ToRegister()
{
   return new Send200ToRegister(*this);
}

TestSipEndPoint::Notify::Notify(TestSipEndPoint & endPoint, boost::shared_ptr<resip::Contents> contents, 
                                const resip::Data& eventPackage, const resip::Data& subscriptionState, int expires, int minExpires, bool firstNotify)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mContents(contents),
     mEventPackage(eventPackage),
     mSubscriptionState(subscriptionState),
     mExpires(expires),
     mMinExpires(minExpires),
     mFirstNotify(firstNotify)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::Notify::go(boost::shared_ptr<resip::SipMessage> msg)
{
   UInt32 expires = msg->header(h_Expires).value();
   if (msg->isRequest() && (expires < mMinExpires))
   {
      boost::shared_ptr<resip::SipMessage> response;
      response = mEndPoint.makeResponse(*msg, 423);
      response->header(h_MinExpires).value() = mMinExpires;
      return response;
   }
   else
   {
      resip::Data remoteTag(msg->isRequest() ? msg->header(h_From).param(p_tag) :
                                               msg->header(h_To).param(p_tag) );
      DeprecatedDialog* dialog = mEndPoint.getDialog(msg->header(h_CallId),remoteTag);
      resip_assert(dialog);
      boost::shared_ptr<SipMessage> notify(dialog->makeNotify());
      notify->setContents(mContents.get());
      notify->header(h_Event).value() = mEventPackage;
      notify->header(h_SubscriptionState).value() = mSubscriptionState;
      notify->header(h_SubscriptionState).param(p_expires) = mExpires;

      if (mFirstNotify)
      {
         notify->header(h_CSeq).sequence() = 0;
      }

      return notify;
   }
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::notify(boost::shared_ptr<resip::Contents> contents, const resip::Data& eventPackage, const resip::Data& subscriptionState, int expires, int minExpires, bool firstNotify)
{
   return new Notify(*this, contents, eventPackage, subscriptionState, expires, minExpires, firstNotify);
}

TestSipEndPoint::Respond::Respond(TestSipEndPoint& endPoint,
                                  int responseCode)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mCode(responseCode)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Respond::go(boost::shared_ptr<resip::SipMessage> msg)
{
   DebugLog(<< "TestSipEndPoint::Respond::go");
   /* !jf! use the dialog */
   if (msg->isRequest() && mCode != 487)
   {
      return mEndPoint.makeResponse(*msg, mCode);
   }
   else
   {
      boost::shared_ptr<resip::SipMessage> invite;
      invite = mEndPoint.getReceivedInvite(msg->header(resip::h_CallId));
      if (invite == boost::shared_ptr<SipMessage>())
      {
        return mEndPoint.makeResponse(*msg, mCode);
      }
      else
      {
        return mEndPoint.makeResponse(*invite, mCode);
      }
   }
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::respond(int code)
{
   return new Respond(*this, code);
}

TestSipEndPoint::Answer::Answer(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::Answer::go(boost::shared_ptr<resip::SipMessage> msg)                                
{                                                                           
   boost::shared_ptr<resip::SipMessage> invite;                         
   invite = mEndPoint.getReceivedInvite(msg->header(resip::h_CallId));  
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*invite, 200);
   
   const resip::SdpContents* sdp=0;
   if( mSdp.get() )
   {
      sdp = dynamic_cast<const resip::SdpContents*>(mSdp.get());
      response->setContents(sdp);
   }
   else
   {
      //sdp = dynamic_cast<const resip::SdpContents*>(invite->getContents());
   }

   return response;
}                                                                           

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::answer()
{
   return new Answer(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::answer(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new Answer(*this, sdp);
}


TestSipEndPoint::AnswerUpdate::AnswerUpdate(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::AnswerUpdate::go(boost::shared_ptr<resip::SipMessage> msg)                                
{                                                                           
   boost::shared_ptr<resip::SipMessage> update;                         
   update = mEndPoint.getReceivedUpdate(msg->header(resip::h_CallId));  
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*update, 200);
   
   const resip::SdpContents* sdp=0;
   if( mSdp.get() )
   {
      sdp = dynamic_cast<const resip::SdpContents*>(mSdp.get());
      response->setContents(sdp);
   }
   return response;
}                                                                           

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::answerUpdate()
{
   return new AnswerUpdate(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::answerUpdate(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new AnswerUpdate(*this, sdp);
}

TestSipEndPoint::AnswerTo::AnswerTo(
   TestSipEndPoint& endPoint,
   const boost::shared_ptr<resip::SipMessage>& msg,
   boost::shared_ptr<resip::SdpContents> sdp
) :
   MessageAction(endPoint, Uri()),
   mMsg(msg),
   mSdp(sdp)
{}

boost::shared_ptr<SipMessage>
TestSipEndPoint::AnswerTo::go()
{
   boost::shared_ptr<resip::SipMessage> invite;
   invite = mEndPoint.getReceivedInvite(mMsg->header(resip::h_CallId));
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*invite, 200);
   const resip::SdpContents* sdp;
   if( mSdp.get() )
      sdp = dynamic_cast<const resip::SdpContents*>(mSdp.get());
   else
      sdp = dynamic_cast<const resip::SdpContents*>(invite->getContents());
   response->setContents(sdp);
   return response;
}

resip::Data
TestSipEndPoint::AnswerTo::toString() const
{
   return mEndPoint.getName() + ".answer()";
}


TestSipEndPoint::MessageAction*
TestSipEndPoint::answerTo(
   const boost::shared_ptr<resip::SipMessage>& invite,
   boost::shared_ptr<resip::SdpContents> sdp
)
{
   return new AnswerTo(*this, invite, sdp);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ring()
{
   return new Ring(*this);
}

TestSipEndPoint::RingNewBranch::RingNewBranch(TestSipEndPoint& endPoint)
: MessageExpectAction(endPoint),
  mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::RingNewBranch::go(boost::shared_ptr<resip::SipMessage> msg)                                
{
   boost::shared_ptr<resip::SipMessage> inv = mEndPoint.getReceivedInvite(msg->header(resip::h_CallId));
   //boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*inv, 183);

   SipMessage& request = *inv;

   resip_assert(request.isRequest());
   int responseCode = 180;
   if (request.header(h_RequestLine).getMethod() == INVITE)
   {
      //DeprecatedDialog* dialog = getDialog(request.header(h_CallId));
      //if (!dialog)
      //{
      DebugLog(<< "making a dialog, contact: " << mEndPoint.getContact());
      DeprecatedDialog* dialog = new DeprecatedDialog(mEndPoint.getContact());
      mEndPoint.mDialogs.push_back(dialog);
      DebugLog(<< "made a dialog (" << dialog << "), contact: " << mEndPoint.getContact());
      //}
      DebugLog(<< "Creating response using dialog: " << dialog);
      boost::shared_ptr<SipMessage> response(dialog->makeResponse(request, responseCode));

      return response;
   }
   else
   {
      DebugLog(<<"RingNewBranch error -- original request must be an INVITE!!!");
      throw AssertException("RingNewBranch error -- original request must be an INVITE!!!",
                              __FILE__, __LINE__);
   }

   return boost::shared_ptr<SipMessage>((SipMessage*)0);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ringNewBranch()
{
   return new RingNewBranch(*this);
}

TestSipEndPoint::Ring183::Ring183(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp, bool removeContact)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp),
     mReliable(false),
     mRseq(0),
     mRemoveContact(removeContact)
{
}

TestSipEndPoint::Ring183::Ring183(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp, int rseq)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp),
     mReliable(true),
     mRseq(rseq),
     mRemoveContact(false)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::Ring183::go(boost::shared_ptr<resip::SipMessage> msg)                                
{
   boost::shared_ptr<resip::SipMessage> inv = mEndPoint.getReceivedInvite(msg->header(resip::h_CallId));
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*inv, 183);

   if (mReliable)
   {
      bool required = inv->exists(h_Requires) && inv->header(h_Requires).find(Token(Symbols::C100rel));
      bool supported = inv->exists(h_Supporteds) && inv->header(h_Supporteds).find(Token(Symbols::C100rel));

      if ( mReliable && (!required && !supported) )
      {
         InfoLog (<< "Supported header: " << Inserter(inv->header(h_Supporteds))
                  << " : " 
                  << inv->header(h_Supporteds).find(Token(Symbols::C100rel)));

         throw AssertException("Trying to send reliable provisional when UAC doesn't support it",
                               __FILE__, __LINE__);
      }
      if (!mReliable && required)
      {
         throw AssertException("Failing to send reliable provisional when UAC requires it",
                               __FILE__, __LINE__);
      }

//       if (mReliable && msg->header(h_RequestLine).method() != INVITE)
//       {
//          throw AssertException("Requesting reliable provisional on non-INVITE method",
//                                __FILE__, __LINE__);
//       }
      
      if (mReliable)
      {
         response->header(h_Requires).push_back(Token(Symbols::C100rel));
         response->header(h_RSeq).value() = mRseq;
      }
   }

   if( mSdp.get() )
   {
      response->setContents(mSdp.get());
   }

   if (mRemoveContact)
   {
      response->remove(h_Contacts);
   }
   return response;
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ring183()
{
   return new Ring183(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ring183(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new Ring183(*this, sdp);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ring183_missingContact(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new Ring183(*this, sdp, true);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::reliableProvisional(const boost::shared_ptr<resip::SdpContents>& sdp, int rseq)
{
   if (rseq <= 0)
   {
      throw AssertException("RSEQ value must be greater than 0",
                            __FILE__, __LINE__);
   }
   return new Ring183(*this, sdp, rseq);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::reliableProvisional(int rseq)
{
   if (rseq <= 0)
   {
      throw AssertException("RSEQ value must be greater than 0",
                            __FILE__, __LINE__);
   }
   return new Ring183(*this, boost::shared_ptr<resip::SdpContents>(), rseq);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ok()
{
   return new Ok(*this);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send401()
{
   return new Send401(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send403()
{
   return new Send403(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send404()
{
   return new Send404(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send415()
{
   return new Send415(*this);
}

TestSipEndPoint::Send420::Send420(TestSipEndPoint & endPoint, const Token& unsupported)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mUnsupported(unsupported)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::Send420::go(boost::shared_ptr<resip::SipMessage> msg)                                
{
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*msg, 420);
   response->header(h_Unsupporteds).push_back(mUnsupported);
   return response;
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send420(const resip::Token& unsupported)
{
   return new Send420(*this, unsupported);
}


TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send480()
{
   return new Send480(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send486()
{
   return new Send486(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send487()
{
   return new Send487(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send488()
{
   return new Send488(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send491()
{
   return new Send491(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send202()
{
   return new Send202(*this);
}

// vk
TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::send200()
{
   return new Send200(*this);
}
// end - vk

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send100()
{
   return new Send100(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send503()
{
   return new Send503(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send500()
{
   return new Send500(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send513()
{
   return new Send513(*this);
}

// vk
TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send501()
{
   return new Send501(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send502()
{
   return new Send502(*this);
}
// end- vk

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send504()
{
   return new Send504(*this);
}

// vk
TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send506()
{
   return new Send506(*this);
}
// end- vk

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send600()
{
   return new Send600(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send603()
{
   return new Send603(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send604()
{
   return new Send604(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send606()
{
   return new Send606(*this);
}


TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::dump()
{
   return new Dump(*this);
}

TestSipEndPoint::Ack::Ack(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp)
{
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Ack::go(boost::shared_ptr<SipMessage> response)
{
   resip_assert(response->isResponse());
   int code = response->header(h_StatusLine).responseCode();
   boost::shared_ptr<SipMessage> invite;
   try
   {
      invite = mEndPoint.getSentInvite(response->header(h_CallId));
   }
   catch(...)
   {
      if(!mEndPoint.mRequests.empty())
      {
         invite = mEndPoint.mRequests.begin()->second;
      }
   }
   

   if (code == 200)
   {
      DeprecatedDialog* dialog = mEndPoint.getDialog(response->header(h_CallId),
                                                      response->header(h_To).param(p_tag));
      if(dialog)
      {
         DebugLog(<< "Constructing ack against 200 using dialog.");
         DebugLog(<< *dialog);
         // !dlb! should use contact from 200?
         boost::shared_ptr<SipMessage> ack(dialog->makeAck(*invite));
         if( mSdp.get() )
            ack->setContents(mSdp.get());
         return ack;
      }
      else
      {
         DebugLog(<< "Constructing ack against 200 using Helper.");
         // Allow garbage ACK (ACK to non-INVITE)
         MethodTypes method=invite->method();
         invite->header(h_RequestLine).method()=INVITE;
         invite->header(h_CSeq).method()=INVITE;
         boost::shared_ptr<SipMessage> ack(Helper::makeFailureAck(*invite,*response));
         // Reset tid
         ack->header(h_Vias).front().param(p_branch).reset();
         // Reset request-line
         ack->header(h_RequestLine).uri()=response->header(h_Contacts).front().uri();
         // Set Routes
         if(!response->empty(h_RecordRoutes))
         {
            ack->header(h_Routes)=response->header(h_RecordRoutes).reverse();
         }
         if( mSdp.get() )
            ack->setContents(mSdp.get());
         invite->header(h_RequestLine).method()=method;
         invite->header(h_CSeq).method()=method;
         return ack;
      }
   }
   else
   {
      DebugLog(<<"Constructing failure ack.");
      // Allow garbage ACK (ACK to non-INVITE)
      MethodTypes method=invite->method();
      invite->header(h_RequestLine).method()=INVITE;
      invite->header(h_CSeq).method()=INVITE;
      boost::shared_ptr<SipMessage> ack(Helper::makeFailureAck(*invite, *response));
      invite->header(h_RequestLine).method()=method;
      invite->header(h_CSeq).method()=method;
      ack->header(h_Vias).front().param(p_branch).reset(response->getTransactionId());
      return ack;
   }
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::ack()
{
   return new Ack(*this);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::ack(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new Ack(*this, sdp);
}

TestSipEndPoint::AckNewTid::AckNewTid(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp)
{
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::AckNewTid::go(boost::shared_ptr<SipMessage> response)
{
   resip_assert(response->isResponse());
   int code = response->header(h_StatusLine).responseCode();
   boost::shared_ptr<SipMessage> invite = mEndPoint.getSentInvite(response->header(h_CallId));
   resip_assert (invite->header(h_RequestLine).getMethod() == INVITE);

   boost::shared_ptr<SipMessage> ack;

   if (code == 200)
   {
      DebugLog(<< "Constructing ack against 200 using dialog.");
      DeprecatedDialog* dialog = mEndPoint.getDialog(response->header(h_CallId),
                                                      response->header(h_To).param(p_tag));
      resip_assert (dialog);
      DebugLog(<< *dialog);
      // !dlb! should use contact from 200?
      ack.reset(dialog->makeAck(*invite));
      if( mSdp.get() )
         ack->setContents(mSdp.get());
   }
   else
   {
      DebugLog(<<"Constructing failure ack.");
      ack.reset(Helper::makeFailureAck(*invite, *response));
   }
   
   ack->header(h_Vias).front().param(p_branch).reset();
   
   return ack;
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ackNewTid()
{
   return new AckNewTid(*this);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::ackNewTid(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new AckNewTid(*this, sdp);
}

TestSipEndPoint::AckOldTid::AckOldTid(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp)
{
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::AckOldTid::go(boost::shared_ptr<SipMessage> response)
{
   resip_assert(response->isResponse());
   int code = response->header(h_StatusLine).responseCode();
   boost::shared_ptr<SipMessage> invite = mEndPoint.getSentInvite(response->header(h_CallId));
   resip_assert (invite->header(h_RequestLine).getMethod() == INVITE);

   boost::shared_ptr<SipMessage> ack;

   if (code == 200)
   {
      DebugLog(<< "Constructing ack against 200 using dialog.");
      DeprecatedDialog* dialog = mEndPoint.getDialog(response->header(h_CallId),
                                                      response->header(h_To).param(p_tag));
      resip_assert (dialog);
      DebugLog(<< *dialog);
      // !dlb! should use contact from 200?
      ack.reset(dialog->makeAck(*invite));
      if( mSdp.get() )
         ack->setContents(mSdp.get());
   }
   else
   {
      DebugLog(<<"Constructing failure ack.");
      ack.reset(Helper::makeFailureAck(*invite, *response));
   }
   
   resip::Data oldTid(invite->header(h_Vias).front().param(p_branch).getTransactionId());

   ack->header(h_Vias).front().param(p_branch).reset(oldTid);
   return ack;
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ackOldTid()
{
   return new AckOldTid(*this);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::ackOldTid(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new AckOldTid(*this, sdp);
}

TestSipEndPoint::Reflect::Reflect(TestSipEndPoint& endPoint, MethodTypes method, const Uri& to):
   MessageExpectAction(endPoint),
   mEndPoint(endPoint),
   mMethod(method),
   mReqUri(to)
{}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Reflect::go(boost::shared_ptr<resip::SipMessage> msg)
{
   boost::shared_ptr<SipMessage> reflect(static_cast<SipMessage*>(msg->clone()));
   if(mMethod!=UNKNOWN)
   {
      if(reflect->isRequest())
      {
         reflect->header(h_RequestLine).method()=mMethod;
      }

      reflect->header(h_CSeq).method()=mMethod;
   }

   if(reflect->isRequest())
   {
      reflect->header(h_RequestLine).uri()=mReqUri;
   }

   if(mMethod == INVITE)
   {
      mEndPoint.storeSentInvite(reflect);
   }
   return reflect;
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::reflect(const Uri& reqUri,MethodTypes method)
{
   return new Reflect(*this,method,reqUri);
}

TestSipEndPoint::MessageExpectAction*
TestSipEndPoint::reflect(const TestUser& user,MethodTypes method)
{
   return new Reflect(*this,method,user.getAddressOfRecord());
}

#if 0
TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ackReferred()
{
   return new AckReferred(*this);
}
#endif

ExpectAction* 
TestSipEndPoint::bye(const resip::Uri& target)
{
   return new ByeTo(*this, target);
}

ExpectAction* 
TestSipEndPoint::bye(const TestSipEndPoint* target)
{
   return new ByeTo(*this, target->getContact().uri());
}

ExpectAction* 
TestSipEndPoint::bye(const TestSipEndPoint& target)
{
   return new ByeTo(*this, target.getContact().uri());
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::bye()
{
   return new Bye(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::cancel()
{
   return new Cancel(*this);
}

TestSipEndPoint::Prack::Prack(TestSipEndPoint & endPoint, const boost::shared_ptr<resip::SdpContents> sdp)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mSdp(sdp)
{
}

boost::shared_ptr<SipMessage>
TestSipEndPoint::Prack::go(boost::shared_ptr<SipMessage> msg)
{
   resip::Data remoteTag(msg->isRequest() ? msg->header(h_From).param(p_tag) :
                                            msg->header(h_To).param(p_tag) );
   DeprecatedDialog* dialog = mEndPoint.getDialog(msg->header(h_CallId),remoteTag);
   if(!dialog)
   {
      resip::Data localTag(!msg->isRequest() ? msg->header(h_From).param(p_tag):
                                               msg->header(h_To).param(p_tag) );
      dialog=mEndPoint.getDialog(msg->header(h_CallId),localTag);
   }
   resip_assert(dialog);
   boost::shared_ptr<SipMessage> prack(dialog->makeRequest(PRACK));

   // Add RAck header
   prack->header(h_RAck) = mEndPoint.mRelRespInfo;

   // Add body if provided
   if(mSdp.get())
   {
      prack->setContents(mSdp.get());
   }

   return prack;
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::prack()
{
   return new Prack(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::prack(const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new Prack(*this, sdp);
}

ExpectAction* 
TestSipEndPoint::notify200(const resip::Uri& target)
{
   return new Notify200To(*this, target);
}

ExpectAction* 
TestSipEndPoint::notify200(const TestSipEndPoint& target)
{
   return new Notify200To(*this, target.getContact().uri());
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::notify200()
{
   return new Notify200(*this);
}

TestSipEndPoint::SaveMatcher::SaveMatcher(Matcher* matcher, boost::shared_ptr<SipMessage>& msg) :
   mMsg(msg),
   mMatcher(matcher)
{
}

Data
TestSipEndPoint::SaveMatcher::toString() const
{
   return "SaveMatcher: " + mMatcher->toString();
}

bool
TestSipEndPoint::SaveMatcher::isMatch(boost::shared_ptr<resip::SipMessage>& message) const
{
   TestSipEndPoint::SaveMatcher* ncThis = const_cast<TestSipEndPoint::SaveMatcher*>(this);
   ncThis->mMsg = message;
   return mMatcher->isMatch(message);
}

TestSipEndPoint::From::From(const TestSipEndPoint& testEndPoint)
   : mEndPoint(&testEndPoint),
     mProxy(0),
     mContact()
{
   resip_assert(mEndPoint);
}

TestSipEndPoint::From::From(TestProxy& testProxy)
   : mEndPoint(0),
     mProxy(&testProxy),
     mContact()
{
   resip_assert(mProxy);
}

TestSipEndPoint::From::From(const resip::Uri& contact)
   : mEndPoint(0),
     mProxy(0),
     mContact(contact)
{
   resip_assert(!mContact.host().empty());
}

TestSipEndPoint::From::From(const resip::Data& instanceId)
   : mEndPoint(0),
     mProxy(0),
     mContact(),
     mInstanceId(instanceId)
{
   resip_assert(!instanceId.empty());
}


// check if the message is within a known transaction
// check via
// check max-forwards
// check record-route
// check route
// this requires the endpoint to store all sent and received requests, not just
// INVITE and SUBSCRIBE?
bool 
TestSipEndPoint::From::isMatch(boost::shared_ptr<SipMessage>& message) const
{
   //DebugLog(<< "TestSipEndPoint::From::isMatch");
   // check that the from matches the stored agent
   if (!mInstanceId.empty())
   {
      if (message->exists(h_Contacts) && message->header(h_Contacts).size() == 1)
      {
         if (message->header(h_Contacts).front().exists(p_Instance))
         {
            if (message->header(h_Contacts).front().param(p_Instance) == mInstanceId)
            {
               return true;
            }
            else
            {
               DebugLog (<< "instanceId doesn't match " 
                         << message->header(h_Contacts).front().param(p_Instance) << " != " 
                         << mInstanceId);
            }
         }
         else
         {
            DebugLog (<< "no instanceId in contact " << message->header(h_Contacts).front());
         }
      }
      else
      {
         DebugLog (<< "wrong number of contacts in msg ");
      }
      return false;
   }
   else if (mEndPoint || !mContact.host().empty())
   {
      const Uri& localContact = mEndPoint ? mEndPoint->getContact().uri() : mContact;
      if (message->isRequest())
      {
         if (message->exists(h_Contacts) && message->header(h_Contacts).size() == 1)
         {
            if (localContact.getAor() == message->header(h_Contacts).front().uri().getAor())
            {
               DebugLog(<< "matched");
               return true;
            }
         }

         Via& via = message->header(h_Vias).back(); // via of originator
         DebugLog(<< "Trying to match: " << via.sentHost() << ":" << via.sentPort() << " against: " << localContact);
         
         if ((via.sentHost() != localContact.host()
              && !(via.exists(p_received) && via.param(p_received) == localContact.host()))
             || via.sentPort() != localContact.port())
         {
            InfoLog(<< "From::isMatch failed for (endPoint) " 
                    << (mEndPoint ? mEndPoint->getName() : "") 
                    << ". Via did not match in command");
            return false;
         }
         //DebugLog("matched");
         return true;
      }
      else
      {
         return true;
      }
      
      NameAddrs& contacts = message->header(h_Contacts);
      if (contacts.size() == 0)
      {
         InfoLog(<< "From::isMatch failed for (endPoint) " << mEndPoint->getName());
         InfoLog(<< "No contact header was present in the message: " );
         return false;
      }
      if (contacts.size() != 1)
      {
         InfoLog(<< "From::isMatch failed for (endPoint) " << mEndPoint->getName());
         InfoLog(<< "More than one contact header was present in the message: " );
         return false;
      }
      if ( !(localContact.getAor() == contacts.front().uri().getAor()))
      {
         InfoLog(<< "From::isMatch failed for (endPoint) " << mEndPoint->getName());
         InfoLog(<< "Expected: " << localContact << ", got: " << contacts.front().uri().getAor() << " in msg: " );
         return false;
      }
      //DebugLog("matched");
      return true;
   }
   else if (mProxy)
   {
      DebugLog(<< "using proxy->isFromMe");
      return mProxy->isFromMe(*message);
   }
   resip_assert(false);
   return false;   
}

resip::Data 
TestSipEndPoint::From::toString() const
{
   if (mEndPoint) 
   {
      return "from(" + mEndPoint->getName() + ")";
   }
   else if (!mContact.host().empty())
   {
      return "from(" + mContact.getAor() + ")";
   }
   else if (mProxy)
   {
      return "from(" + mProxy->toString() + ")";
   }
   else if (!mInstanceId.empty())
   {
      return "from(" + mInstanceId + ")";
   }

   resip_assert(false);
   return Data::Empty;   
}

TestSipEndPoint::Contact::Contact(const TestSipEndPoint& testEndPoint)
   : mEndPoint(&testEndPoint),
     mProxy(0),
     mContact()
{
   resip_assert(mEndPoint);
}

TestSipEndPoint::Contact::Contact(TestProxy& testProxy)
   : mEndPoint(0),
     mProxy(&testProxy),
     mContact()
{
}

TestSipEndPoint::Contact::Contact(const resip::Uri& contact)
   : mEndPoint(0),
     mProxy(0),
     mContact(contact)
{
}

bool 
TestSipEndPoint::Contact::isMatch(boost::shared_ptr<SipMessage>& message) const
{
   if (message->exists(h_Contacts) &&
       message->header(h_Contacts).size() == 1)
   {
      bool ret; 
      if (mEndPoint)
      {
         DebugLog(<< "Contact::isMatch " << message->header(h_Contacts).front().uri().getAor()
                  << " " << mEndPoint->getContact().uri().getAor());
         ret = message->header(h_Contacts).front().uri().getAor() == mEndPoint->getContact().uri().getAor();
      }
      else if (!mContact.host().empty())
      {
         DebugLog(<< "Contact::isMatch " << message->header(h_Contacts).front().uri().getAor()
                  << " " << mContact.getAor());
         ret =  message->header(h_Contacts).front().uri().getAor() == mContact.getAor();
      }
      else
      {
         DebugLog(<< "Contact::isMatch " << message->header(h_Contacts).front().uri().getAor()
                  << " " << mProxy->getContact().uri().getAor());
         ret =  message->header(h_Contacts).front().uri().getAor() == mProxy->getContact().uri().getAor();
      }

      return ret;
   }

   InfoLog(<< "Contact::isMatch: no contacts");
   return false;
}

resip::Data 
TestSipEndPoint::Contact::toString() const
{
   if (mEndPoint) 
   {
      return "contact(" + mEndPoint->getName() + ")";
   }
   else if (!mContact.host().empty())
   {
      return "contact(" + mContact.getAor() + ")";
   }
   else
   {
      return "contact(" + mProxy->toString() + ")";
   }
}

TestSipEndPoint::Contact*
contact(const TestSipEndPoint& testEndPoint)
{
   return new TestSipEndPoint::Contact(testEndPoint);
}

TestSipEndPoint::Contact*
contact(TestProxy& testProxy)
{
   return new TestSipEndPoint::Contact(testProxy);
}

TestSipEndPoint::Contact*
contact(const TestSipEndPoint* testEndPoint)
{
   resip_assert (testEndPoint);
   return new TestSipEndPoint::Contact(*testEndPoint);
}

TestSipEndPoint::Contact*
contact(TestProxy* testProxy)
{
   return new TestSipEndPoint::Contact(*testProxy);
}

TestSipEndPoint::Contact*
contact(const Uri& contact)
{
   return new TestSipEndPoint::Contact(contact);
}

TestSipEndPoint::Contact*
contact(const NameAddr& contact)
{
   return new TestSipEndPoint::Contact(contact.uri());
}

resip::Data
TestSipEndPoint::HasMessageBodyMatch::toString() const
{
   return "HasMessageBodyMatch";
}

bool
TestSipEndPoint::HasMessageBodyMatch::isMatch(boost::shared_ptr<SipMessage>& message) const
{
   if (message->exists(h_ContentLength) && message->header(h_ContentLength).value())
   {
      return true;
   }
   DebugLog(<< "HasMessageBodyMatch::isMatch failed: no body in the message");
   return false;
}

TestSipEndPoint::HasMessageBodyMatch* hasMessageBodyMatch()
{
   return new TestSipEndPoint::HasMessageBodyMatch();
}

// JF

TestSipEndPoint::UnknownHeaderMatch::UnknownHeaderMatch(const resip::Data& name, const resip::Data& value)
 : mName(name), mValue(value)
{
}

TestSipEndPoint::UnknownHeaderMatch* 
unknownHeaderMatch(const resip::Data& name, const resip::Data& value)
{
    return new TestSipEndPoint::UnknownHeaderMatch(name,value);
}


resip::Data
TestSipEndPoint::UnknownHeaderMatch::toString() const
{
  return "UnknownHeaderMatch("+mName+","+mValue+")";
}


bool
TestSipEndPoint::UnknownHeaderMatch::isMatch(boost::shared_ptr<SipMessage>& message) const
{
  return (   message->exists(resip::UnknownHeaderType(mName)) 
          && message->header(resip::UnknownHeaderType(mName)).front().value() == mValue 
         );
}


boost::shared_ptr<SipMessage>
TestSipEndPoint::Cancel::go(boost::shared_ptr<SipMessage> msg)
{
   boost::shared_ptr<SipMessage> invite;
   try
   {
      invite = mEndPoint.getSentInvite(msg->header(h_CallId));
   }
   catch(...)
   {
      if(!mEndPoint.mRequests.empty())
      {
         invite= mEndPoint.mRequests.begin()->second;
      }
   }
   
   resip_assert(invite!=0);
   // Allow for CANCEL to be sent for non-INVITE.
   MethodTypes method=invite->method();
   invite->header(h_RequestLine).method()=INVITE;
   invite->header(h_CSeq).method()=INVITE;
   boost::shared_ptr<SipMessage> cancel(Helper::makeCancel(*invite));
   invite->header(h_RequestLine).method()=method;
   invite->header(h_CSeq).method()=method;
   return cancel;
}

void 
TestSipEndPoint::NoAction::operator()(boost::shared_ptr<Event> event)
{
}




//---
TestSipEndPoint::ToMatch::ToMatch(const TestSipEndPoint& ep,
                                  const resip::Uri& to)
   : mEndPoint(&ep),
     mTo(to)
{}

Data 
TestSipEndPoint::ToMatch::toString() const
{
   return mEndPoint->getName() + ".toMatch(" + Data::from(mTo) + ")";
}

bool
TestSipEndPoint::ToMatch::passes(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   resip_assert(mEndPoint);

   DebugLog(<< "TestSipEndPoint::ToMatch(" << Data::from(mTo) << " ? " << Data::from(sipEvent->getMessage()->header(h_To)) << ")");
   return (sipEvent->getMessage()->exists(h_To) &&
           sipEvent->getMessage()->header(h_To).uri() == mTo);
}



TestSipEndPoint::ToMatch& 
TestSipEndPoint::toMatch(const resip::Uri& to)
{
   TestSipEndPoint::ToMatch* toMatch = new TestSipEndPoint::ToMatch(*this, to);
   // !dlb! leak...
   return *toMatch;
}

//---

TestSipEndPoint::SipExpect::SipExpect(TestSipEndPoint& endPoint, 
                                      std::pair<resip::MethodTypes, int> msgTypeCode, 
                                      Matcher* matcher,
                                      ExpectPreCon& preCon,
                                      int timeoutMs,
                                      ActionBase* expectAction)
   : mEndPoint(endPoint),
     mMsgTypeCode(msgTypeCode),
     mMatcher(matcher),
     mPreCon(preCon),
     mTimeoutMs(timeoutMs*TestEndPoint::DebugTimeMult()),
     mExpectAction(expectAction)
{
}

TestSipEndPoint::SipExpect::~SipExpect()
{
   delete mExpectAction;
   delete mMatcher;
}

TestEndPoint* 
TestSipEndPoint::SipExpect::getEndPoint() const
{
   return &mEndPoint; 
}

unsigned int 
TestSipEndPoint::SipExpect::getTimeout() const
{
   return mTimeoutMs; 
}

void
TestSipEndPoint::SipExpect::onEvent(TestEndPoint& endPoint, boost::shared_ptr<Event> event)
{
   mExpectAction->exec(event);
}
resip::Data
TestSipEndPoint::SipExpect::getMsgTypeString() const
{
   if (mMsgTypeCode.second != 0)
   {
      return getMethodName(mMsgTypeCode.first) + "/" + resip::Data(mMsgTypeCode.second);
   }
   else
   {
      return getMethodName(mMsgTypeCode.first);
   }
}

EncodeStream& 
TestSipEndPoint::SipExpect::output(EncodeStream& s) const
{
   if (isOptional())
   {
      s << "opt(";
   }

   s << mEndPoint.getName() << ".expect(" << getMsgTypeString() << ", " << mMatcher->toString() << ")";

   if (isOptional())
   {
      s << ")";
   }

   s.flush();
   return s;
}

bool
TestSipEndPoint::SipExpect::isMatch(boost::shared_ptr<Event> event) const
{
   DebugLog (<< "matching: " << *event);
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   if (!sipEvent) return false;
   
   boost::shared_ptr<SipMessage> msg = sipEvent->getMessage();

   TestSipEndPoint* endPoint = dynamic_cast<TestSipEndPoint*>(sipEvent->getEndPoint());
   resip_assert(endPoint);

   // check to
   //DebugLog(<< "Matching stored endpoint: " << mEndPoint << ":" << mEndPoint.getPort() << " against: " << *endPoint << ":" << endPoint->getPort());
   if (&mEndPoint != endPoint) 
   {
      DebugLog(<< "Matching stored endpoint: " << mEndPoint << ":" << mEndPoint.getPort() << " against: " << *endPoint << ":" << endPoint->getPort());
      DebugLog(<< "isMatch failed on endPoint");
      return false;
   }

   // check from
   if (!mMatcher->isMatch(msg)) 
   {
      DebugLog(<< "isMatch failed on matcher");
      return false;
   }
   
   if(!mPreCon.passes(event))
   {
      DebugLog(<< "isMatch failed on precondition: " << mPreCon.toString());
      return false;
   }

   if (msg->isResponse())
   {
      if (msg->header(h_CSeq).method() != mMsgTypeCode.first)
      {
         DebugLog(<< "isMatch failed on method.");
         return false;
      }

      if (msg->header(h_StatusLine).responseCode() != mMsgTypeCode.second)
      {
         DebugLog(<< "isMatch failed on response code: " 
                  << msg->header(h_StatusLine).responseCode() 
                  << ", needed: " << mMsgTypeCode.second);
         return false;
      }

      return true;
   }

   DebugLog(<< "message header: " << msg->header(h_RequestLine).getMethod());
   DebugLog(<< "type code: " << mMsgTypeCode.first << " : " << mMsgTypeCode.second);
   
   bool r = (msg->header(h_RequestLine).getMethod() == mMsgTypeCode.first &&
             mMsgTypeCode.second == 0);
   if (!r)
   {
      DebugLog(<< "isMatch failed on method.");
   }
   return r;
}

resip::Data
TestSipEndPoint::SipExpect::explainMismatch(boost::shared_ptr<Event> event) const
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   boost::shared_ptr<SipMessage> msg = sipEvent->getMessage();

   Data s;
   {
      DataStream str(s);
      str << "Failed expect: received " << msg->brief() 
          << " for " << msg->header(h_To).uri().user()
          << " expected " << getMsgTypeString()
          << " user=" << *sipEvent->getEndPoint();
   }
   
   return s;
}


TestSipEndPoint::SipExpect::Exception::Exception(const resip::Data& msg,
                                                 const resip::Data& file,
                                                 const int line)
   : resip::BaseException(msg, file, line)
{
}

resip::Data 
TestSipEndPoint::SipExpect::Exception::getName() const 
{
   return "TestEndPoint::Expect::Exception";
}

const char* 
TestSipEndPoint::SipExpect::Exception::name() const 
{
   return "TestEndPoint::Expect::Exception";
}

void 
TestSipEndPoint::buildFdSet(FdSet& fdset)
{
   resip_assert(mTransport);
   mTransport->buildFdSet(fdset);
}

void
TestSipEndPoint::process(FdSet& fdset)
{
   mTransport->process(fdset);

   try 
   {
      if (mIncoming.messageAvailable())
      {
         Message* msg = mIncoming.getNext();
         SipMessage* sip = dynamic_cast<SipMessage*>(msg);
         if (sip)
         {
            DebugLog ( << getName() << ":" << getPort() << " got message: " << sip->brief());
            boost::shared_ptr<SipMessage> sipMsg(sip);
            handleEvent(boost::shared_ptr<Event>(new SipEvent(this, sipMsg)));
         }
         else
         {
            delete msg;
         }
      }
   }
   catch (SipExpect::Exception& e)
   {
      DebugLog(<< "Caught: " << e);
      {
         boost::shared_ptr<SequenceSet> sset(getSequenceSet());
         if (sset)
         {
            Data msg;
            {
               DataStream str(msg);
               str << e;
            }
            sset->globalFailure(msg);
         }
      }
   }
   catch (resip::BaseException& e)
   {
      DebugLog (<< "Uncaught VOCAL exception: " << e << "...ignoring");
      
      boost::shared_ptr<SequenceSet> sset(getSequenceSet());
      if (sset)
      {
         Data msg;
         {
            DataStream str(msg);
            str << e;
         }
         sset->globalFailure(msg);
      }
   }
   catch (std::exception& e)
   {
      DebugLog (<< "Uncaught std::exception exception: " << e.what() << "...ignoring");

      boost::shared_ptr<SequenceSet> sset(getSequenceSet());
      if (sset)
      {
         sset->globalFailure(e.what());
      }
   }
   catch (...)
   {
      DebugLog (<< "Uncaught unknown exception ");
      boost::shared_ptr<SequenceSet> sset(getSequenceSet());
      if (sset)
      {
         sset->globalFailure("Uncaught unknown exception ");
      }
   }
}

void 
TestSipEndPoint::handleEvent(boost::shared_ptr<Event> event)
{
#if BOOST_VERSION >= 103500
   boost::shared_ptr<SipEvent> sipEvent = dynamic_pointer_cast<SipEvent>(event);
#else
   boost::shared_ptr<SipEvent> sipEvent = shared_dynamic_cast<SipEvent>(event);
#endif
   resip_assert(sipEvent);
   boost::shared_ptr<SipMessage> msg = sipEvent->getMessage();
   mLastMessage = msg;
   
   DebugLog(<< getContact() << " is handling: " << *msg);
   if (msg->isResponse())
   {
      if (msg->header(h_CSeq).method() == INVITE && 
          msg->header(h_StatusLine).responseCode() > 100 &&
          msg->header(h_StatusLine).responseCode() <= 200)
      {
         boost::shared_ptr<SipMessage> invite = getSentInvite(msg->header(h_CallId));
         //DebugLog (<< "invite map = " << mInvitesSent);
         
         // If this is a reliable provisional, then store the sequence numbers for insertion into
         // the result PRACK message
         if (msg->exists(h_RSeq))
         {
            // store state about the provisional if reliable
            mRelRespInfo.rSequence() = (unsigned int) msg->header(h_RSeq).value();
            mRelRespInfo.cSequence() = msg->header(h_CSeq).sequence();
            mRelRespInfo.method() = msg->header(h_CSeq).method();
         }

         resip_assert(invite != 0);
         
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId),
                                                msg->header(h_To).param(p_tag));
         if (dialog != 0)
         {
            dialog->targetRefreshResponse(*msg);
         }
         else
         {
            DebugLog (<< "received response in UAS: " << *msg);
            DebugLog (<< "original invite: " << *invite);
            
            dialog = new DeprecatedDialog(getContact());
            DebugLog(<<"Creating dialog as UAC from: " << getContact());
            dialog->createDialogAsUAC(*msg);
            DebugLog(<<"Created dialog as UAC from: " << getContact() << ", to: " << dialog->getRemoteTarget());
            mDialogs.push_back(dialog);
         }
      }
      if (msg->header(h_CSeq).method() == SUBSCRIBE && 
          msg->header(h_StatusLine).responseCode()/100 == 2)
      {
         //CerrLog(<< "handling SUBSCRIBE/2xx");
         bool found = false;
         for (SentSubscribeList::iterator i = mSubscribesSent.begin();
              i != mSubscribesSent.end(); ++i)
         {
            if (i->isMatch(msg))
            {
               found = true;
               i->dispatch(msg);
               break;
            }
         }
         if (!found)
         {
            CritLog(<< "no DialogSet for " << *msg);
         }
         resip_assert(found);
         
         boost::shared_ptr<SipMessage> subscribe = getSentSubscribe(msg);
         resip_assert(subscribe != 0);
         
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId),
                                                msg->header(h_To).param(p_tag));
         if (dialog != 0)
         {
            DebugLog(<< "refreshing with SUBSCRIBE/2xx"); // not an error
            dialog->targetRefreshResponse(*msg);
         }
         else
         {
            dialog = new DeprecatedDialog(getContact());
            dialog->createDialogAsUAC(*msg);
            DebugLog(<<"Created dialog as UAC from: " << getContact() << ", to: " << dialog->getRemoteTarget());
            mDialogs.push_back(dialog);
         }
      }
   }
   else if (msg->isRequest())
   { 
      if (msg->exists(h_Vias) &&
          !msg->header(h_Vias).empty() &&
          msg->header(h_Vias).front().exists(p_branch))
      {
         mRequests[msg->header(h_Vias).front().param(p_branch).getTransactionId()] = msg;
      }

      if (msg->header(h_RequestLine).getMethod() == INVITE)
      {
         storeReceivedInvite(msg);
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId),
                                                msg->header(h_From).param(p_tag));
         if (dialog != 0)
         {
            if (dialog->targetRefreshRequest(*msg) != 0)
            {
               throw GlobalFailure("Target refresh failed", __FILE__, __LINE__);
            }
         }
      }
      if (msg->header(h_RequestLine).getMethod() == UPDATE)
      {
         storeReceivedUpdate(msg);
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId),
                                                msg->header(h_From).param(p_tag));
         if (dialog != 0)
         {
            if (dialog->targetRefreshRequest(*msg) != 0)
            {
               throw GlobalFailure("Target refresh failed", __FILE__, __LINE__);
            }
         }
      }
      else if (msg->header(h_RequestLine).getMethod() == SUBSCRIBE)
      {
         storeReceivedSubscribe(msg);
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId),
                                                msg->header(h_From).param(p_tag));
         if (dialog != 0)
         {
            if (dialog->targetRefreshRequest(*msg) != 0)
            {
               throw GlobalFailure("Target refresh failed", __FILE__, __LINE__);
            }
         }
      }
      else if (msg->header(h_RequestLine).getMethod() == PUBLISH)
      {
         storeReceivedPublish(msg);
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId),
                                                msg->header(h_From).param(p_tag));
         if (dialog != 0)
         {
            if (dialog->targetRefreshRequest(*msg) != 0)
            {
               throw GlobalFailure("Target refresh failed", __FILE__, __LINE__);
            }
         }
      }
      else if (msg->header(h_RequestLine).getMethod() == NOTIFY)
      {
         // find SUBSCRIPTION by CallId (+ localtag eventually), match or create Dialog, update dialog
      }
   }
   else
   {
      DebugLog(<<"Unknown message type: " << *msg);
      throw GlobalFailure("Unknown msg type", __FILE__, __LINE__);
   }
   boost::shared_ptr<SequenceSet> sset(getSequenceSet());
   if (sset)
   {
      sset->enqueue(event);
   }
   else
   {
      WarningLog(<< *this << " has no associated SequenceSet: discarding event " << *event);
   }
}

resip::Data 
TestSipEndPoint::getName() const
{
   return mAor.user();
}

int 
TestSipEndPoint::getPort() const 
{
   return mContact.uri().port();         
}

const resip::NameAddr& 
TestSipEndPoint::getContact() const
{
   return mContact;
}

const std::set<resip::NameAddr>& 
TestSipEndPoint::getDefaultContacts() const
{
   return mContactSet;
}
      
const resip::Uri& 
TestSipEndPoint::getAddressOfRecord() const
{
   return mAor;
}

const resip::Uri& 
TestSipEndPoint::getUri() const
{
   return mAor;
}

resip::Data 
TestSipEndPoint::getAddressOfRecordString() const
{
   return mAor.getAor();
}

TestSipEndPoint::SaveMatcher*
saveMatcher(TestSipEndPoint::Matcher* matcher, boost::shared_ptr<SipMessage>& msg)
{
   return new TestSipEndPoint::SaveMatcher(matcher, msg);
}

TestSipEndPoint::From*
from(const TestSipEndPoint& testEndPoint)
{
   return new TestSipEndPoint::From(testEndPoint);
}

TestSipEndPoint::From*
from(TestProxy& testProxy)
{
   return new TestSipEndPoint::From(testProxy);
}

TestSipEndPoint::From*
from(const Uri& contact)
{
   return new TestSipEndPoint::From(contact);
}

TestSipEndPoint::From*
from(const NameAddr& contact)
{
   return new TestSipEndPoint::From(contact.uri());
}

TestSipEndPoint::From*
from(const TestSipEndPoint* testEndPoint)
{
   resip_assert (testEndPoint);
   return new TestSipEndPoint::From(*testEndPoint);
}

TestSipEndPoint::From*
from(TestProxy* testProxy)
{
   return new TestSipEndPoint::From(*testProxy);
}

TestSipEndPoint::From* 
from(const resip::Data& instanceId)
{
   return new TestSipEndPoint::From(instanceId);
}

TestSipEndPoint::AlwaysMatches* 
alwaysMatches()
{
   return new TestSipEndPoint::AlwaysMatches;
}

bool 
TestSipEndPoint::MatchNonceCount::isMatch(boost::shared_ptr<resip::SipMessage>& message) const
{
   if (message->isRequest())
   {
      Auth* checkAgainst = 0;      
      if (message->exists(h_Authorizations))
      {
         checkAgainst = &message->header(h_Authorizations).front();
      }
      else if (message->exists(h_ProxyAuthorizations))
      {
         checkAgainst = &message->header(h_ProxyAuthorizations).front();
      }
      if (checkAgainst && checkAgainst->exists(p_nc))
      {
         bool ret = checkAgainst->param(p_nc).convertInt() == mCount;
         InfoLog(<< "TestSipEndPoint::MatchNonceCount expected: " << mCount << " " 
                 << "got: " << checkAgainst->param(p_nc) << " returning " << ret);
         return ret;
      }
   }
   return false;
}

Data 
TestSipEndPoint::MatchNonceCount::toString() const
{
   Data ret;
   {
      DataStream ds(ret);
      ds << "MatchNonceCount " << mCount;
   }
   return ret;
}

TestSipEndPoint::MessageAction*
condition(TestSipEndPoint::MessageConditionerFn fn, TestSipEndPoint::MessageAction* action)
{
   action->setConditioner(fn);
   return action;
}

TestSipEndPoint::MessageAction*
rawcondition(TestSipEndPoint::RawConditionerFn fn, TestSipEndPoint::MessageAction* action)
{
   action->setRawConditioner(fn);
   return action;
}

TestSipEndPoint::RawSend*
rawcondition(TestSipEndPoint::RawConditionerFn fn, TestSipEndPoint::RawSend* raw)
{
   raw->setRawConditioner(fn);
   return raw;
}

TestSipEndPoint::MessageAction*
save(boost::shared_ptr<resip::SipMessage>& msgPtr,
     TestSipEndPoint::MessageAction* action)
{
   TestSipEndPoint::SaveMessage saver(msgPtr);
   return condition(saver, action);
}

TestSipEndPoint::MessageAction*
operator<=(boost::shared_ptr<resip::SipMessage>& msgPtr, TestSipEndPoint::MessageAction* action)
{
   return save(msgPtr, action);
}

TestSipEndPoint::MessageExpectAction*
condition(TestSipEndPoint::MessageConditionerFn fn, TestSipEndPoint::MessageExpectAction* action)
{
   action->setConditioner(fn);
   return action;
}

TestSipEndPoint::MessageExpectAction*
rawcondition(TestSipEndPoint::RawConditionerFn fn, TestSipEndPoint::MessageExpectAction* action)
{
   action->setRawConditioner(fn);
   return action;
}

TestSipEndPoint::MessageExpectAction*
save(boost::shared_ptr<resip::SipMessage>& msgPtr,
     TestSipEndPoint::MessageExpectAction* action)
{
   TestSipEndPoint::SaveMessage saver(msgPtr);
   return condition(saver, action);
}

OptionTagConditioner::OptionTagConditioner(const resip::Tokens& tags, Location loc) :
   mTags(tags),
   mLocation(loc)
{}

boost::shared_ptr<resip::SipMessage> 
OptionTagConditioner::operator()(boost::shared_ptr<resip::SipMessage> msg)
{
   switch(mLocation)
   {
      case Supported:
         copy(mTags.begin(), mTags.end(), back_insert_iterator<Tokens>(msg->header(h_Supporteds)));
         break;
      case Required:
         copy(mTags.begin(), mTags.end(), back_insert_iterator<Tokens>(msg->header(h_Requires)));
         break;
      default:
         resip_assert(0);
   }
   return msg;
}

TestSipEndPoint::MessageAction*
addSupported(const resip::Tokens& tokens, TestSipEndPoint::MessageAction* action)
{
   return condition(OptionTagConditioner(tokens, OptionTagConditioner::Supported), action);
}

TestSipEndPoint::MessageAction*
addSupported(const resip::Data& tag, TestSipEndPoint::MessageAction* action)
{
   resip::Tokens t;
   t.push_back(resip::Token(tag));
   return condition(OptionTagConditioner(t, OptionTagConditioner::Supported), action);
}

TestSipEndPoint::MessageAction*
addRequired(const resip::Tokens& tokens, TestSipEndPoint::MessageAction* action)
{
   return condition(OptionTagConditioner(tokens, OptionTagConditioner::Required), action);
}

TestSipEndPoint::MessageAction*
addRequired(const resip::Data& tag, TestSipEndPoint::MessageAction* action)
{
   resip::Tokens t;
   t.push_back(resip::Token(tag));
   return condition(OptionTagConditioner(t, OptionTagConditioner::Required), action);
}

TestSipEndPoint::MessageExpectAction*
operator<=(boost::shared_ptr<resip::SipMessage>& msgPtr, TestSipEndPoint::MessageExpectAction* action)
{
   return save(msgPtr, action);
}

TestEndPoint::ExpectBase* 
TestSipEndPoint::expect(MethodTypes msgType, Matcher* matcher, int timeoutMs, ActionBase* expectAction)
{
   return new SipExpect(*this, make_pair(msgType, 0), matcher, *TestEndPoint::AlwaysTruePred, timeoutMs, expectAction);
}
      
TestEndPoint::ExpectBase* 
TestSipEndPoint::expect(MethodTypes msgType, Matcher* matcher, ExpectPreCon& pred, int timeoutMs, ActionBase* expectAction)
{
   return new SipExpect(*this, make_pair(msgType, 0), matcher, pred, timeoutMs, expectAction);
}

TestEndPoint::ExpectBase* 
TestSipEndPoint::expect(pair<MethodTypes, int> msgTypeCode, Matcher* matcher, int timeoutMs, ActionBase* expectAction)
{
   return new SipExpect(*this, msgTypeCode, matcher, *TestEndPoint::AlwaysTruePred, timeoutMs, expectAction);
}
      
TestEndPoint::ExpectBase* 
TestSipEndPoint::expect(pair<MethodTypes, int> msgTypeCode, Matcher* matcher, ExpectPreCon& pred, int timeoutMs, ActionBase* expectAction)
{
   return new SipExpect(*this, msgTypeCode, matcher, pred, timeoutMs, expectAction);
}

Box
TestSipEndPoint::SipExpect::layout() const
{
   mBox.mX = 1;
   mBox.mY = 0;

   //     |
   // ext17->100
   mBox.mHeight = 2;

   Data out;
   {
      DataStream str(out);
      bool previousActive = false;
      prettyPrint(str, previousActive, 0);
   }
   mBox.mWidth = out.size()+2;

   //InfoLog(<< "TestSipEndPoint::SipExpect::layout: " << mBox);

   return mBox;
}

void
TestSipEndPoint::SipExpect::render(CharRaster& out) const
{
   //InfoLog(<< "TestSipEndPoint::SipExpect::render");
   out[mBox.mY][mBox.mX + mBox.mWidth/2] = '|';

   Data s(" ");
   {
      DataStream str(s);

      bool previousActive = false;
      prettyPrint(str, previousActive, 0);
   }
   s += " ";

   //InfoLog(<< "size of SipExpect::render size = " << s.size());
   int x = 0;
   for (unsigned int i=0; i<s.size(); i++)
   {
      out[mBox.mY+1][mBox.mX + x] = s[i];
      x++;
   }
}

std::pair<resip::MethodTypes, int>
operator/(resip::MethodTypes meth,
          int code)
{
   return std::make_pair(meth, code);
}

TestSipEndPoint::MessageConditionerFn
compose(TestSipEndPoint::MessageConditionerFn fn2,
        TestSipEndPoint::MessageConditionerFn fn1)
{
   return TestSipEndPoint::ChainConditions(fn2, fn1);
}

TestSipEndPoint::MessageConditionerFn
compose(TestSipEndPoint::MessageConditionerFn fn3,
        TestSipEndPoint::MessageConditionerFn fn2,
        TestSipEndPoint::MessageConditionerFn fn1)
{
   return compose(fn3, compose(fn2, fn1));
}

TestSipEndPoint::MessageConditionerFn
compose(TestSipEndPoint::MessageConditionerFn fn4,
        TestSipEndPoint::MessageConditionerFn fn3,
        TestSipEndPoint::MessageConditionerFn fn2,
        TestSipEndPoint::MessageConditionerFn fn1)
{
   return compose(fn4, compose(fn3, compose(fn2, fn1)));
}


TestSipEndPoint::RawConditionerFn
rawcompose(TestSipEndPoint::RawConditionerFn fn2,
        TestSipEndPoint::RawConditionerFn fn1)
{
   return TestSipEndPoint::ChainRawConditions(fn2, fn1);
}

TestSipEndPoint::RawConditionerFn
rawcompose(TestSipEndPoint::RawConditionerFn fn3,
        TestSipEndPoint::RawConditionerFn fn2,
        TestSipEndPoint::RawConditionerFn fn1)
{
   return rawcompose(fn3, rawcompose(fn2, fn1));
}

TestSipEndPoint::RawConditionerFn
rawcompose(TestSipEndPoint::RawConditionerFn fn4,
        TestSipEndPoint::RawConditionerFn fn3,
        TestSipEndPoint::RawConditionerFn fn2,
        TestSipEndPoint::RawConditionerFn fn1)
{
   return rawcompose(fn4, rawcompose(fn3, rawcompose(fn2, fn1)));
}


#ifdef RTP_ON
void
TestSipEndPoint::CreateRtpSession::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   shared_ptr<SipMessage> msg = sipEvent->getMessage();

   SdpContents* remoteSdp = dynamic_cast<SdpContents*>(msg->getContents());
   resip_assert(remoteSdp != 0);

   resip_assert(!_localSdp->session().getMedia().empty());
   resip_assert(!remoteSdp->session().getMedia().empty());
   
   int localPort = _localSdp->session().getMedia().front().getPort();
   int remotePort = remoteSdp->session().getMedia().front().getPort();
   
   resip::Data rmtSdpAddr = remoteSdp->session().getOrigin().getAddress();
   
   mEndPoint->mRtpSession = new RtpSession( rmtSdpAddr.c_str(),
                                            remotePort,
                                            localPort,
                                            0,0,
                                            rtpPayloadPCMU,
                                            rtpPayloadPCMU,-1 );
}
resip::Data
TestSipEndPoint::CreateRtpSession::toString() const
{
   return mEndPoint->getName() + ".createRtpSession()";
}

 
TestSipEndPoint::SendDtmf::SendDtmf(TestSipEndPoint* endPoint, const DtmfSequence& dtmfString)
   : mEndPoint(endPoint)
{
   ActionBase* endOfChain = 0;
   ActionBase* startOfChain = 0;
   for ( unsigned int i=0; i < dtmfString.size(); i++ )
   {
      int dtmfDigit;
      switch(dtmfString[i].first)
      {
         case '*' : dtmfDigit = 10; break;
         case '#' : dtmfDigit = 11; break;
         default  : dtmfDigit = (int)(dtmfString[i].first - '0');
      }

      SendDtmfChar* s = new SendDtmfChar(mEndPoint, dtmfDigit);
      Pause* p = new Pause(dtmfString[i].second, mEndPoint);
      
      s->addNext(p);
      if (endOfChain != 0)
      {
         endOfChain->addNext(s);
         startOfChain = s;
      }
      endOfChain = p;
   }   
   this->addNext(startOfChain);
}

void
TestSipEndPoint::SendDtmf::operator()(boost::shared_ptr<Event> event)
{}

void 
TestSipEndPoint::SendDtmf::SendDtmfChar::operator()(TestSipEndPoint& endPoint)
{
   RtpPacket *pkt = mEndPoint->mRtpSession->receive();
   if (pkt)
   {
      DebugLog(<<"Received Rtp Packet");
      delete pkt;
      pkt = 0;
   }
      
   if(_dtmf >= 0 && _dtmf <= 11)
   {
      mEndPoint->mRtpSession->transmitEvent(_dtmf);
      DebugLog(<<"Sending: " << _dtmf << " via DTMF");
   }   
}

#endif // RTP_ON

#if 0
shared_ptr<SdpContents> 
TestSipEndPoint::MakeSdp(const Data& user,
                         const Data& hostname,
                         int port, 
                         const Data& codec)
{
   shared_ptr<SdpContents> sdp = new SdpContents();

   sdp->session().version() = 0;
   sdp->session().origin().user() = user;
   sdp->session().origin().setAddress(hostname);
   sdp->session().name() = "-";

   SdpContents::Session::Medium medium;
   medium.port() = port;

   medium.setConnection(SdpContents::Session::Connection(SdpContents::IP4, hostname));
   sdp->session().addMedium(medium);
   
   return sdp;
}
#endif

// Copyright 2005 Purplecomm, Inc.
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
