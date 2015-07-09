#include "resip/stack/GenericPidfContents.hxx"
#include <iostream>
#include "rutil/Logger.hxx"
#include "rutil/HashMap.hxx"
#include "TestSupport.hxx"

//#define ENABLE_VLD
#ifdef ENABLE_VLD
#include "vld.h"
#endif

using namespace resip;
using namespace std;

#define CRLF "\r\n"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

#ifdef ENABLE_VLD
   VLDMarkAllLeaksAsReported();
#endif
   // Timestamp generation test
   {
      time_t zerotime = { 0 };
      Data timestamp = GenericPidfContents::generateTimestampData(zerotime);
      cout << timestamp << endl;
      assert(timestamp == "1970-01-01T00:00:00Z");
   }

   // Create simple Pidf test
   {
      GenericPidfContents pidf;
      pidf.setEntity(Uri("sip:entity@domain"));
      pidf.setSimplePresenceTupleNode("1234", true, "2005-05-30T22:00:29Z", "Online and ready to go", "sip:entity@domain", "0.8");
      cout << pidf << endl;
      assert(pidf.getEntity() == Uri("sip:entity@domain"));
      assert(pidf.getNamespaces().size() == 1);
      assert(pidf.getRootPidfNamespacePrefix().empty());
      assert(pidf.getRootNodes().size() == 1);
      assert(pidf.getSimplePresenceTupleId() == "1234");
      assert(pidf.getSimplePresenceOnline() == true);
      assert(pidf.getSimplePresenceTimestamp() == "2005-05-30T22:00:29Z");
      assert(pidf.getSimplePresenceNote() == "Online and ready to go");
      assert(pidf.getSimplePresenceContact() == "sip:entity@domain");
      assert(pidf.getSimplePresenceContactPriority() == "0.8");
      pidf.setSimplePresenceTupleNode("1234", false, "2005-05-30T22:00:30Z", "Offline", "sip:entity2@domain", "0.7");
      cout << pidf << endl;
      assert(pidf.getEntity() == Uri("sip:entity@domain"));
      assert(pidf.getNamespaces().size() == 1);
      assert(pidf.getRootPidfNamespacePrefix().empty());
      assert(pidf.getRootNodes().size() == 1);
      assert(pidf.getSimplePresenceTupleId() == "1234");
      assert(pidf.getSimplePresenceOnline() == false);
      assert(pidf.getSimplePresenceTimestamp() == "2005-05-30T22:00:30Z");
      assert(pidf.getSimplePresenceNote() == "Offline");
      assert(pidf.getSimplePresenceContact() == "sip:entity2@domain");
      assert(pidf.getSimplePresenceContactPriority() == "0.7");
   }

   // Test SipMessage Parse
   {
       Data txt(
           "INVITE sip:bob@biloxi.com SIP/2.0\r\n"
           "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8\r\n"
           "To: Bob <sip:bob@biloxi.com>\r\n"
           "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
           "Call-ID: a84b4c76e66710\r\n"
           "CSeq: 314159 INVITE\r\n"
           "Max-Forwards: 70\r\n"
           "Contact: <sip:alice@pc33.atlanta.com>\r\n"
           "Content-Type: application/pidf+xml\r\n"
           "Content-Length: 8000\r\n"
           "\r\n"
           "<?xml version='1.0' encoding='UTF-8'?>" CRLF
           "<presence xmlns='urn:ietf:params:xml:ns:pidf'" CRLF
           "          xmlns:dm='urn:ietf:params:xml:ns:pidf:data-model'" CRLF
           "          xmlns:rpid='urn:ietf:params:xml:ns:pidf:rpid'" CRLF
           "          xmlns:c='urn:ietf:params:xml:ns:pidf:cipid'" CRLF
           "        entity='sip:5000@blitzzgod.com'>" CRLF
           "  <tuple id='ta054033a'>" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "  </tuple>" CRLF
           "  <dm:person id='p23008e26'>" CRLF
           "    <rpid:activities>" CRLF
           "      <rpid:unknown/>" CRLF
           "    </rpid:activities>" CRLF
           "  </dm:person>" CRLF
           "</presence>" CRLF
           );
       try
       {
           auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
           Contents* body = msg->getContents();

           assert(body != 0);
           GenericPidfContents* pidf = dynamic_cast<GenericPidfContents*>(body);
           if (pidf)
           {
               assert(pidf->getEntity() == Uri("sip:5000@blitzzgod.com"));
               assert(pidf->getNamespaces().size() == 4);
               assert(pidf->getRootPidfNamespacePrefix().empty());
               assert(pidf->getRootNodes().size() == 2);
               assert(pidf->getRootNodes().front()->mTag == "tuple");
               assert(pidf->getRootNodes().front()->mAttributes.size() == 1);
               assert(pidf->getRootNodes().front()->mAttributes["id"] == "ta054033a");
               assert(pidf->getRootNodes().front()->mChildren.size() == 1);
               assert(pidf->getRootNodes().front()->mChildren.front()->mTag == "status");
               assert(pidf->getRootNodes().front()->mChildren.front()->mChildren.size() == 1);
               assert(pidf->getRootNodes().front()->mChildren.front()->mChildren.front()->mTag == "basic");
               assert(pidf->getRootNodes().front()->mChildren.front()->mChildren.front()->mValue == "open");
               assert(pidf->getRootNodes().back()->mTag == "person");
               assert(pidf->getRootNodes().back()->mAttributes.size() == 1);
               assert(pidf->getRootNodes().back()->mAttributes["id"] == "p23008e26");
               assert(pidf->getRootNodes().back()->mChildren.size() == 1);
               assert(pidf->getRootNodes().back()->mChildren.front()->mTag == "activities");
               assert(pidf->getRootNodes().back()->mChildren.front()->mChildren.size() == 1);
               assert(pidf->getRootNodes().back()->mChildren.front()->mChildren.front()->mTag == "unknown");
               assert(pidf->getRootNodes().back()->mChildren.front()->mChildren.front()->mValue.empty());
               assert(pidf->getSimplePresenceTupleId() == "ta054033a");
               assert(pidf->getSimplePresenceOnline() == true);
               assert(pidf->getSimplePresenceTimestamp() == Data::Empty);
               assert(pidf->getSimplePresenceNote() == Data::Empty);
               assert(pidf->getSimplePresenceContact() == Data::Empty);
               assert(pidf->getSimplePresenceContactPriority() == Data::Empty);
               cout << *pidf << endl;
           }
           else
           {
               assert(false);
           }
       }
       catch (BaseException& e)
       {
          cerr << e << endl;
          assert(false);
       }
   }

   {
       // From XLite
       const Data txt(
           "<?xml version='1.0' encoding='UTF-8'?>" CRLF
           "<presence xmlns='urn:ietf:params:xml:ns:pidf'" CRLF
           "          xmlns:dm='urn:ietf:params:xml:ns:pidf:data-model'" CRLF
           "          xmlns:rpid='urn:ietf:params:xml:ns:pidf:rpid'" CRLF
           "          xmlns:c='urn:ietf:params:xml:ns:pidf:cipid'" CRLF
           "        entity='sip:5000@blitzzgod.com'>" CRLF
           "  <tuple id='ta054033a'>" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "  </tuple>" CRLF
           "  <dm:person id='p23008e26'>" CRLF
           "    <rpid:activities>" CRLF
           "      <rpid:unknown/>" CRLF
           "    </rpid:activities>" CRLF
           "  </dm:person>" CRLF
           "</presence>" CRLF
           );

       HeaderFieldValue hfv(txt.data(), txt.size());
       GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType());
       assert(pidf.getEntity() == Uri("sip:5000@blitzzgod.com"));
       assert(pidf.getNamespaces().size() == 4);
       assert(pidf.getRootPidfNamespacePrefix().empty());
       assert(pidf.getRootNodes().size() == 2);
       assert(pidf.getRootNodes().front()->mTag == "tuple");
       assert(pidf.getRootNodes().front()->mAttributes.size() == 1);
       assert(pidf.getRootNodes().front()->mAttributes["id"] == "ta054033a");
       assert(pidf.getRootNodes().front()->mChildren.size() == 1);
       assert(pidf.getRootNodes().front()->mChildren.front()->mTag == "status");
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.size() == 1);
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.front()->mTag == "basic");
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.front()->mValue == "open");
       assert(pidf.getRootNodes().back()->mTag == "person");
       assert(pidf.getRootNodes().back()->mAttributes.size() == 1);
       assert(pidf.getRootNodes().back()->mAttributes["id"] == "p23008e26");
       assert(pidf.getRootNodes().back()->mChildren.size() == 1);
       assert(pidf.getRootNodes().back()->mChildren.front()->mTag == "activities");
       assert(pidf.getRootNodes().back()->mChildren.front()->mChildren.size() == 1);
       assert(pidf.getRootNodes().back()->mChildren.front()->mChildren.front()->mTag == "unknown");
       assert(pidf.getRootNodes().back()->mChildren.front()->mChildren.front()->mValue.empty());
       assert(pidf.getSimplePresenceTupleId() == "ta054033a");
       assert(pidf.getSimplePresenceOnline() == true);
       assert(pidf.getSimplePresenceTimestamp() == Data::Empty);
       assert(pidf.getSimplePresenceNote() == Data::Empty);
       assert(pidf.getSimplePresenceContact() == Data::Empty);
       assert(pidf.getSimplePresenceContactPriority() == Data::Empty);
       cout << pidf << endl;
   }

   {
       // From SNOM
       Data txt(
           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
           "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"" CRLF
           "          xmlns:im=\"urn:ietf:params:xml:ns:pidf:im\"" CRLF
           "        entity=\"pres:2002@blitzzgod.com\">" CRLF
           "  <tuple id=\"snom370-000413261510\">" CRLF
           "    <status>" CRLF
           "      <basic>closed</basic>" CRLF
           "    </status>" CRLF
           "    <contact priority=\"1.00\">sip:2002@blitzzgod.com</contact>" CRLF
           "  </tuple>" CRLF
           "</presence>" CRLF
           );
       HeaderFieldValue hfv(txt.data(), txt.size());
       GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType());
       assert(pidf.getEntity() == Uri("pres:2002@blitzzgod.com"));
       assert(pidf.getNamespaces().size() == 2);
       assert(pidf.getRootPidfNamespacePrefix().empty());
       assert(pidf.getRootNodes().size() == 1);
       assert(pidf.getRootNodes().front()->mTag == "tuple");
       assert(pidf.getRootNodes().front()->mAttributes.size() == 1);
       assert(pidf.getRootNodes().front()->mAttributes["id"] == "snom370-000413261510");
       assert(pidf.getRootNodes().front()->mChildren.size() == 2);
       assert(pidf.getRootNodes().front()->mChildren.front()->mTag == "status");
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.size() == 1);
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.front()->mTag == "basic");
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.front()->mValue == "closed");
       assert(pidf.getRootNodes().front()->mChildren.back()->mTag == "contact");
       assert(pidf.getRootNodes().front()->mChildren.back()->mValue == "sip:2002@blitzzgod.com");
       assert(pidf.getRootNodes().front()->mChildren.back()->mAttributes["priority"] == "1.00");
       assert(pidf.getSimplePresenceTupleId() == "snom370-000413261510");
       assert(pidf.getSimplePresenceOnline() == false);
       assert(pidf.getSimplePresenceTimestamp() == Data::Empty);
       assert(pidf.getSimplePresenceNote() == Data::Empty);
       assert(pidf.getSimplePresenceContact() == "sip:2002@blitzzgod.com");
       assert(pidf.getSimplePresenceContactPriority() == "1.00");
       cout << pidf << endl;
   }

   {
       // From Jitsi
       Data txt(
           "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" CRLF
           "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" " CRLF
           "          xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" " CRLF
           "          xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" " CRLF
           "        entity=\"sip:2002@blitzzgod.com\">" CRLF
           "  <dm:person id=\"p719\">" CRLF
           "    <rpid:activities/>" CRLF
           "  </dm:person>" CRLF
           "  <tuple id=\"t8642\">" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "    <contact>sip:2002@blitzzgod.com</contact>" CRLF
           "    <note>Online</note>" CRLF
           "  </tuple>" CRLF
           "</presence>" CRLF
           );
       HeaderFieldValue hfv(txt.data(), txt.size());
       GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType());
       assert(pidf.getEntity() == Uri("sip:2002@blitzzgod.com"));
       assert(pidf.getNamespaces().size() == 3);
       assert((*((GenericPidfContents::NamespaceMap*)&pidf.getNamespaces()))["urn:ietf:params:xml:ns:pidf"] == "");
       assert((*((GenericPidfContents::NamespaceMap*)&pidf.getNamespaces()))["urn:ietf:params:xml:ns:pidf:data-model"] == "dm:");
       assert((*((GenericPidfContents::NamespaceMap*)&pidf.getNamespaces()))["urn:ietf:params:xml:ns:pidf:rpid"] == "rpid:");
       assert(pidf.getRootPidfNamespacePrefix().empty());
       assert(pidf.getRootNodes().size() == 2);
       assert(pidf.getRootNodes().front()->mTag == "person");
       assert(pidf.getRootNodes().front()->mAttributes.size() == 1);
       assert(pidf.getRootNodes().front()->mAttributes["id"] == "p719");
       assert(pidf.getRootNodes().front()->mChildren.size() == 1);
       assert(pidf.getRootNodes().front()->mChildren.front()->mTag == "activities");
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.size() == 0);
       assert(pidf.getRootNodes().back()->mTag == "tuple");
       assert(pidf.getRootNodes().back()->mAttributes.size() == 1);
       assert(pidf.getRootNodes().back()->mAttributes["id"] == "t8642");
       assert(pidf.getRootNodes().back()->mChildren.size() == 3);
       assert(pidf.getRootNodes().back()->mChildren.front()->mTag == "status");
       assert(pidf.getRootNodes().back()->mChildren.front()->mChildren.size() == 1);
       assert(pidf.getRootNodes().back()->mChildren.front()->mChildren.front()->mTag == "basic");
       assert(pidf.getRootNodes().back()->mChildren.front()->mChildren.front()->mValue == "open");
       assert((*++(pidf.getRootNodes().back()->mChildren.begin()))->mTag == "contact");
       assert((*++(pidf.getRootNodes().back()->mChildren.begin()))->mValue == "sip:2002@blitzzgod.com");
       assert(pidf.getRootNodes().back()->mChildren.back()->mTag == "note");
       assert(pidf.getRootNodes().back()->mChildren.back()->mValue == "Online");
       assert(pidf.getSimplePresenceTupleId() == "t8642");
       assert(pidf.getSimplePresenceOnline() == true);
       assert(pidf.getSimplePresenceTimestamp() == Data::Empty);
       assert(pidf.getSimplePresenceNote() == "Online");
       assert(pidf.getSimplePresenceContact() == "sip:2002@blitzzgod.com");
       assert(pidf.getSimplePresenceContactPriority() == Data::Empty);
       cout << pidf << endl;
   }

   {
       // From RFC3863 - Namespace example
       Data txt(
           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
           "<impp:presence xmlns:impp=\"urn:ietf:params:xml:ns:pidf\"" CRLF
           "      entity=\"pres:someone@example.com\">" CRLF
           "  <impp:tuple id=\"mobile-im\">" CRLF
           "    <impp:status>" CRLF
           "      <impp:basic>open</impp:basic>" CRLF
           "    </impp:status>" CRLF
           "    <impp:contact priority=\"0.8\">tel:+09012345678</impp:contact>" CRLF
           "  </impp:tuple>" CRLF
           "</impp:presence>" CRLF
           );
       HeaderFieldValue hfv(txt.data(), txt.size());
       GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType());
       assert(pidf.getEntity() == Uri("pres:someone@example.com"));
       assert(pidf.getNamespaces().size() == 1);
       assert((*((GenericPidfContents::NamespaceMap*)&pidf.getNamespaces()))["urn:ietf:params:xml:ns:pidf"] == "impp:");
       assert(pidf.getRootPidfNamespacePrefix() == "impp:");
       assert(pidf.getRootNodes().size() == 1);
       assert(pidf.getRootNodes().front()->mTag == "tuple");
       assert(pidf.getRootNodes().front()->mAttributes.size() == 1);
       assert(pidf.getRootNodes().front()->mAttributes["id"] == "mobile-im");
       assert(pidf.getRootNodes().front()->mChildren.size() == 2);
       assert(pidf.getRootNodes().front()->mChildren.front()->mTag == "status");
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.size() == 1);
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.front()->mTag == "basic");
       assert(pidf.getRootNodes().front()->mChildren.front()->mChildren.front()->mValue == "open");
       assert(pidf.getRootNodes().front()->mChildren.back()->mTag == "contact");
       assert(pidf.getRootNodes().front()->mChildren.back()->mValue == "tel:+09012345678");
       assert(pidf.getRootNodes().front()->mChildren.back()->mChildren.size() == 0);
       assert(pidf.getRootNodes().front()->mChildren.back()->mAttributes["priority"] == "0.8");
       assert(pidf.getSimplePresenceTupleId() == "mobile-im");
       assert(pidf.getSimplePresenceOnline() == true);
       assert(pidf.getSimplePresenceTimestamp() == Data::Empty);
       assert(pidf.getSimplePresenceNote() == Data::Empty);
       assert(pidf.getSimplePresenceContact() == "tel:+09012345678");
       assert(pidf.getSimplePresenceContactPriority() == "0.8");
       cout << pidf << endl;
   }

   {
       // From RFC4479 - data-model
       Data txt(
           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
           "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"" CRLF
           "          xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\"" CRLF
           "          xmlns:rp=\"urn:ietf:params:xml:ns:pidf:rpid\"" CRLF
           "          xmlns:caps=\"urn:ietf:params:xml:ns:pidf:caps\"" CRLF
           "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">" CRLF
           "  <tuple id=\"sg89ae\">" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "    <dm:deviceID>mac:8asd7d7d70</dm:deviceID>" CRLF
           "    <caps:servcaps>" CRLF
           "      <caps:extensions>" CRLF
           "       <caps:supported>" CRLF
           "        <caps:pref/>" CRLF
           "       </caps:supported>" CRLF
           "      </caps:extensions>" CRLF
           "      <caps:methods>" CRLF
           "       <caps:supported>" CRLF
           "        <caps:MESSAGE/>" CRLF
           "        <caps:OPTIONS/>" CRLF
           "       </caps:supported>" CRLF
           "      </caps:methods>" CRLF
           "    </caps:servcaps>" CRLF
           "    <contact>sip:someone@example.com</contact>" CRLF
           "  </tuple>" CRLF
           "  <dm:person id=\"p1\">" CRLF
           "    <rp:activities>" CRLF
           "      <rp:on-the-phone/>" CRLF
           "    </rp:activities>" CRLF
           "  </dm:person>" CRLF
           "  <dm:device id=\"pc122\">" CRLF
           "    <rp:user-input>idle</rp:user-input>" CRLF
           "    <dm:deviceID>mac:8asd7d7d70</dm:deviceID>" CRLF
           "  </dm:device>" CRLF
           "</presence>" CRLF
           );
       HeaderFieldValue hfv(txt.data(), txt.size());
       GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType());
       assert(pidf.getNamespaces().size() == 5);
       assert(pidf.getRootPidfNamespacePrefix().empty());
       assert(pidf.getRootNodes().size() == 3);
       assert(pidf.getRootNodes().front()->mTag == "tuple");
       assert(pidf.getRootNodes().front()->mChildren.size() == 4);
       assert(pidf.getRootNodes().front()->mChildren.front()->mTag == "status");
       assert(pidf.getRootNodes().front()->mChildren.back()->mTag == "contact");
       assert(pidf.getRootNodes().back()->mTag == "device");
       assert(pidf.getRootNodes().back()->mChildren.size() == 2);
       assert(pidf.getRootNodes().back()->mChildren.front()->mTag == "user-input");
       assert(pidf.getRootNodes().back()->mChildren.front()->mValue == "idle");
       assert(pidf.getRootNodes().back()->mChildren.back()->mTag == "deviceID");
       assert(pidf.getRootNodes().back()->mChildren.back()->mValue == "mac:8asd7d7d70");
       cout << pidf << endl;
   }

   {
       // From RFC4428 - cipid
       Data txt(
           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
           "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"" CRLF
           "          xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" CRLF
           "          xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\"" CRLF
           "          xmlns:c=\"urn:ietf:params:xml:ns:pidf:cipid\"" CRLF
           "          xmlns:r=\"urn:ietf:params:xml:ns:pidf:rpid\"" CRLF
           "          xsi:schemaLocation=\"urn:ietf:params:xml:ns:pidf pidf.xsd" CRLF
           "                              urn:ietf:params:xml:ns:pidf:data-model data-model.xsd" CRLF
           "                              urn:ietf:params:xml:ns:pidf:cipid cipid.xsd" CRLF
           "                              urn:ietf:params:xml:ns:pidf:rpid rpid.xsd\"" CRLF
           "       entity=\"pres:someone@example.com\">" CRLF
           "  <tuple id=\"bs35r9\">" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "    <contact priority=\"0.8\">im:someone@mobile.example.net</contact>" CRLF
           "    <timestamp>2005-05-30T22:00:29Z</timestamp>" CRLF
           "  </tuple>" CRLF
           "" CRLF
           "  <tuple id=\"bs78\">" CRLF
           "    <status>" CRLF
           "      <basic>closed</basic>" CRLF
           "    </status>" CRLF
           "    <r:relationship><r:assistant/></r:relationship>" CRLF
           "    <c:card>http://example.com/~assistant/card.vcd</c:card>" CRLF
           "    <c:homepage>http://example.com/~assistant</c:homepage>" CRLF
           "    <contact priority=\"0.1\">im:assistant@example.com</contact>" CRLF
           "    <timestamp>2005-05-30T22:00:29Z</timestamp>" CRLF
           "  </tuple>" CRLF
           "" CRLF
           "  <dm:person id=\"p1\">" CRLF
           "    <c:card>http://example.com/~someone/card.vcd</c:card>" CRLF
           "    <c:homepage>http://example.com/~someone</c:homepage>" CRLF
           "    <c:icon>http://example.com/~someone/icon.gif</c:icon>" CRLF
           "    <c:map>http://example.com/~someone/gml-map.xml</c:map>" CRLF
           "    <c:sound>http://example.com/~someone/whoosh.wav</c:sound>" CRLF
           "    <dm:timestamp>2005-05-30T22:02:44+05:00</dm:timestamp>" CRLF
           "  </dm:person>" CRLF
           "</presence>" CRLF
           );
       HeaderFieldValue hfv(txt.data(), txt.size());
       GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType());
       assert(pidf.getEntity() == Uri("pres:someone@example.com"));
       assert(pidf.getNamespaces().size() == 5);
       assert(pidf.getRootPidfNamespacePrefix().empty());
       assert(pidf.getRootNodes().size() == 3);
       assert(pidf.getRootNodes().front()->mTag == "tuple");
       assert(pidf.getRootNodes().front()->mChildren.size() == 3);
       assert(pidf.getRootNodes().front()->mChildren.front()->mTag == "status");
       assert(pidf.getRootNodes().front()->mChildren.back()->mTag == "timestamp");
       assert(pidf.getRootNodes().front()->mChildren.back()->mValue == "2005-05-30T22:00:29Z");       
       assert(pidf.getRootNodes().back()->mTag == "person");
       assert(pidf.getRootNodes().back()->mChildren.size() == 6);
       assert(pidf.getRootNodes().back()->mChildren.front()->mTag == "card");
       assert(pidf.getRootNodes().back()->mChildren.front()->mValue == "http://example.com/~someone/card.vcd");
       assert(pidf.getRootNodes().back()->mChildren.back()->mTag == "timestamp");
       assert(pidf.getRootNodes().back()->mChildren.back()->mValue == "2005-05-30T22:02:44+05:00");
       assert(pidf.getSimplePresenceTupleId() == "bs35r9");
       assert(pidf.getSimplePresenceOnline() == true);
       assert(pidf.getSimplePresenceTimestamp() == "2005-05-30T22:00:29Z");
       assert(pidf.getSimplePresenceNote() == Data::Empty);
       assert(pidf.getSimplePresenceContact() == "im:someone@mobile.example.net");
       assert(pidf.getSimplePresenceContactPriority() == "0.8");
       cout << pidf << endl;
   }

   {
       // From RFC4480 - rpid
       Data txt(
           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
           "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"" CRLF
           "          xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\"" CRLF
           "          xmlns:lt=\"urn:ietf:params:xml:ns:location-type\"" CRLF
           "          xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\"" CRLF
           "       entity=\"pres:someone@example.com\">" CRLF
           CRLF
           "  <tuple id=\"bs35r9\">" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "    <dm:deviceID>urn:device:0003ba4811e3</dm:deviceID>" CRLF
           "    <rpid:relationship><rpid:self/></rpid:relationship>" CRLF
           "    <rpid:service-class><rpid:electronic/></rpid:service-class>" CRLF
           "    <contact priority=\"0.8\">im:someone@mobile.example.net</contact>" CRLF
           "    <note xml:lang=\"en\">Don't Disturb Please!</note>" CRLF
           "    <note xml:lang=\"fr\">Ne derangez pas, s'il vous plait</note>" CRLF
           "    <timestamp>2005-10-27T16:49:29Z</timestamp>" CRLF
           "  </tuple>" CRLF
           CRLF
           "  <tuple id=\"ty4658\">" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "    <rpid:relationship><rpid:assistant/></rpid:relationship>" CRLF
           "    <contact priority=\"1.0\">mailto:secretary@example.com</contact>" CRLF
           "  </tuple>" CRLF
           CRLF
           "  <tuple id=\"eg92n8\">" CRLF
           "    <status>" CRLF
           "      <basic>open</basic>" CRLF
           "    </status>" CRLF
           "    <dm:deviceID>urn:x-mac:0003ba4811e3</dm:deviceID>" CRLF
           "    <rpid:class>email</rpid:class>" CRLF
           "    <rpid:service-class><rpid:electronic/></rpid:service-class>" CRLF
           "    <rpid:status-icon>http://example.com/mail.png</rpid:status-icon>" CRLF
           "    <contact priority=\"1.0\">mailto:someone@example.com</contact>" CRLF
           "  </tuple>" CRLF
           CRLF
           "  <note>I'll be in Tokyo next week</note>" CRLF
           CRLF
           "  <dm:device id=\"pc147\">" CRLF
           "    <rpid:user-input idle-threshold=\"600\" last-input=\"2004-10-21T13:20:00-05:00\">idle</rpid:user-input>" CRLF
           "    <dm:deviceID>urn:device:0003ba4811e3</dm:deviceID>" CRLF
           "    <dm:note>PC</dm:note>" CRLF
           "  </dm:device>" CRLF
           CRLF
           "  <dm:person id=\"p1\">" CRLF
           "    <rpid:activities from=\"2005-05-30T12:00:00+05:00\" until=\"2005-05-30T17:00:00+05:00\">" CRLF
           "      <rpid:note>Far away</rpid:note>" CRLF
           "      <rpid:away/>" CRLF
           "    </rpid:activities>" CRLF
           "    <rpid:class>calendar</rpid:class>" CRLF
           "    <rpid:mood>" CRLF
           "      <rpid:angry/>" CRLF
           "      <rpid:other>brooding</rpid:other>" CRLF
           "    </rpid:mood>" CRLF
           "    <rpid:place-is>" CRLF
           "      <rpid:audio>" CRLF
           "        <rpid:noisy/>" CRLF
           "      </rpid:audio>" CRLF
           "    </rpid:place-is>" CRLF
           "    <rpid:place-type><lt:residence/></rpid:place-type>" CRLF
           "    <rpid:privacy><rpid:unknown/></rpid:privacy>" CRLF
           "    <rpid:sphere>bowling league</rpid:sphere>" CRLF
           "    <rpid:status-icon>http://example.com/play.gif</rpid:status-icon>" CRLF
           "    <rpid:time-offset>-240</rpid:time-offset>" CRLF
           "    <dm:note>Scoring 120</dm:note>" CRLF
           "    <dm:timestamp>2005-05-30T16:09:44+05:00</dm:timestamp>" CRLF
           "  </dm:person>" CRLF
           "</presence>" CRLF
           );
       HeaderFieldValue hfv(txt.data(), txt.size());
       GenericPidfContents pidf(hfv, GenericPidfContents::getStaticType());
       assert(pidf.getEntity() == Uri("pres:someone@example.com"));
       assert(pidf.getNamespaces().size() == 4);
       assert(pidf.getRootPidfNamespacePrefix().empty());
       assert(pidf.getRootNodes().size() == 6);
       assert(pidf.getRootNodes().front()->mTag == "tuple");
       assert(pidf.getRootNodes().front()->mChildren.size() == 8);
       assert(pidf.getRootNodes().front()->mChildren.front()->mTag == "status");
       assert(pidf.getRootNodes().front()->mChildren.back()->mTag == "timestamp");
       assert(pidf.getRootNodes().front()->mChildren.back()->mValue == "2005-10-27T16:49:29Z");
       assert(pidf.getRootNodes().back()->mTag == "person");
       assert(pidf.getRootNodes().back()->mChildren.size() == 11);
       assert(pidf.getRootNodes().back()->mChildren.front()->mTag == "activities");
       assert(pidf.getRootNodes().back()->mChildren.front()->mAttributes.size() == 2);
       assert(pidf.getRootNodes().back()->mChildren.front()->mAttributes["from"] == "2005-05-30T12:00:00+05:00");
       assert(pidf.getRootNodes().back()->mChildren.front()->mAttributes["until"] == "2005-05-30T17:00:00+05:00");
       assert(pidf.getRootNodes().back()->mChildren.back()->mTag == "timestamp");
       assert(pidf.getRootNodes().back()->mChildren.back()->mValue == "2005-05-30T16:09:44+05:00");
       assert(pidf.getSimplePresenceTupleId() == "bs35r9");
       assert(pidf.getSimplePresenceOnline() == true);
       assert(pidf.getSimplePresenceTimestamp() == "2005-10-27T16:49:29Z");
       assert(pidf.getSimplePresenceNote() == "Don't Disturb Please!");
       assert(pidf.getSimplePresenceContact() == "im:someone@mobile.example.net");
       assert(pidf.getSimplePresenceContactPriority() == "0.8");
       cout << pidf << endl;
   }

   // Merge test
   {
      const Data txt1(
         "<?xml version='1.0' encoding='UTF-8'?>" CRLF
         "<presence xmlns='urn:ietf:params:xml:ns:pidf'" CRLF
         "          xmlns:dm='urn:ietf:params:xml:ns:pidf:data-model'" CRLF
         "          xmlns:rpid='urn:ietf:params:xml:ns:pidf:rpid'" CRLF
         "          xmlns:c='urn:ietf:params:xml:ns:pidf:cipid'" CRLF
         "        entity='sip:5000@blitzzgod.com'>" CRLF
         "  <tuple id='ta054033a'>" CRLF
         "    <status>" CRLF
         "      <basic>open</basic>" CRLF
         "    </status>" CRLF
         "  </tuple>" CRLF
         "  <dm:person id='p23008e26'>" CRLF
         "    <rpid:activities>" CRLF
         "      <rpid:unknown/>" CRLF
         "    </rpid:activities>" CRLF
         "  </dm:person>" CRLF
         "</presence>" CRLF
         );
      Data txt2(
         "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" CRLF
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" " CRLF
         "          xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" " CRLF
         "          xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" " CRLF
         "        entity=\"pres:5000@blitzzgod.com\">" CRLF
         "  <dm:person id=\"p1\">" CRLF
         "    <rpid:activities/>" CRLF
         "    <dm:timestamp>2005-05-30T16:12:44+05:00</dm:timestamp>" CRLF
         "  </dm:person>" CRLF
         "  <tuple id=\"t8642\">" CRLF
         "    <status>" CRLF
         "      <basic>open</basic>" CRLF
         "    </status>" CRLF
         "    <contact>sip:2002@blitzzgod.com</contact>" CRLF
         "    <note>Online</note>" CRLF
         "  </tuple>" CRLF
         "</presence>" CRLF
         );
      Data txt3(
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
         "<impp:presence xmlns:impp=\"urn:ietf:params:xml:ns:pidf\"" CRLF
         "      entity=\"pres:5000@blitzzgod.com\">" CRLF
         "  <impp:tuple id=\"mobile-im\">" CRLF
         "    <impp:status>" CRLF
         "      <impp:basic>open</impp:basic>" CRLF
         "    </impp:status>" CRLF
         "    <impp:contact priority=\"0.8\">tel:+09012345678</impp:contact>" CRLF
         "  </impp:tuple>" CRLF
         "</impp:presence>" CRLF
         );
      Data txt4(
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" CRLF
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"" CRLF
         "          xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\"" CRLF
         "          xmlns:lt=\"urn:ietf:params:xml:ns:location-type\"" CRLF
         "          xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\"" CRLF
         "       entity=\"pres:5000@blitzzgod.com\">" CRLF
         CRLF
         "  <tuple id=\"bs35r9\">" CRLF
         "    <status>" CRLF
         "      <basic>open</basic>" CRLF
         "    </status>" CRLF
         "    <dm:deviceID>urn:device:0003ba4811e3</dm:deviceID>" CRLF
         "    <rpid:relationship><rpid:self/></rpid:relationship>" CRLF
         "    <rpid:service-class><rpid:electronic/></rpid:service-class>" CRLF
         "    <contact priority=\"0.8\">im:someone@mobile.example.net</contact>" CRLF
         "    <note xml:lang=\"en\">Don't Disturb Please!</note>" CRLF
         "    <note xml:lang=\"fr\">Ne derangez pas, s'il vous plait</note>" CRLF
         "    <timestamp>2005-10-27T16:49:29Z</timestamp>" CRLF
         "  </tuple>" CRLF
         CRLF
         "  <tuple id=\"ta054033a\">" CRLF  // Note:  This will not merge over tuple id="ta054033a" from first pidf
         "    <status>" CRLF
         "      <basic>open</basic>" CRLF
         "    </status>" CRLF
         "    <rpid:relationship><rpid:assistant/></rpid:relationship>" CRLF
         "    <contact priority=\"1.0\">mailto:secretary@example.com</contact>" CRLF
         "  </tuple>" CRLF
         CRLF
         "  <tuple id=\"eg92n8\">" CRLF
         "    <status>" CRLF
         "      <basic>open</basic>" CRLF
         "    </status>" CRLF
         "    <dm:deviceID>urn:x-mac:0003ba4811e3</dm:deviceID>" CRLF
         "    <rpid:class>email</rpid:class>" CRLF
         "    <rpid:service-class><rpid:electronic/></rpid:service-class>" CRLF
         "    <rpid:status-icon>http://example.com/mail.png</rpid:status-icon>" CRLF
         "    <contact priority=\"1.0\">mailto:someone@example.com</contact>" CRLF
         "  </tuple>" CRLF
         CRLF
         "  <note>I'll be in Tokyo next week</note>" CRLF
         CRLF
         "  <dm:device id=\"pc147\">" CRLF
         "    <rpid:user-input idle-threshold=\"600\" last-input=\"2004-10-21T13:20:00-05:00\">idle</rpid:user-input>" CRLF
         "    <dm:deviceID>urn:device:0003ba4811e3</dm:deviceID>" CRLF
         "    <dm:note>PC</dm:note>" CRLF
         "  </dm:device>" CRLF
         CRLF
         "  <dm:person id=\"p1\">" CRLF   // Note:  This will not merge since person with id="p1" exists in other pidf and it's timestamp is newer
         "    <rpid:activities from=\"2005-05-30T12:00:00+05:00\" until=\"2005-05-30T17:00:00+05:00\">" CRLF
         "      <rpid:note>Far away</rpid:note>" CRLF
         "      <rpid:away/>" CRLF
         "    </rpid:activities>" CRLF
         "    <rpid:class>calendar</rpid:class>" CRLF
         "    <rpid:mood>" CRLF
         "      <rpid:angry/>" CRLF
         "      <rpid:other>brooding</rpid:other>" CRLF
         "    </rpid:mood>" CRLF
         "    <rpid:place-is>" CRLF
         "      <rpid:audio>" CRLF
         "        <rpid:noisy/>" CRLF
         "      </rpid:audio>" CRLF
         "    </rpid:place-is>" CRLF
         "    <rpid:place-type><lt:residence/></rpid:place-type>" CRLF
         "    <rpid:privacy><rpid:unknown/></rpid:privacy>" CRLF
         "    <rpid:sphere>bowling league</rpid:sphere>" CRLF
         "    <rpid:status-icon>http://example.com/play.gif</rpid:status-icon>" CRLF
         "    <rpid:time-offset>-240</rpid:time-offset>" CRLF
         "    <dm:note>Scoring 120</dm:note>" CRLF
         "    <dm:timestamp>2005-05-30T16:09:44+05:00</dm:timestamp>" CRLF
         "  </dm:person>" CRLF
         "</presence>" CRLF
         );

      try
      {
         HeaderFieldValue hfv1(txt1.data(), txt1.size());
         HeaderFieldValue hfv2(txt2.data(), txt2.size());
         HeaderFieldValue hfv3(txt3.data(), txt3.size());
         HeaderFieldValue hfv4(txt4.data(), txt4.size());
         Mime type("application", "pidf+xml");
         GenericPidfContents pidf1(hfv1, type);
         GenericPidfContents pidf2(hfv2, type);
         GenericPidfContents pidf3(hfv3, type);
         GenericPidfContents pidf4(hfv4, type);
         GenericPidfContents merged;
         merged = pidf1;
         merged.merge(pidf2);
         merged.merge(pidf3);
         merged.merge(pidf4);

         pidf1.checkParsed();
         assert(pidf1.getEntity() == Uri("sip:5000@blitzzgod.com"));
         assert(pidf1.getNamespaces().size() == 4);
         assert(pidf1.getRootPidfNamespacePrefix().empty());
         assert(pidf1.getRootNodes().size() == 2);

         pidf2.checkParsed();
         assert(pidf2.getEntity() == Uri("pres:5000@blitzzgod.com"));
         assert(pidf2.getNamespaces().size() == 3);
         assert(pidf2.getRootPidfNamespacePrefix().empty());
         assert(pidf2.getRootNodes().size() == 2);

         pidf3.checkParsed();
         assert(pidf3.getEntity() == Uri("pres:5000@blitzzgod.com"));
         assert(pidf3.getNamespaces().size() == 1);
         assert(pidf3.getRootPidfNamespacePrefix() == "impp:");
         assert(pidf3.getRootNodes().size() == 1);

         pidf4.checkParsed();
         assert(pidf4.getEntity() == Uri("pres:5000@blitzzgod.com"));
         assert(pidf4.getNamespaces().size() == 4);
         assert(pidf4.getRootPidfNamespacePrefix().empty());
         assert(pidf4.getRootNodes().size() == 6);

         cout << merged << endl;
         assert(merged.getEntity() == Uri("sip:5000@blitzzgod.com"));
         assert(merged.getNamespaces().size() == 5);
         assert(merged.getRootPidfNamespacePrefix().empty());
         assert(merged.getRootNodes().size() == 9);
         assert(merged.getRootNodes().front()->mTag == "tuple");
         assert(merged.getRootNodes().front()->mAttributes.size() == 1);
         assert(merged.getRootNodes().front()->mAttributes["id"] == "ta054033a");
         assert(merged.getRootNodes().front()->mChildren.size() == 3);
         assert(merged.getRootNodes().front()->mChildren.front()->mTag == "status");
         assert(merged.getRootNodes().front()->mChildren.front()->mChildren.size() == 1);
         assert(merged.getRootNodes().front()->mChildren.front()->mChildren.front()->mTag == "basic");
         assert(merged.getRootNodes().front()->mChildren.front()->mChildren.front()->mValue == "open");
         assert(merged.getRootNodes().front()->mChildren.back()->mTag == "contact");
         assert(merged.getRootNodes().front()->mChildren.back()->mValue == "mailto:secretary@example.com");
         assert(merged.getRootNodes().back()->mTag == "device");
         assert(merged.getRootNodes().back()->mChildren.size() == 3);
         assert(merged.getRootNodes().back()->mChildren.front()->mTag == "user-input");
         assert(merged.getRootNodes().back()->mChildren.front()->mAttributes.size() == 2);
         assert(merged.getRootNodes().back()->mChildren.front()->mAttributes["idle-threshold"] == "600");
         assert(merged.getRootNodes().back()->mChildren.front()->mAttributes["last-input"] == "2004-10-21T13:20:00-05:00");
         assert(merged.getRootNodes().back()->mChildren.front()->mValue == "idle");
         assert(merged.getRootNodes().back()->mChildren.back()->mTag == "note");
         assert(merged.getRootNodes().back()->mChildren.back()->mValue == "PC");
         assert(merged.getSimplePresenceTupleId() == "ta054033a");
         assert(merged.getSimplePresenceOnline() == true);
         assert(merged.getSimplePresenceTimestamp() == Data::Empty);
         assert(merged.getSimplePresenceNote() == Data::Empty);
         assert(merged.getSimplePresenceContact() == "mailto:secretary@example.com");
         assert(merged.getSimplePresenceContactPriority() == "1.0");
      }
      catch (BaseException& e)
      {
         resipCerr << "Caught: " << e << endl;
      }
   }

   cerr << "All OK" << endl;
   return 0;
}

/* ====================================================================
*
* Copyright (c) 2015 SIP Spectrum, Inc.  All rights reserved.
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
* 3. Neither the name of the author(s) nor the names of any contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* ====================================================================
*
*/
/*
* vi: set shiftwidth=3 expandtab:
*/