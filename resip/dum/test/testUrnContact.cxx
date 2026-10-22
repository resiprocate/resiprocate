// Regression test for the behavior documented in Dialog.cxx and
// BaseCreator.cxx: when a request's To/Request-URI is a urn: (e.g. an
// RFC 5031 emergency-services URN), its user part is NOT copied into the
// local Contact (urn:'s NID:NSS has nothing to do with a sip: user's
// ABNF). This sends a real INVITE from a UAC to a urn: target and checks
// that the UAS's resulting Contact (as seen by the UAC in the 180
// response) ends up with a sip: URI and no user part.

#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

class TestInviteSessionHandler : public InviteSessionHandler
{
   public:
      Data name;
      bool sawProvisional;
      NameAddr contactSeen;

      TestInviteSessionHandler(const Data& n) : name(n), sawProvisional(false)
      {
      }

      virtual ~TestInviteSessionHandler()
      {
      }

      virtual void onNewSession(ClientInviteSessionHandle, InviteSession::OfferAnswerType, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onNewSession - " << msg.brief() << endl;
      }

      // UAS side: a new INVITE arrived (Request-URI/To == urn:service:sos).
      // Immediately send a provisional response so the UAC can observe the
      // Contact this Dialog builds for itself.
      virtual void onNewSession(ServerInviteSessionHandle sis, InviteSession::OfferAnswerType, const SipMessage& msg)
      {
         cout << name << ": ServerInviteSession-onNewSession - " << msg.brief() << endl;
         sis->provisional(180);
      }

      virtual void onFailure(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onFailure - " << msg.brief() << endl;
      }

      // UAC side: capture the Contact from the 180 -- this is the UAS's
      // mLocalContact, built while processing our urn: INVITE.
      virtual void onProvisional(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onProvisional - " << msg.brief() << endl;
         if (msg.exists(h_Contacts) && msg.header(h_Contacts).size() == 1)
         {
            sawProvisional = true;
            contactSeen = msg.header(h_Contacts).front();
         }
      }

      virtual void onConnected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onConnected - " << msg.brief() << endl;
      }

      virtual void onStaleCallTimeout(ClientInviteSessionHandle)
      {
         cout << name << ": ClientInviteSession-onStaleCallTimeout" << endl;
      }

      virtual void onConnected(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onConnected - " << msg.brief() << endl;
      }

      virtual void onRedirected(ClientInviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": ClientInviteSession-onRedirected - " << msg.brief() << endl;
      }

      virtual void onTerminated(InviteSessionHandle, InviteSessionHandler::TerminatedReason, const SipMessage* msg)
      {
         cout << name << ": InviteSession-onTerminated" << endl;
      }

      virtual void onAnswer(InviteSessionHandle, const SipMessage&, const SdpContents&)
      {
         cout << name << ": InviteSession-onAnswer(SDP)" << endl;
      }

      virtual void onOffer(InviteSessionHandle is, const SipMessage&, const SdpContents&)
      {
         cout << name << ": InviteSession-onOffer(SDP)" << endl;
      }

      virtual void onEarlyMedia(ClientInviteSessionHandle, const SipMessage&, const SdpContents&)
      {
         cout << name << ": InviteSession-onEarlyMedia(SDP)" << endl;
      }

      virtual void onOfferRequired(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onOfferRequired - " << msg.brief() << endl;
      }

      virtual void onOfferRejected(InviteSessionHandle, const SipMessage*)
      {
         cout << name << ": InviteSession-onOfferRejected" << endl;
      }

      virtual void onRefer(InviteSessionHandle, ServerSubscriptionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onRefer - " << msg.brief() << endl;
      }

      virtual void onReferAccepted(InviteSessionHandle, ClientSubscriptionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onReferAccepted - " << msg.brief() << endl;
      }

      virtual void onReferRejected(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onReferRejected - " << msg.brief() << endl;
      }

      virtual void onReferNoSub(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onReferNoSub - " << msg.brief() << endl;
      }

      virtual void onInfo(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onInfo - " << msg.brief() << endl;
      }

      virtual void onInfoSuccess(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onInfoSuccess - " << msg.brief() << endl;
      }

      virtual void onInfoFailure(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onInfoFailure - " << msg.brief() << endl;
      }

      virtual void onMessage(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onMessage - " << msg.brief() << endl;
      }

      virtual void onMessageSuccess(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onMessageSuccess - " << msg.brief() << endl;
      }

      virtual void onMessageFailure(InviteSessionHandle, const SipMessage& msg)
      {
         cout << name << ": InviteSession-onMessageFailure - " << msg.brief() << endl;
      }

      virtual void onForkDestroyed(ClientInviteSessionHandle)
      {
         cout << name << ": ClientInviteSession-onForkDestroyed" << endl;
      }
};

int
main(int argc, char* argv[])
{
   Log::initialize(Log::Cout, resip::Log::Info, argv[0]);

   SdpContents::Session::Medium medium("audio", 8000, 1, "RTP/AVP");
   Data sdpTxt(
      "v=0\r\n"
      "o=1900 369696545 369696545 IN IP4 127.0.0.1\r\n"
      "s=-\r\n"
      "c=IN IP4 127.0.0.1\r\n"
      "t=0 0\r\n"
      "m=audio 8000 RTP/AVP 0\r\n"
      "a=rtpmap:0 PCMU/8000\r\n");
   HeaderFieldValue hfv(sdpTxt.data(), sdpTxt.size());
   Mime sdpType("application", "sdp");
   SdpContents offer(hfv, sdpType);

   // Set up UAC.
   SipStack stackUac;
   DialogUsageManager dumUac(stackUac);
   stackUac.addTransport(UDP, 27298);

   auto uacMasterProfile = std::make_shared<MasterProfile>();
   dumUac.setMasterProfile(uacMasterProfile);
   dumUac.getMasterProfile()->setDefaultFrom(NameAddr("sip:uac@127.0.0.1:27298"));
   // A urn: Request-URI has no host to resolve; a real UAC relies on an
   // outbound/emergency proxy to route it (e.g. via LoST). Point directly
   // at the UAS to exercise that without needing a real proxy.
   dumUac.getMasterProfile()->setOutboundProxy(Uri("sip:127.0.0.1:27299"));

   TestInviteSessionHandler uac("UAC");
   dumUac.setInviteSessionHandler(&uac);

   // Set up UAS. urn: support is opt-in per application (see the comment
   // on MasterProfile::addSupportedScheme()) -- this is exactly the call
   // an emergency-services UAS is expected to make.
   SipStack stackUas;
   DialogUsageManager dumUas(stackUas);
   stackUas.addTransport(UDP, 27299);

   auto uasMasterProfile = std::make_shared<MasterProfile>();
   uasMasterProfile->addSupportedScheme(Symbols::Urn);
   dumUas.setMasterProfile(uasMasterProfile);
   dumUas.getMasterProfile()->setDefaultFrom(NameAddr("sip:uas@127.0.0.1:27299"));

   TestInviteSessionHandler uas("UAS");
   dumUas.setInviteSessionHandler(&uas);

   // UAC sends an INVITE targeting a urn: (RFC 5031 emergency call). Per
   // BaseCreator::makeInitialRequest(), this becomes both the Request-URI
   // and the To header.
   NameAddr target("urn:service:sos");
   dumUac.send(dumUac.makeInviteSession(target, &offer));

   for (int i = 0; i < 20 && !uac.sawProvisional; i++)
   {
      stackUac.process(50);
      while (dumUac.process());
      stackUas.process(50);
      while (dumUas.process());
   }

   assert(uac.sawProvisional);
   assert(isEqualNoCase(uac.contactSeen.uri().scheme(), Symbols::Sip));
   assert(uac.contactSeen.uri().user().empty());

   cout << "!!!!!!!!!!!!!!!!!! Successful !!!!!!!!!! " << endl;
}
