#include <iostream>
#include <memory>

#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/PlainContents.hxx"
#include "resiprocate/UnknownHeaderType.hxx"
#include "resiprocate/UnknownParameterType.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
#define CRLF "\r\n"

int
main(int argc, char** argv)
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

   {
     char * txt =(              
        "SIP/2.0 489 Bad Event" CRLF
       "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1-c87542-" CRLF
       "CSeq: 1 SUBSCRIBE" CRLF
       "Allow-Events: " CRLF
       "Call-ID:  f354ce714fb8a95c" CRLF
       "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
       "To:  <sip:RjS@127.0.0.1:5060>" CRLF
       CRLF
       );
     auto_ptr<SipMessage> response(TestSupport::makeMessage(txt,true));
     assert(response->exists(h_AllowEvents));
     assert(response->header(h_AllowEvents).size() == 1);
     assert(response->header(h_AllowEvents).front().value().empty());
     
     char * txt2 =(              
       "SIP/2.0 489 Bad Event" CRLF
       "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1-c87542-" CRLF
       "CSeq: 1 SUBSCRIBE" CRLF
       "Call-ID:  f354ce714fb8a95c" CRLF
       "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
       "To:  <sip:RjS@127.0.0.1:5060>" CRLF
       "Allow-Events: " CRLF
       CRLF
       );
     SipMessage * r2 = TestSupport::makeMessage(txt2,true);
     assert(r2->exists(h_AllowEvents) );
     assert(r2->header(h_AllowEvents).size() == 1);
     assert(r2->header(h_AllowEvents).front().value().empty());


     char * txt3 =(              
       "SIP/2.0 489 Bad Event" CRLF
       "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1-c87542-" CRLF
       "CSeq: 1 SUBSCRIBE" CRLF
       "Call-ID:  f354ce714fb8a95c" CRLF
       "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
       "To:  <sip:RjS@127.0.0.1:5060>" CRLF
       "Allow-Events: foo" CRLF
       "Allow-Events: bar" CRLF
       "Allow-Events: " CRLF
       CRLF
       );
     SipMessage * r3 = TestSupport::makeMessage(txt3,true);
     assert(r3->exists(h_AllowEvents) );
     assert(r3->header(h_AllowEvents).size() == 3);
     assert(r3->header(h_AllowEvents).front().value() == "foo");
   }
   {
      // Test just in time parsing with comparison: NameAddr;
      char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-ec1e.0-1-c87542-;stid=489573115\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Accept: foo/bar\r\n"
                   "Content-Type: bar/foo\r\n"
                   "Content-Encoding: foo\r\n"
                   "Content-Disposition: bar\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->header(h_ContentDisposition) < message->header(h_ContentEncoding));
      assert(message->header(h_ContentType) < message->header(h_Accepts).front());
      assert(message->header(h_To) < message->header(h_From));
   }
   
   {
      // Proxy-Authorization does not allow comma joined headers
      char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-ec1e.0-1-c87542-;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1-c87542-;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", reponse=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\", Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", reponse=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      cerr << *message << endl;

      try
      {
         int s = message->header(h_ProxyAuthorizations).size();
         cerr << "!! " << s << endl;
         cerr << "!! " << message->header(h_ProxyAuthorizations).front().exists(p_realm) << endl;
         cerr << *message << endl;
         assert(false);
      }
      catch (ParseBuffer::Exception& e)
      {
         assert(e.getMessage() == "Proxy-Authorization");
      }
   }

   {
      char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-ec1e.0-1-c87542-;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1-c87542-;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", reponse=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Proxy-Authorization: Digest username=\"Alice\", realm = \"atlanta.com\", nonce=\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\", reponse=\"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\"\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      cerr << *message << endl;

      assert(message->header(h_ProxyAuthorizations).size() == 2);
   }

   {
      char* txt = ("INVITE sip:ext101@192.168.2.220:5064;transport=UDP SIP/2.0\r\n"
                   "To: <sip:ext101@whistler.gloo.net:5061>\r\n"
                   "From: <sip:ext103@whistler.gloo.net:5061>;tag=a731\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5061;branch=z9hG4bK-c87542-ec1e.0-1-c87542-;stid=489573115\r\n"
                   "Via: SIP/2.0/UDP whistler.gloo.net:5068;branch=z9hG4bK-c87542-489573115-1-c87542-;received=192.168.2.220\r\n"
                   "Call-ID: 643f2f06\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Record-Route: <sip:proxy@whistler.gloo.net:5061;lr>\r\n"
                   "Contact: <sip:ext103@192.168.2.220:5068;transport=UDP>\r\n"
                   "Max-Forwards: 69\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      cerr << *message << endl;

      assert(message->header(h_Vias).front().param(UnknownParameterType("stid")) == "489573115");
      message->header(h_Vias).front().param(p_received) = "received";
      assert(message->header(h_Vias).front().param(UnknownParameterType("stid")) == "489573115");
   }

   {
      char* txt = ("SIP/2.0 200 OK""\r\n"
                   "From: 1245<sip:4000@193.12.63.124:5060>;tag=7c3f0cc1-13c4-3e5a380c-1ac5646-257e""\r\n"
                   "To: prolab<sip:5000@host2.sipdragon.sipit.net>;tag=7c3f0cc1-13c5-3e5a380d-1ac5827-618f""\r\n"
                   "Call-ID: 9e9017c-7c3f0cc1-13c4-3e5a380c-1ac5646-3700@193.12.63.124""\r\n"
                   "CSeq: 1 INVITE""\r\n"
                   "Via: SIP/2.0/UDP host2.sipdragon.sipit.net;received=193.12.62.209;branch=z9hG4bK-c87542--3e5a380c-1ac5646-adf-c87542-1-1""\r\n"
                   "Via: SIP/2.0/UDP 193.12.63.124:5060;received=193.12.63.124;branch=z9hG4bK-3e5a380c-1ac5646-adf""\r\n"
                   "Contact: <sip:5000@193.12.63.124:5061>""\r\n"
                   "Record-Route: <sip:proxy@host2.sipdragon.sipit.net:5060;lr>""\r\n"
                   "Content-Length:0\r\n\r\n");
      
  
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      cerr << *message << endl;
      message->header(h_Vias).front();
      message->header(h_From);
      message->header(h_To);
      cerr << message->header(h_Vias).front().param(p_branch).getTransactionId() << endl;
      cerr << message->header(h_Vias).back().param(p_branch).getTransactionId() << endl;
   }

   {
      char *txt = ("SIP/2.0 200 OK\r\n"
                   "From: 1245<sip:4000@193.12.63.124:5060>;tag=7c3f0cc1-13c4-3e5a380c-1ac5646-257e\r\n"
                   "To: prolab<sip:5000@host2.sipdragon.sipit.net>;tag=7c3f0cc1-13c5-3e5a380d-1ac5827-618f\r\n"
                   "Call-ID: 9e9017c-7c3f0cc1-13c4-3e5a380c-1ac5646-3700@193.12.63.124\r\n"
                   "CSeq: 1 INVITE\r\n"
                   "Via: SIP/2.0/UDP host2.sipdragon.sipit.net;received=193.12.62.209;branch=z9hG4bK-c87542--3e5a380c-1ac5646-adf-c87542-1-1\r\n"
                   "Via: SIP/2.0/UDP 193.12.63.124:5060;received=193.12.63.124;branch=z9hG4bK-3e5a380c-1ac5646-adf\r\n"
                   "Contact: <sip:5000@193.12.63.124:5061>\r\n"
                   "Record-Route: <sip:proxy@host2.sipdragon.sipit.net:5060;lr>\r\n"
                   "Content-Length:0\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      assert(message.get());
      for (Vias::iterator i = message->header(h_Vias).begin();
           i != message->header(h_Vias).end(); i++)
      {
         i->param(p_branch).encode(cerr);
         cerr << endl;
      }
   }

   {
      char* txt = ("SIP/2.0 200 OK""\r\n"
                   "From: 1245<sip:4000@193.12.63.124:5060>;tag=7c3f0cc1-13c4-3e5a380c-1ac5646-257e""\r\n"
                   "To: prolab<sip:5000@host2.sipdragon.sipit.net>;tag=7c3f0cc1-13c5-3e5a380d-1ac5827-618f""\r\n"
                   "Call-ID: 9e9017c-7c3f0cc1-13c4-3e5a380c-1ac5646-3700@193.12.63.124""\r\n"
                   "CSeq: 1 INVITE""\r\n"
                   "Via: SIP/2.0/UDP host2.sipdragon.sipit.net;received=193.12.62.209;branch=z9hG4bK-c87542--3e5a380c-1ac5646-adf-c87542-1-1""\r\n"
                   "Via: SIP/2.0/UDP 193.12.63.124:5060;received=193.12.63.124;branch=z9hG4bK-3e5a380c-1ac5646-adf""\r\n"
                   "Contact: <sip:5000@193.12.63.124:5061>""\r\n"
                   "Record-Route: <sip:proxy@host2.sipdragon.sipit.net:5060;lr>""\r\n"
                   "Content-Length:0\r\n\r\n");
      
   
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      cerr << *message << endl;
      message->header(h_Vias).front();
      message->header(h_From);
      message->header(h_To);
      cerr << message->header(h_Vias).front() << endl;
      cerr << message->header(h_Vias).back() << endl;
      cerr << message->header(h_Vias).front().param(p_branch).getTransactionId() << endl;
      cerr << message->header(h_Vias).back().param(p_branch).getTransactionId() << endl;
   }

   {
      char *txt = ("To: <sip:106@kelowna.gloo.net>"
                   "From: <sip:106@kelowna.gloo.net>;tag=18c7b33a-430c-429c-9f46-e5b509264519\r\n"
                   "Via: SIP/2.0/UDP 192.168.2.15:10276;received=192.168.2.15\r\n"
                   "Call-ID: cb15283c-6efb-452e-aef2-5e44e02e2440@192.168.2.15\r\n"
                   "CSeq: 2 REGISTER\r\n"
                   "Contact: <sip:192.168.2.15:10276>;methods=\"INVITE, MESSAGE, INFO, SUBSCRIBE, OPTIONS, BYE, CANCEL, NOTIFY, ACK\"\r\n"
                   "Expires: 0\r\n"
                   "User-Agent: Windows RTC/1.0\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      cerr << message->header(h_Contacts).front().param(UnknownParameterType("methods")) << endl;
      cerr << *message << endl;
   }

   {
      Data txt1("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");
      
      Data txt2("INVITE sip:joe@biloxi.com SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt3("INVITE sip:bob@biloxi.com;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt4("INVITE sip:bob@biloxi.com;user=phone;lr SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt5("INVITE sip:bob@biloxi.com;user=phone;lr;maddr=192.168.1.1 SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt6("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt7("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;user=phone;jason=foo SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt8("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt9("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com; branch=foobar\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314159 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt10("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com; branch=foobar\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314158 INVITE\r\n"
                "Max-Forwards: 70\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc33.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");

      Data txt11("INVITE sip:bob@biloxi.com;maddr=192.168.1.1;lr;jason=foo;user=phone SIP/2.0\r\n"
                "Via: SIP/2.0/UDP pc33.atlanta.com; branch=foobar\r\n"
                "To: Bob <sip:bob@biloxi.com>\r\n"
                "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                "Call-ID: a84b4c76e66710\r\n"
                "CSeq: 314158 INVITE\r\n"
                "Max-Forwards: 73\r\n"
                "Foobie-Blech: it is not a glass paperweight\r\n"
                "Contact: <sip:alice@pc.atlanta.com>\r\n"
                "Content-Length: 0\r\n"
                "\r\n");
      
      
      auto_ptr<SipMessage> msg1(TestSupport::makeMessage(txt1));
      auto_ptr<SipMessage> msg2(TestSupport::makeMessage(txt2));
      auto_ptr<SipMessage> msg3(TestSupport::makeMessage(txt3));
      auto_ptr<SipMessage> msg4(TestSupport::makeMessage(txt4));
      auto_ptr<SipMessage> msg5(TestSupport::makeMessage(txt5));
      auto_ptr<SipMessage> msg6(TestSupport::makeMessage(txt6));
      auto_ptr<SipMessage> msg7(TestSupport::makeMessage(txt7));
      auto_ptr<SipMessage> msg8(TestSupport::makeMessage(txt8));
      auto_ptr<SipMessage> msg9(TestSupport::makeMessage(txt9));
      auto_ptr<SipMessage> msg10(TestSupport::makeMessage(txt10));
      auto_ptr<SipMessage> msg11(TestSupport::makeMessage(txt11));

      assert(msg1->getTransactionId() == msg1->getTransactionId());
      assert(msg2->getTransactionId() != msg3->getTransactionId());
      assert(msg3->getTransactionId() == msg4->getTransactionId());
      assert(msg4->getTransactionId() != msg5->getTransactionId());
      assert(msg4->getTransactionId() != msg6->getTransactionId());
      assert(msg5->getTransactionId() == msg6->getTransactionId());
      assert(msg7->getTransactionId() == msg8->getTransactionId());
      assert(msg6->getTransactionId() != msg8->getTransactionId());
      assert(msg8->getTransactionId() != msg9->getTransactionId());
      assert(msg9->getTransactionId() != msg10->getTransactionId());
      assert(msg10->getTransactionId() == msg11->getTransactionId());
   }
   
   {
      SipMessage inv;

      inv.header(h_Vias);
      inv.header(h_To);
      
      inv.header(h_RequestLine) = RequestLine(INVITE);
      inv.header(h_RequestLine).uri() = Uri("sip:bob@biloxi.com");
      inv.header(h_To) = NameAddr("sip:bob@biloxi.com");
      inv.header(h_From) = NameAddr("Alice <sip:alice@atlanta.com>;tag=1928301774");
      inv.header(h_CallId).value() = "314159";
      inv.header(h_CSeq).sequence() = 14;
      inv.header(h_CSeq).method() = INVITE;

      PlainContents pc("here is some plain ol' contents");
      inv.setContents(&pc);

      cerr << inv.header(h_ContentType).type() << endl;
      assert(inv.header(h_ContentType).type() == "text");
      assert(inv.header(h_ContentType).subType() == "plain");

      assert(!inv.exists(h_ContentLength));

      assert(inv.getContents());
      assert(dynamic_cast<PlainContents*>(inv.getContents()));
      assert(dynamic_cast<PlainContents*>(inv.getContents())->text() == "here is some plain ol' contents");

      Data d;
      {
         DataStream s(d);
         inv.encode(s);
      }
      
      cerr << "!! " << d;
      assert(d == ("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
                   "To: <sip:bob@biloxi.com>\r\n"
                   "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                   "Call-ID: 314159\r\n"
                   "CSeq: 14 INVITE\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: 31\r\n"
                   "\r\n"
                   "here is some plain ol' contents"));
   }
   
   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
      
      assert(msg->exists(h_ContentType));
      Contents* body = msg->getContents();

      assert(body != 0);
      SdpContents* sdp = dynamic_cast<SdpContents*>(body);
      assert(sdp != 0);

      assert(sdp->session().version() == 0);
      assert(sdp->session().origin().user() == "alice");
      assert(!sdp->session().media().empty());
      assert(sdp->session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");

      msg->encode(cerr);
   }

   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Foobie-Blech: it is not a glass paperweight\r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      assert(!msg->header(UnknownHeaderType("Foobie-Blech")).empty());
      assert(msg->header(UnknownHeaderType("Foobie-Blech")).front().value() == "it is not a glass paperweight");
      
      Contents* body = msg->getContents();

      assert(body != 0);
      SdpContents* sdp = dynamic_cast<SdpContents*>(body);
      assert(sdp != 0);

      assert(sdp->session().version() == 0);
      assert(sdp->session().origin().user() == "alice");
      assert(!sdp->session().media().empty());
      assert(sdp->session().media().front().getValues("rtpmap").front() == "0 PCMU/8000");

      msg->encode(cerr);
   }

   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Foobie-Blech: \r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      msg->header(UnknownHeaderType("Foobie-Blech")).empty();
      //assert(!msg->header(UnknownHeaderType("Foobie-Blech")).empty());
      //assert(msg->header(UnknownHeaderType("Foobie-Blech")).front().value() == "");
      msg->encode(cerr);
   }

   {
      char* b = "shared buffer";
      HeaderFieldValue h1(b, strlen(b));
      HeaderFieldValue h2(h1);
   }

   {
      char *txt = 
         ("SIP/2.0 200\r\n"
          "To: <sip:ext102@squamish.gloo.net:5060>;tag=8be36d98\r\n"
          "From: <sip:ext102@squamish.gloo.net:5060>;tag=38810b6d\r\n"
          "Call-ID: a6aea86d75a6bb45\r\n"
          "CSeq: 2 REGISTER\r\n"
          "Contact: <sip:ext102@whistler.gloo.net:6064>;expires=63\r\n"
          "Via: SIP/2.0/UDP whistler.gloo.net:6064;rport=6064;received=192.168.2.220;branch=z9hG4bK-kcD23-4-1\r\n"
          "Content-Length: 0\r\n"
          "\r\n");
      
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt));
      cerr << msg->header(h_Contacts).front().param(p_expires) << endl;
      assert(msg->header(h_Contacts).front().param(p_expires) == 63);
   }

   {
      cerr << "test backward compatible expires parameter" << endl;
      char *txt1 = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                    "Max-Forwards: 70\r\n"
                    "To: Bob <sip:bob@biloxi.com>\r\n"
                    "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                    "Call-ID: 843817637684230@998sdasdh09\r\n"
                    "CSeq: 1826 REGISTER\r\n"
                    "Contact: <sip:bob@192.0.2.4>;expires=\"Sat, 01 Dec 2040 16:00:00 GMT\";foo=bar\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));
      cerr << message1->header(h_Contacts).front().param(p_expires) << endl;
      assert(message1->header(h_Contacts).front().param(p_expires) == 3600);
      assert(message1->header(h_Contacts).front().param(UnknownParameterType("foo")) == "bar");
   }

   {
      cerr << "test header copying between unparsed messages" << endl;
      char *txt1 = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                    "Max-Forwards: 70\r\n"
                    "To: Bob <sip:bob@biloxi.com>\r\n"
                    "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                    "Call-ID: 843817637684230@998sdasdh09\r\n"
                    "CSeq: 1826 REGISTER\r\n"
                    "Contact: <sip:bob@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 7200\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));
      auto_ptr<SipMessage> r(Helper::makeResponse(*message1, 100));
      r->encode(cerr);

      char *txt2 = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfirst\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=ssecond\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sthird\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfourth\r\n"
                    "Max-Forwards: 7\r\n"
                    "To: Speedy <sip:speedy@biloxi.com>\r\n"
                    "From: Speedy <sip:speedy@biloxi.com>;tag=88888\r\n"
                    "Call-ID: 88888@8888\r\n"
                    "CSeq: 6281 REGISTER\r\n"
                    "Contact: <sip:speedy@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 2700\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message2(TestSupport::makeMessage(txt2));

      // copy over everything
      message1->header(h_RequestLine) = message2->header(h_RequestLine);
      message1->header(h_Vias) = message2->header(h_Vias);
      message1->header(h_MaxForwards) = message2->header(h_MaxForwards);
      message1->header(h_To) = message2->header(h_To);
      message1->header(h_From) = message2->header(h_From);
      message1->header(h_CallId) = message2->header(h_CallId);
      message1->header(h_CSeq) = message2->header(h_CSeq);
      message1->header(h_Contacts) = message2->header(h_Contacts);
      message1->header(h_Expires) = message2->header(h_Expires);
      message1->header(h_ContentLength) = message2->header(h_ContentLength);
      
      assert(message1->header(h_To).uri().user() == "speedy");
      assert(message1->header(h_From).uri().user() == "speedy");
      assert(message1->header(h_MaxForwards).value() == 7);
      assert(message1->header(h_Contacts).empty() == false);
      assert(message1->header(h_CallId).value() == "88888@8888");
      assert(message1->header(h_CSeq).sequence() == 6281);
      assert(message1->header(h_CSeq).method() == REGISTER);
      assert(message1->header(h_Vias).empty() == false);
      assert(message1->header(h_Vias).size() == 4);
      assert(message1->header(h_Expires).value() == 2700);
      assert(message1->header(h_ContentLength).value() == 0);
      cerr << "Port: " << message1->header(h_RequestLine).uri().port() << endl;
      cerr << "AOR: " << message1->header(h_RequestLine).uri().getAor() << endl;
      assert(message1->header(h_RequestLine).uri().getAor() == "registrar.ixolib.com");
   }

   {
      cerr << "test header copying between parsed messages" << endl;
      cerr << " should NOT COPY any HeaderFieldValues" << endl;
      char *txt1 = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                    "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                    "Max-Forwards: 70\r\n"
                    "To: Bob <sip:bob@biloxi.com>\r\n"
                    "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                    "Call-ID: 843817637684230@998sdasdh09\r\n"
                    "CSeq: 1826 REGISTER\r\n"
                    "Contact: <sip:bob@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 7200\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message1(TestSupport::makeMessage(txt1));

      // parse it
      message1->header(h_RequestLine).getMethod();
      for (NameAddrs::iterator i = message1->header(h_Contacts).begin();
           i != message1->header(h_Contacts).end(); i++)
      {
         i->uri();
      }

      for (Vias::iterator i = message1->header(h_Vias).begin();
           i != message1->header(h_Vias).end(); i++)
      {
         i->sentPort();
      }

      message1->header(h_To).uri().user();
      message1->header(h_From).uri().user();
      message1->header(h_MaxForwards).value();
      message1->header(h_Contacts).empty();
      message1->header(h_CallId).value();
      message1->header(h_CSeq).sequence();
      message1->header(h_CSeq).method();
      message1->header(h_Vias).empty();
      message1->header(h_Vias).size();
      message1->header(h_Expires).value();
      message1->header(h_ContentLength).value();

      char *txt2 = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=sfirst\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=ssecond\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=sthird\r\n"
                    "Via: SIP/2.0/UDP speedyspc.biloxi.com:5061;branch=sfourth\r\n"
                    "Max-Forwards: 7\r\n"
                    "To: Speedy <sip:speedy@biloxi.com>\r\n"
                    "From: Belle <sip:belle@biloxi.com>;tag=88888\r\n"
                    "Call-ID: 88888@8888\r\n"
                    "CSeq: 6281 REGISTER\r\n"
                    "Contact: <sip:belle@192.0.2.4>\r\n"
                    "Contact: <sip:qoq@192.0.2.4>\r\n"
                    "Expires: 2700\r\n"
                    "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message2(TestSupport::makeMessage(txt2));

      assert(message2->header(h_RequestLine).getMethod() == REGISTER);
      assert(message2->header(h_To).uri().user() == "speedy");
      assert(message2->header(h_From).uri().user() == "belle");
      assert(message2->header(h_MaxForwards).value() == 7);
      for (NameAddrs::iterator i = message2->header(h_Contacts).begin();
           i != message2->header(h_Contacts).end(); i++)
      {
         i->uri();
      }

      for (Vias::iterator i = message2->header(h_Vias).begin();
           i != message2->header(h_Vias).end(); i++)
      {
         assert(i->sentPort() == 5061);
      }
      assert(message2->header(h_CallId).value() == "88888@8888");
      assert(message2->header(h_CSeq).sequence() == 6281);
      assert(message2->header(h_CSeq).method() == REGISTER);
      assert(message2->header(h_Vias).empty() == false);
      assert(message2->header(h_Vias).size() == 4);
      assert(message2->header(h_Expires).value() == 2700);
      assert(message2->header(h_ContentLength).value() == 0);

      // copy over everything
      message1->header(h_RequestLine) = message2->header(h_RequestLine);
      message1->header(h_Vias) = message2->header(h_Vias);
      message1->header(h_MaxForwards) = message2->header(h_MaxForwards);
      message1->header(h_To) = message2->header(h_To);
      message1->header(h_From) = message2->header(h_From);
      message1->header(h_CallId) = message2->header(h_CallId);
      message1->header(h_CSeq) = message2->header(h_CSeq);
      message1->header(h_Contacts) = message2->header(h_Contacts);
      message1->header(h_Expires) = message2->header(h_Expires);
      message1->header(h_ContentLength) = message2->header(h_ContentLength);

      assert(message1->header(h_To).uri().user() == "speedy");
      assert(message1->header(h_From).uri().user() == "belle");
      assert(message1->header(h_MaxForwards).value() == 7);
      assert(message1->header(h_Contacts).empty() == false);
      assert(message1->header(h_CallId).value() == "88888@8888");
      assert(message1->header(h_CSeq).sequence() == 6281);
      assert(message1->header(h_CSeq).method() == REGISTER);
      assert(message1->header(h_Vias).empty() == false);
      assert(message1->header(h_Vias).size() == 4);
      assert(message1->header(h_Expires).value() == 2700);
      assert(message1->header(h_ContentLength).value() == 0);
      assert(message1->header(h_RequestLine).uri().getAor() == "registrar.ixolib.com");
   }

   {
      cerr << "test unparsed message copy" << endl;
      char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Contact: <sip:qoq@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      SipMessage copy(*message);
      copy.encode(cerr);
      cerr << endl;
   }
   
   {
      cerr << "test header creation" << endl;
      SipMessage message;

      message.header(h_CSeq).sequence() = 123456;
      assert(message.header(h_CSeq).sequence() == 123456);

      message.header(h_To).uri().user() = "speedy";
      assert(message.header(h_To).uri().user() == "speedy");
      
      message.encode(cerr);

   }
   
   {
      cerr << "test multiheaders access" << endl;

      char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Contact: <sip:qoq@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      cerr << "Encode from unparsed: " << endl;
      message->encode(cerr);

      assert(message->header(h_To).uri().user() == "bob");
      assert(message->header(h_From).uri().user() == "bob");
      assert(message->header(h_MaxForwards).value() == 70);
      assert(message->header(h_Contacts).empty() == false);
      assert(message->header(h_CallId).value() == "843817637684230@998sdasdh09");
      assert(message->header(h_CSeq).sequence() == 1826);
      assert(message->header(h_CSeq).method() == REGISTER);
      assert(message->header(h_Vias).empty() == false);
      assert(message->header(h_Vias).size() == 5);
      assert(message->header(h_Expires).value() == 7200);
      assert(message->header(h_ContentLength).value() == 0);
      assert(message->header(h_RequestLine).uri().getAor() == "registrar.biloxi.com");
      
      cerr << "Encode from parsed: " << endl;
      message->encode(cerr);

      message->header(h_Contacts).front().uri().user() = "jason";

      cerr << "Encode after messing: " << endl;
      message->encode(cerr);

      SipMessage copy(*message);
      assert(copy.header(h_To).uri().user() == "bob");
      assert(copy.header(h_From).uri().user() == "bob");
      assert(copy.header(h_MaxForwards).value() == 70);
      assert(copy.header(h_Contacts).empty() == false);
      assert(copy.header(h_CallId).value() == "843817637684230@998sdasdh09");
      assert(copy.header(h_CSeq).sequence() == 1826);
      assert(copy.header(h_CSeq).method() == REGISTER);
      assert(copy.header(h_Vias).empty() == false);
      assert(copy.header(h_Vias).size() == 5);
      assert(copy.header(h_Expires).value() == 7200);
      assert(copy.header(h_ContentLength).value() == 0);
      cerr << "RequestLine Uri AOR = " << copy.header(h_RequestLine).uri().getAor() << endl;
      assert(copy.header(h_RequestLine).uri().getAor() == "registrar.biloxi.com");


      cerr << "Encode after copying: " << endl;
      copy.encode(cerr);
   }
   
   {
      cerr << "test callId access" << endl;

      char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      message->encode(cerr);
      
      //Data v = message->header(h_CallId).value();
      assert(message->header(h_CallId).value() == "843817637684230@998sdasdh09");
      //StatusLine& foo = message->header(h_StatusLine);
      //RequestLine& bar = message->header(h_RequestLine);
      //cerr << bar.getMethod() << endl;
   }
   
   {
      char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248;mobility=hobble\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
          
      Data v = message->header(h_CallId).value();
      cerr << "Call-ID is " << v << endl;

      message->encode(cerr);
  
      //StatusLine& foo = message->header(h_StatusLine);
      //RequestLine& bar = message->header(h_RequestLine);
      //cerr << bar.getMethod() << endl;
   }

   {
      
      char *txt = ("REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                   "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\n"
                   "Max-Forwards: 70\r\n"
                   "To: Bob <sip:bob@biloxi.com>\r\n"
                   "From: Bob <sip:bob@biloxi.com>;tag=456248;mobility=hobble\r\n"
                   "Call-ID: 843817637684230@998sdasdh09\r\n"
                   "CSeq: 1826 REGISTER\r\n"
                   "Contact: <sip:bob@192.0.2.4>\r\n"
                   "Expires: 7200\r\n"
                   "Content-Length: 0\r\n\r\n");
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      assert(message->getRawHeader(Headers::From));
      assert(&message->header(h_From));
      assert(message->header(h_From).exists(p_tag) == true);
      assert(message->header(h_From).exists(p_mobility) == true);
      assert(message->header(h_From).param(p_tag) == "456248");
      assert(message->header(h_From).param(p_mobility) == "hobble");

      message->encode(cerr);
  
      //StatusLine& foo = message->header(h_StatusLine);
      //RequestLine& bar = message->header(h_RequestLine);
      //cerr << bar.getMethod() << endl;
   }

   {
      cerr << "first REGISTER in torture test" << endl;
      
      char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->isRequest());
      assert(message->isResponse() == false);

      assert(message->exists(h_To));
      assert(message->header(h_To).uri().user() == "user");
      assert(message->header(h_To).uri().host() == "company.com");
      assert(message->header(h_To).uri().exists(p_tag) == false);

      assert(message->exists(h_From));
      assert(message->header(h_From).uri().user() == "user");
      assert(message->header(h_From).uri().host() == "company.com");
      assert(message->header(h_From).param(p_tag) == "3411345");

      assert(message->exists(h_MaxForwards));
      assert(message->header(h_MaxForwards).value() == 8);
      assert(message->header(h_MaxForwards).exists(p_tag) == false);

      assert(message->exists(h_Contacts));
      assert(message->header(h_Contacts).empty() == false);
      assert(message->header(h_Contacts).front().uri().user() == "user");
      assert(message->header(h_Contacts).front().uri().host() == "host.company.com");
      assert(message->header(h_Contacts).front().uri().port() == 0);

      assert(message->exists(h_CallId));
      assert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      assert(message->exists(h_CSeq));
      assert(message->header(h_CSeq).sequence() == 8);
      assert(message->header(h_CSeq).method() == REGISTER);

      assert(message->exists(h_Vias));
      assert(message->header(h_Vias).empty() == false);
      assert(message->header(h_Vias).front().protocolName() == "SIP");
      assert(message->header(h_Vias).front().protocolVersion() == "2.0");
      assert(message->header(h_Vias).front().transport() == "UDP");
      assert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      assert(message->header(h_Vias).front().sentPort() == 0);

      assert(message->exists(h_Expires));
      assert(message->header(h_Expires).value() == 353245);

      cerr << "Headers::Expires enum = " << h_Expires.getTypeNum() << endl;
      
      message->encode(cerr);
   }

   {
      cerr << "first REGISTER in torture test" << endl;
      
      char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->header(h_MaxForwards).value() == 8);
      message->getRawHeader(Headers::MaxForwards)->getParserContainer()->encode(Headers::getHeaderName(Headers::MaxForwards), cerr) << endl;
   }

   {
      cerr << "response to REGISTER" << endl;
      
      char *txt = ("SIP/2.0 100 Trying\r\n"
                   "To: sip:localhost:5070\r\n"
                   "From: sip:localhost:5070;tag=73483366\r\n"
                   "Call-ID: 51dcb07418a21008e0ba100800000000\r\n"
                   "CSeq: 1 REGISTER\r\n"
                   "Via: SIP/2.0/UDP squamish.gloo.net:5060;branch=z9hG4bKff5c491951e40f08\r\n"
                   "Content-Length: 0\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      assert(message->header(h_To).uri().host() == "localhost");
   }
   {
      NameAddr me;
      me.uri().host() = "localhost";
      me.uri().port() = 5070;
      //auto_ptr<SipMessage> msg(Helper::makeRegister(me, me));
      auto_ptr<SipMessage> msg(Helper::makeRegister(me, me, me));
      cerr << "encoded=" << *msg << endl;
   }
   {
      char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      message->header(h_RequestLine).uri();
      auto_ptr<SipMessage> copy(new SipMessage(*message));
      assert(message->header(h_RequestLine).getMethod() == copy->header(h_RequestLine).getMethod());
   }
   {
      char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "Security-Client: ipsec-ike;d-alg=md5;q=0.1\r\n"
                   "Security-Server: tls;q=0.2;d-qop=verify\r\n"
                   "Security-Verify: tls;q=0.2;d-ver=\"0000000000000000000000000000abcd\"\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      assert(message->header(h_SecurityClients).front().value() == "ipsec-ike");
      assert(message->header(h_SecurityClients).front().param(p_dAlg) == "md5");
      assert(message->header(h_SecurityClients).front().param(p_q) == 0.1f);

      assert(message->header(h_SecurityServers).front().value() == "tls");
      assert(message->header(h_SecurityServers).front().param(p_dQop) == "verify");
      assert(message->header(h_SecurityServers).front().param(p_q) == 0.2f);

      assert(message->header(h_SecurityVerifies).front().value() == "tls");
      assert(message->header(h_SecurityVerifies).front().param(p_dVer) == "0000000000000000000000000000abcd");
      assert(message->header(h_SecurityVerifies).front().param(p_q) == 0.2f);

      assert(message->exists(h_AllowEvents) == false);
      assert(message->header(h_AllowEvents).size() == 1);
      assert(message->header(h_AllowEvents).front().value().empty());
   }

   {
      char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      char *txt2 = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "Security-Client: ipsec-ike;d-alg=md5;q=0.1\r\n"
                   "Security-Server: tls;q=0.2;d-qop=verify\r\n"
                   "Security-Verify: tls;q=0.2;d-ver=\"0000000000000000000000000000abcd\"\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: 353245\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      
      {
         Token sec;
         sec.value() = "ipsec-ike";
         sec.param(p_dAlg) = "md5";
         sec.param(p_q) = 0.1f;
         message->header(h_SecurityClients).push_back(sec);
      }
      {
         Token sec;
         sec.value() = "tls";
         sec.param(p_q) = 0.2f;
         sec.param(p_dQop) = "verify";
         message->header(h_SecurityServers).push_back(sec);
      }
      {
         Token sec;
         sec.value() = "tls";
         sec.param(p_q) = 0.2f;
         sec.param(p_dVer) = "0000000000000000000000000000abcd";
         message->header(h_SecurityVerifies).push_back(sec);
      }

      auto_ptr<SipMessage> message2(TestSupport::makeMessage(txt2));

      Data msgEncoded;
      {
         DataStream s(msgEncoded);
         s << *message;
      }
      
      Data msg2Encoded;
      {
         DataStream s(msg2Encoded);
         s << *message2;
      }
      
      assert(msgEncoded == msg2Encoded);
   }

   {
      Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
               "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
               "To: Bob <sip:bob@biloxi.com>\r\n"
               "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
               "Call-ID: a84b4c76e66710\r\n"
               "CSeq: 314159 INVITE\r\n"
               "Max-Forwards: 70\r\n"
               "Accepts: \r\n"
               "Foobie-Blech: \r\n"
               "Contact: <sip:alice@pc33.atlanta.com>\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: 150\r\n"
               "\r\n"
               "v=0\r\n"
               "o=alice 53655765 2353687637 IN IP4 pc33.atlanta.com\r\n"
               "s=-\r\n"
               "c=IN IP4 pc33.atlanta.com\r\n"
               "t=0 0\r\n"
               "m=audio 3456 RTP/AVP 0 1 3 99\r\n"
               "a=rtpmap:0 PCMU/8000\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      assert(msg->exists(h_Accepts));
      assert(msg->header(h_Accepts).empty());
      
      assert(msg->exists(UnknownHeaderType("Foobie-Blech")));
      assert(msg->header(UnknownHeaderType("Foobie-Blech")).empty());
   }

   cerr << "\nTEST OK" << endl;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
