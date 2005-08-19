#include <cppunit/TestCase.h>


#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipFrag.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/TlsTransport.hxx"
#include "resip/stack/UdpTransport.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"

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

using namespace resip;
using namespace boost;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

boost::shared_ptr<resip::SipMessage> TestSipEndPoint::nil;
resip::Uri TestSipEndPoint::NoOutboundProxy;

TestSipEndPoint::IdentityMessageConditioner TestSipEndPoint::identity;

TestSipEndPoint::TestSipEndPoint(const Uri& addressOfRecord,
                                 const Uri& contactUrl,
                                 const Uri& outboundProxy,
                                 bool hasStack,
                                 const Data& interface)
   : mAor(addressOfRecord),
     mContact(contactUrl),
     mOutboundProxy(outboundProxy),
     mTransport(0)
{
#ifdef RTP_ON
   mRtpSession = 0;
#endif

   if (hasStack)
   {
      if (!contactUrl.exists(p_transport) ||
          (isEqualNoCase(contactUrl.param(p_transport), Tuple::toData(UDP))))
      {
         //CerrLog(<< "transport is UDP " << interface);
         mTransport = new UdpTransport(mIncoming, mContact.uri().port(), V4, interface);
      }
      else if (isEqualNoCase(contactUrl.param(p_transport), Tuple::toData(TCP)))
      {
         //CerrLog(<< "transport is TCP " << interface);
         mTransport = new TcpTransport(mIncoming, mContact.uri().port(), V4, interface);
      }
/*
      else if (isEqualNoCase(contactUrl.param(p_transport), Tuple::toData(Transport::TLS)))
      {
         mTransport = new TlsTransport(Resolver::getHostName(), mContact.uri().port(), "", mIncoming);
      }
*/
      else
      {
         assert(0);
      }

      assert(mTransport);
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
                                 const Data& interface)
   : mAor(contactUrl),
     mContact(contactUrl),
     mOutboundProxy(outboundProxy),
     mTransport(0)
{
#ifdef RTP_ON
   mRtpSession = 0;
#endif

   DebugLog(<< "TestSipEndPoint::TestSipEndPoint contact: " << mContact);
   if (hasStack)
   {
      if (!contactUrl.exists(p_transport) ||
          (contactUrl.param(p_transport) == Tuple::toData(UDP)))
      {
         //CerrLog(<< "transport is UDP " << interface);
         mTransport = new UdpTransport(mIncoming, mContact.uri().port(), V4, interface);
      }
      else if (contactUrl.param(p_transport) == Tuple::toData(TCP))
      {
         //CerrLog(<< "transport is TCP " << interface);
         mTransport = new TcpTransport(mIncoming, mContact.uri().port(), V4, interface);
      }
/*
      else if (contactUrl.param(p_transport) == Tuple::toData(Transport::TLS))
      {
         mTransport = new TlsTransport(interface, mContact.uri().port(), "", mIncoming);
      }
*/

      assert(mTransport);
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
   mInvitesSent.clear();
   mInvitesReceived.clear();
   mSubscribesSent.clear();
   mSubscribesReceived.clear();
   mRequests.clear();
   
   for (DialogList::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
   {
      delete (*it);
   }
   mDialogs.clear();

#ifdef RTP_ON
   delete mRtpSession;
   mRtpSession = 0;
#endif

   TestEndPoint::clear();
}


void 
TestSipEndPoint::send(shared_ptr<SipMessage>& msg)
{
   const Tuple* useTuple = 0;

   Uri uri;
   if (msg->isRequest())
   {
      assert(!msg->header(h_Vias).empty());
      assert(msg->header(h_Vias).front().exists(p_branch));
      assert(!msg->header(h_Vias).front().param(p_branch).getTransactionId().empty());

      if (msg->header(h_RequestLine).getMethod() != ACK)
      {
         mRequests[msg->header(h_Vias).front().param(p_branch).getTransactionId()] = msg;
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
      assert (!msg->header(h_Vias).empty());
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
      assert(0);
   }
   
   DebugLog(<<"Target uri: " << uri);

   // modifying the via
   if (msg->isRequest())
   {
      assert(!msg->header(h_Vias).empty());
      msg->header(h_Vias).front().remove(p_maddr);
      msg->header(h_Vias).front().transport() = Tuple::toData(mTransport->transport()); 
      msg->header(h_Vias).front().sentHost() = Resolver::getHostName();
      msg->header(h_Vias).front().sentPort() = mTransport->port();
   }
   
   resip::Data& encoded = msg->getEncoded();
   encoded = "";
   DataStream encodeStream(encoded);
   msg->encode(encodeStream);
   encodeStream.flush();
   
   DebugLog (<< "encoded=" << encoded.c_str());

   if (useTuple)
   {
      useTuple->transport->send(*useTuple, encoded, "bogus");
   }
   else
   {
      // send it over the transport
      Resolver r(uri);
      assert (!r.mNextHops.empty());
      
      mTransport->send(r.mNextHops.front(), encoded, "bogus");
   }
}

void TestSipEndPoint::storeSentInvite(const shared_ptr<SipMessage>& invite)
{
   assert(invite->isRequest());
   assert(invite->header(h_RequestLine).getMethod() == INVITE);
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

void TestSipEndPoint::storeReceivedInvite(const shared_ptr<SipMessage>& invite)
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

shared_ptr<SipMessage> 
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
 
shared_ptr<SipMessage> 
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
   return shared_ptr<SipMessage>();
}  

void TestSipEndPoint::storeSentSubscribe(const shared_ptr<SipMessage>& subscribe)
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

void TestSipEndPoint::storeReceivedSubscribe(const shared_ptr<SipMessage>& subscribe)
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

shared_ptr<SipMessage> 
TestSipEndPoint::getSentSubscribe(shared_ptr<SipMessage> msg)
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
   return shared_ptr<SipMessage>();
}
 
shared_ptr<SipMessage> 
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
   return shared_ptr<SipMessage>();
}  

DeprecatedDialog*
TestSipEndPoint::getDialog()
{
   assert(mDialogs.size() <= 1);
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
TestSipEndPoint::getDialog(const CallId& callId)
{
   DebugLog(<< "searching for callid: " << callId);
   for(DialogList::iterator it = mDialogs.begin();
       it != mDialogs.end(); it++)
   {
      DebugLog (<< "Comparing to dialog: " << (*it));
      if ( (*it)->getCallId() == callId)
      {
         DebugLog(<< "Dialog: " << *it << "  " << (*it)->getCallId() << "matched.");
         return *it;
      }
      DebugLog(<< "Dialog: " << *it << "  " << (*it)->getCallId() << "did not match.");
   }
   return 0;
}

DeprecatedDialog* 
TestSipEndPoint::getDialog(const Uri& target)
{
   //DebugLog(<< this->getContact()->encode() << " has " << mDialogs.size() << " dialogs.");
   for(DialogList::iterator it = mDialogs.begin();
       it != mDialogs.end(); it++)
   {
      //DebugLog(<< (*it)->getRemoteTarheader()->encode());
      if ((*it)->getRemoteTarget().uri().getAor() == target.getAor())
      {
         //DebugLog(<< "Matched in getDialogByTarget");
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
shared_ptr<SipMessage>
TestSipEndPoint::makeResponse(SipMessage& request, int responseCode)
{
   assert(request.isRequest());
   if ( responseCode < 300 && responseCode > 100 &&
        (request.header(h_RequestLine).getMethod() == INVITE ||
         request.header(h_RequestLine).getMethod() == SUBSCRIBE))
   {
      DeprecatedDialog* dialog = getDialog(request.header(h_CallId));
      if (!dialog)
      {
         DebugLog(<< "making a dialog, contact: " << getContact());
         dialog = new DeprecatedDialog(getContact());
         mDialogs.push_back(dialog);
         DebugLog(<< "made a dialog (" << dialog << "), contact: " << getContact());
      }
      DebugLog(<< "Creating response using dialog: " << dialog);
      shared_ptr<SipMessage> response(dialog->makeResponse(request, responseCode));
      return response;
   }
   else
   {
      DeprecatedDialog* dialog = getDialog(request.header(h_CallId));
      if (!dialog)
      {
         DebugLog(<<"making response outside of dialog, contact: " << getContact());
         shared_ptr<SipMessage> response(Helper::makeResponse(request, responseCode, getContact()));
         return response;
      }
      else
      {
         DebugLog(<< "Creating response using dialog: " << dialog);
         shared_ptr<SipMessage> response(dialog->makeResponse(request, responseCode));
         return response;
      }
   }
}
 
shared_ptr<SipMessage>
TestSipEndPoint::Dump::go(shared_ptr<SipMessage> msg)
{
   InfoLog(<<"##Dump: " << mEndPoint.getName() << " " << *msg);
   // don't send it
   static const shared_ptr<SipMessage> nul;
   return nul;
}
     
shared_ptr<SipMessage>
TestSipEndPoint::Ack::go(shared_ptr<SipMessage> response)
{
   assert(response->isResponse());
   int code = response->header(h_StatusLine).responseCode();
   shared_ptr<SipMessage> invite = mEndPoint.getSentInvite(response->header(h_CallId));
   assert (invite->header(h_RequestLine).getMethod() == INVITE);
   
   if (code == 200)
   {
      DebugLog(<< "Constructing ack against 200 using dialog.");
      DeprecatedDialog* dialog = mEndPoint.getDialog(invite->header(h_CallId));
      assert (dialog);
      DebugLog(<< *dialog);
      // !dlb! should use contact from 200?
      shared_ptr<SipMessage> ack(dialog->makeAck(*invite));
      return ack;
   }
   else
   {
      DebugLog(<<"Constructing failure ack.");
      shared_ptr<SipMessage> ack(Helper::makeFailureAck(*invite, *response));
      return ack;
   }
}

shared_ptr<SipMessage>
TestSipEndPoint::ByeTo::go(shared_ptr<SipMessage> msg, const Uri& target)
{
   DeprecatedDialog* dialog = mEndPoint.getDialog(target);
   assert(dialog);
   shared_ptr<SipMessage> bye(dialog->makeBye());
   return bye;
}

shared_ptr<SipMessage>
TestSipEndPoint::Bye::go(shared_ptr<SipMessage> msg)
{
   DeprecatedDialog* dialog = mEndPoint.getDialog(msg->header(h_CallId));
   assert(dialog);
   shared_ptr<SipMessage> bye(dialog->makeBye());
   return bye;
}

shared_ptr<SipMessage>
TestSipEndPoint::Notify200To::go(shared_ptr<SipMessage> msg, const Uri& target)
{
   DeprecatedDialog* dialog = mEndPoint.getDialog(target);
   assert(dialog);
   shared_ptr<SipMessage> notify(dialog->makeNotify());
   notify->header(h_Event).value() = "refer";
   SipFrag frag;
   frag.message().header(h_StatusLine).responseCode() = 200;
   frag.message().header(h_StatusLine).reason() = "OK";
   notify->setContents(&frag);
   return notify;
}

shared_ptr<SipMessage>
TestSipEndPoint::Notify200::go(shared_ptr<SipMessage> msg)
{
   DeprecatedDialog* dialog = mEndPoint.getDialog(msg->header(h_CallId));
   assert(dialog);
   shared_ptr<SipMessage> notify(dialog->makeNotify());
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
   DeprecatedDialog* dialog = endPoint.getDialog(mWho);
   assert(dialog);
   shared_ptr<SipMessage> refer(dialog->makeRefer(NameAddr(mTo)));
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
                                    const resip::Uri& to)
   : mEndPoint(*from),
     mTo(to)
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
   DeprecatedDialog* dialog = mEndPoint.getDialog(mTo);
   if (!dialog) 
   {
      InfoLog (<< "No matching dialog on " << mEndPoint.getName() << " for " << mTo);
      throw AssertException(resip::Data("No matching dialog"), __FILE__, __LINE__);
   }
      
   shared_ptr<SipMessage> invite(dialog->makeInvite());
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
TestSipEndPoint::reInvite(resip::Uri url) 
{
   return new ReInvite(this, url); 
}

TestSipEndPoint::InviteReferReplaces::InviteReferReplaces(TestSipEndPoint* from, 
                                                          bool replaces) 
   : mEndPoint(*from), mReplaces(replaces) 
{
}

void
TestSipEndPoint::InviteReferReplaces::operator()(shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   assert(sipEvent);
   shared_ptr<SipMessage> refer = sipEvent->getMessage();
   assert(refer->isRequest());
   assert(refer->header(h_RequestLine).getMethod() == REFER);
   
   shared_ptr<SipMessage> invite(Helper::makeInvite(refer->header(h_ReferTo),
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
     mConditioner(TestSipEndPoint::identity)                 
{
}

void 
TestSipEndPoint::MessageAction::setConditioner(MessageConditionerFn conditioner)
{
   mConditioner = ChainConditions(conditioner, mConditioner);
}

void
TestSipEndPoint::MessageAction::operator()() 
{ 
   mMsg = go(); 
   if (mMsg.get())
   {
      boost::shared_ptr<SipMessage> conditioned(mConditioner(mMsg));  
      DebugLog(<< "sending: " << conditioned->brief());
      mEndPoint.send(conditioned);
   }
}


TestSipEndPoint::Invite::Invite(TestSipEndPoint* from, 
                                const resip::Uri& to, 
                                boost::shared_ptr<resip::SdpContents> sdp)
   : MessageAction(*from, to),
     mSdp(sdp)
{}

shared_ptr<SipMessage>
TestSipEndPoint::Invite::go()
{
   shared_ptr<SipMessage> invite(Helper::makeInvite(NameAddr(mTo),
                                                    NameAddr(mEndPoint.getAddressOfRecord()),
                                                    mEndPoint.getContact()));
   if (mSdp.get() != 0)
   {
      invite->setContents(mSdp.get());
   }

   mEndPoint.storeSentInvite(invite);
   return invite;
}

resip::Data
TestSipEndPoint::Invite::toString() const
{
   return mEndPoint.getName() + ".invite()";
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const TestUser& endPoint)
{
   return new Invite(this, endPoint.getAddressOfRecord());
}


TestSipEndPoint::Invite*
TestSipEndPoint::invite(const TestUser& endPoint, const boost::shared_ptr<resip::SdpContents>& sdp)
{
   return new Invite(this, endPoint.getAddressOfRecord(), sdp);
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const TestSipEndPoint& endPoint) 
{
   return new Invite(this, endPoint.mAor); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const resip::Uri& url) 
{
   return new Invite(this, url); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const resip::Uri& url, const boost::shared_ptr<resip::SdpContents>& sdp) 
{
   return new Invite(this, url, sdp); 
}

TestSipEndPoint::Invite* 
TestSipEndPoint::invite(const resip::Data& url)
{
   return new Invite(this, resip::Uri(url)); 
}

TestSipEndPoint::RawInvite::RawInvite(TestSipEndPoint* from, 
                                      const resip::Uri& to, 
                                      const resip::Data& rawText)
   : MessageAction(*from, to),
     mRawText(rawText)
{}

shared_ptr<SipMessage>
TestSipEndPoint::RawInvite::go()
{
   auto_ptr<SipMessage> msg(Helper::makeInvite(NameAddr(mTo),
                                               NameAddr(mEndPoint.getAddressOfRecord()),
                                               mEndPoint.getContact()));
   shared_ptr<SipMessage> rawInvite(new SipRawMessage(*msg, mRawText));
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
   : MessageAction(*from, to),
     mRawText(rawText)
{}

shared_ptr<SipMessage>
TestSipEndPoint::RawSend::go()
{
   shared_ptr<SipMessage> msg(SipMessage::make(mRawText));
   msg->header(h_To) = NameAddr(mTo);
   return msg;
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


TestSipEndPoint::Subscribe::Subscribe(TestSipEndPoint* from, 
                                      const resip::Uri& to)
   : mEndPoint(*from),
     mTo(to)
{
}

resip::Data
TestSipEndPoint::Subscribe::toString() const
{
   return mEndPoint.getName() + ".subscribe()";
}

void 
TestSipEndPoint::Subscribe::operator()() 
{
   go(); 
}

void 
TestSipEndPoint::Subscribe::operator()(boost::shared_ptr<Event> event) 
{
   go(); 
}

void
TestSipEndPoint::Subscribe::go()
{
   shared_ptr<SipMessage> subscribe(Helper::makeRequest(NameAddr(mTo), 
                                                        NameAddr(mEndPoint.getAddressOfRecord()), 
                                                        mEndPoint.getContact(),
                                                        SUBSCRIBE));
   subscribe->header(h_Expires).value() = 3600;
   
   mEndPoint.storeSentSubscribe(subscribe);
   DebugLog(<< "sending SUBSCRIBE " << subscribe->brief());
   mEndPoint.send(subscribe);
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const TestSipEndPoint* endPoint) 
{
   return new Subscribe(this, endPoint->mAor); 
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const TestUser& endPoint) 
{
   return new Subscribe(this, endPoint.getAddressOfRecord()); 
}

TestSipEndPoint::Subscribe* 
TestSipEndPoint::subscribe(const resip::Uri& url) 
{
   return new Subscribe(this, url); 
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
   DataStream strm(buffer);
   strm << mEndPoint.getName() << "." << getMethodName(mType) << "(" << mTo << ")";
   strm.flush();
   return buffer;
}

shared_ptr<SipMessage>
TestSipEndPoint::Request::go()
{
   // !jf! this really should be passed in a dialog-id to identify which dialog
   // to use
   DeprecatedDialog* dialog = mEndPoint.getDialog();
   if (dialog)
   {
      shared_ptr<SipMessage> request(dialog->makeRequest(mType));
      if (mContents != 0) request->setContents(mContents.get());
      request->header(h_Expires).value() = 3600;
      return request;   
   }
   else
   {
      shared_ptr<SipMessage> request(Helper::makeRequest(NameAddr(mTo), 
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

TestSipEndPoint::Retransmit::Retransmit(TestSipEndPoint* endPoint, 
                                        boost::shared_ptr<resip::SipMessage>& msg)
   : mEndPoint(endPoint),
     mMsgToRetransmit(msg)
{
}

void
TestSipEndPoint::Retransmit::operator()(boost::shared_ptr<Event> event)
{
   assert (mMsgToRetransmit != 0);
   mEndPoint->send(mMsgToRetransmit);
}

resip::Data
TestSipEndPoint::Retransmit::toString() const
{
   return mEndPoint->getName() + ".retransmit()";
}

TestSipEndPoint::Retransmit* 
TestSipEndPoint::retransmit(boost::shared_ptr<resip::SipMessage>& msg)
{ 
   return new Retransmit(this, msg);
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
   assert(dynamic_cast<UdpTransport*>(mEndPoint->mTransport) == 0);

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
   assert(dynamic_cast<UdpTransport*>(mTransport) == 0);
   return new CloseTransport(this);
}


TestSipEndPoint::MessageExpectAction::MessageExpectAction(TestSipEndPoint& from)
   : mEndPoint(from),
     mMsg(),
     mConditioner(TestSipEndPoint::identity)                 
{
}

void
TestSipEndPoint::MessageExpectAction::operator()(boost::shared_ptr<Event> event)
{ 
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());

   if (!sipEvent)
   {
      ErrLog(<< "a TestSipEndPoint action requires a SipEvent!");
      throw AssertException("requires a SipEvent", __FILE__, __LINE__);
   }
   
   boost::shared_ptr<resip::SipMessage> msg = sipEvent->getMessage();
   
   mMsg = go(msg); 
   mConditioner(mMsg);  
   if (mMsg != 0)
   {
      DebugLog(<< "sending: " << mMsg->brief());
      mEndPoint.send(mMsg);
   }
}

void 
TestSipEndPoint::MessageExpectAction::setConditioner(MessageConditionerFn conditioner)
{
   mConditioner = ChainConditions(conditioner, mConditioner);
}

TestSipEndPoint::RawReply::RawReply(TestSipEndPoint& from,
                                    const resip::Data& rawText)
   : MessageExpectAction(from),
     mRawText(rawText)
{
}

shared_ptr<SipMessage>
TestSipEndPoint::RawReply::go(shared_ptr<SipMessage> msg)
{                                                                          
   // set via, from, to off msg?
   shared_ptr<SipMessage> mmsg(SipMessage::make(mRawText));
   return mmsg;
}                                                                       

TestSipEndPoint::Send302::Send302(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Send302::go(boost::shared_ptr<resip::SipMessage> msg)
{
   assert (msg->isRequest());
   return mEndPoint.makeResponse(*msg, 302);                        
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::send302()
{
   return new Send302(*this);
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
      return mEndPoint.makeResponse(*invite, mCode);
   }
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::respond(int code)
{
   return new Respond(*this, code);
}

TestSipEndPoint::Notify::Notify(TestSipEndPoint& endPoint,
                                const resip::Uri& presentity)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint),
     mPresentity(presentity)
{
}
   
boost::shared_ptr<resip::SipMessage>
TestSipEndPoint::Notify::go(boost::shared_ptr<resip::SipMessage> msg)
{
   WarningLog(<< "Notify: " << *msg);
   Pidf* pidf = new Pidf;
   boost::shared_ptr<resip::Contents> body(pidf);   

   if (msg->header(h_Event).value() == resip::Symbols::Presence)
   {
      pidf->setSimpleStatus(false, "offline", Data::from(mPresentity));
      pidf->getTuples().front().id = mPresentity.getAor();
      pidf->getTuples().front().attributes["displayName"] = "displayName";
   }
   else if (msg->header(h_Event).value() == "callme")
   {}
   else
   {
      assert(false);
   }
   pidf->setEntity(mPresentity);

   DeprecatedDialog depd(NameAddr(mEndPoint.getAddressOfRecord()));
   delete depd.makeResponse(*msg, 202);
   
   shared_ptr<SipMessage> notify(depd.makeNotify());
   notify->header(h_SubscriptionState).value() = "active";
   if (msg->exists(h_Event))
   {
      notify->header(h_Event) = msg->header(h_Event);
   }
   // hack to statelessly set the CSequence
   notify->header(h_CSeq).sequence() = msg->header(h_CSeq).sequence();
   notify->setContents(pidf);
   
   return notify;
}

TestSipEndPoint::Notify* 
TestSipEndPoint::notify(const resip::Uri& presentity)
{
   return new Notify(*this, presentity);
}

TestSipEndPoint::Answer::Answer(TestSipEndPoint & endPoint)
   : MessageExpectAction(endPoint),
     mEndPoint(endPoint)
{
}

boost::shared_ptr<resip::SipMessage>                                
TestSipEndPoint::Answer::go(boost::shared_ptr<resip::SipMessage> msg)                                
{                                                                           
   boost::shared_ptr<resip::SipMessage> invite;                         
   invite = mEndPoint.getReceivedInvite(msg->header(resip::h_CallId));  
   boost::shared_ptr<resip::SipMessage> response = mEndPoint.makeResponse(*invite, 200);
   const resip::SdpContents* sdp = dynamic_cast<const resip::SdpContents*>(invite->getContents());
   response->setContents(sdp);
   return response;
}                                                                           

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::answer()
{
   return new Answer(*this);
}


TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ring()
{
   return new Ring(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ring183()
{
   return new Ring183(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ok()
{
   return new Ok(*this);
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
TestSipEndPoint::send202()
{
   return new Send202(*this);
}

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
TestSipEndPoint::dump()
{
   return new Dump(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestSipEndPoint::ack()
{
   return new Ack(*this);
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

TestSipEndPoint::From::From(const TestSipEndPoint& testEndPoint)
   : mEndPoint(&testEndPoint),
     mProxy(0),
     mContact()
{
   assert(mEndPoint);
}

TestSipEndPoint::From::From(TestProxy& testProxy)
   : mEndPoint(0),
     mProxy(&testProxy),
     mContact()
{
   assert(mProxy);
}

TestSipEndPoint::From::From(const resip::Uri& contact)
   : mEndPoint(0),
     mProxy(0),
     mContact(contact)
{
   assert(!mContact.host().empty());
}

// check if the message is within a known transaction
// check via
// check max-forwards
// check record-route
// check route
// this requires the endpoint to store all sent and received requests, not just
// INVITE and SUBSCRIBE?
bool 
TestSipEndPoint::From::isMatch(shared_ptr<SipMessage>& message) const
{
   //DebugLog(<< "TestSipEndPoint::From::isMatch");
   // check that the from matches the stored agent
   if (mEndPoint || !mContact.host().empty())
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
   assert(false);
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

   assert(false);
}

TestSipEndPoint::Contact::Contact(const TestSipEndPoint& testEndPoint)
   : mEndPoint(&testEndPoint),
     mProxy(0),
     mContact()
{
   assert(mEndPoint);
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
TestSipEndPoint::Contact::isMatch(shared_ptr<SipMessage>& message) const
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
   assert (testEndPoint);
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

// JF


shared_ptr<SipMessage>
TestSipEndPoint::Cancel::go(shared_ptr<SipMessage> msg)
{
   DeprecatedDialog* dialog = mEndPoint.getDialog(msg->header(h_CallId));
   assert(dialog);
   shared_ptr<SipMessage> invite = mEndPoint.getSentInvite(msg->header(h_CallId));
   assert(invite != 0);
   shared_ptr<SipMessage> cancel(dialog->makeCancel(*invite));
   return cancel;
}

void 
TestSipEndPoint::NoAction::operator()(shared_ptr<Event> event)
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
   assert(sipEvent);
   assert(mEndPoint);

   DebugLog(<< "TestSipEndPoint::ToMatch(" << Data::from(mTo) << " ? " << Data::from(sipEvent->getMessage()->header(h_To)) << ")");
   return (sipEvent->getMessage()->exists(h_To) &
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
TestSipEndPoint::SipExpect::onEvent(TestEndPoint& endPoint, shared_ptr<Event> event)
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

ostream& 
TestSipEndPoint::SipExpect::output(ostream& s) const
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

   return s;
}

bool
TestSipEndPoint::SipExpect::isMatch(shared_ptr<Event> event) const
{
   DebugLog (<< "matching: " << *event);
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   if (!sipEvent) return false;
   
   shared_ptr<SipMessage> msg = sipEvent->getMessage();

   TestSipEndPoint* endPoint = dynamic_cast<TestSipEndPoint*>(sipEvent->getEndPoint());
   assert(endPoint);

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
TestSipEndPoint::SipExpect::explainMismatch(shared_ptr<Event> event) const
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   assert(sipEvent);
   shared_ptr<SipMessage> msg = sipEvent->getMessage();

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
   assert(mTransport);
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
            shared_ptr<SipMessage> sipMsg(sip);
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
         if (getSequenceSet())
         {
            Data msg;
            {
               DataStream str(msg);
               str << e;
            }
            getSequenceSet()->globalFailure(msg);
         }
      }
   }
   catch (resip::BaseException& e)
   {
      DebugLog (<< "Uncaught VOCAL exception: " << e << "...ignoring");
      if (getSequenceSet())
      {
         Data msg;
         {
            DataStream str(msg);
            str << e;
         }
         getSequenceSet()->globalFailure(msg);
      }
   }
   catch (std::exception& e)
   {
      DebugLog (<< "Uncaught std::exception exception: " << e.what() << "...ignoring");
      if (getSequenceSet())
      {
         getSequenceSet()->globalFailure(e.what());
      }

   }
   catch (...)
   {
      DebugLog (<< "Uncaught unknown exception ");
      if (getSequenceSet())
      {
         getSequenceSet()->globalFailure("Uncaught unknown exception ");
      }
   }
}

void 
TestSipEndPoint::handleEvent(boost::shared_ptr<Event> event)
{
   shared_ptr<SipEvent> sipEvent = shared_dynamic_cast<SipEvent>(event);
   assert(sipEvent);
   shared_ptr<SipMessage> msg = sipEvent->getMessage();
   
   DebugLog(<< getContact() << " is handling: " << *msg);
   if (msg->isResponse())
   {      
      if (msg->header(h_CSeq).method() == INVITE && 
          msg->header(h_StatusLine).responseCode() > 100 &&
          msg->header(h_StatusLine).responseCode() <= 200)
      {
         shared_ptr<SipMessage> invite = getSentInvite(msg->header(h_CallId));
         //DebugLog (<< "invite map = " << mInvitesSent);
         
         assert(invite != 0);
         
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId));
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
         assert(found);
         
         shared_ptr<SipMessage> subscribe = getSentSubscribe(msg);
         assert(subscribe != 0);
         
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId));
         if (dialog != 0)
         {
            CerrLog(<< "refreshing with SUBSCRIBE/2xx");
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
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId));
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
         DeprecatedDialog* dialog = getDialog(msg->header(h_CallId));
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
   if (getSequenceSet())
   {
      getSequenceSet()->enqueue(event);
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
   assert (testEndPoint);
   return new TestSipEndPoint::From(*testEndPoint);
}

TestSipEndPoint::From*
from(TestProxy* testProxy)
{
   return new TestSipEndPoint::From(*testProxy);
}

TestSipEndPoint::MessageAction*
condition(TestSipEndPoint::MessageConditionerFn fn, TestSipEndPoint::MessageAction* action)
{
   action->setConditioner(fn);
   return action;
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
save(boost::shared_ptr<resip::SipMessage>& msgPtr,
     TestSipEndPoint::MessageExpectAction* action)
{
   TestSipEndPoint::SaveMessage saver(msgPtr);
   return condition(saver, action);
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


#ifdef RTP_ON
void
TestSipEndPoint::CreateRtpSession::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   assert(sipEvent);
   shared_ptr<SipMessage> msg = sipEvent->getMessage();

   SdpContents* remoteSdp = dynamic_cast<SdpContents*>(msg->getContents());
   assert(remoteSdp != 0);

   assert(!_localSdp->session().getMedia().empty());
   assert(!remoteSdp->session().getMedia().empty());
   
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
