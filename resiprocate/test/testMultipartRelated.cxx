#include <iostream>
#include <memory>

#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/Rlmi.hxx"
#include "resiprocate/Pidf.hxx"
#include "resiprocate/Pkcs7Contents.hxx"
#include "resiprocate/MultipartSignedContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/ParseBuffer.hxx"

using namespace resip;
using namespace std;

void indent(int indent)
{
   for (int i = 0; i < 3*indent; ++i)
   {
      cerr << " ";
   }
}

void
traverseMulti(const MultipartMixedContents* mp,
              int level = 0)
{
      for (MultipartRelatedContents::Parts::const_iterator i = mp->parts().begin();
           i != mp->parts().end(); ++i)
      {
         Pidf* pidf;
         Rlmi* rlmi;
         MultipartSignedContents* mps;
         MultipartMixedContents* mpm;
         Pkcs7Contents* pkcs7;

         if ((pidf = dynamic_cast<Pidf*>(*i)))
         {
            indent(level);
            cerr << "discovered a Pidf" << endl;
         } 
         else if ((rlmi = dynamic_cast<Rlmi*>(*i)))
         {
            indent(level);
            cerr << "discovered a Rlmi" << endl;
         }
         else if ((pkcs7 = dynamic_cast<Pkcs7Contents*>(*i)))
         {
            indent(level);
            cerr << "discovered a Pkcs7Contents" << endl;
         }
         else if ((mpm = dynamic_cast<MultipartMixedContents*>(*i)))
         {
            indent(level);
            cerr << "discovered a multipart with " << mpm->parts().size() << " parts " << endl;
            traverseMulti(mpm, level+1);
         }
         else 
         {
            indent(level);
            cerr << "Some other kind of content!" << endl;
         }
      }
}

// http://www.softarmor.com/simple/drafts/draft-ietf-simple-event-list-04.txt
int
main()
{
   {
      const Data tx0("NOTIFY sip:terminal.example.com SIP/2.0\r\n"
                     "Via: SIP/2.0/TCP pres.example.com;branch=z9hG4bKMgRenTETmm\r\n"
                     "Max-Forwards: 70\r\n"
                     "From: <sip:adam-buddies@pres.example.com>;tag=zpNctbZq\r\n"
                     "To: <sip:adam@example.com>;tag=ie4hbb8t\r\n"
                     "Call-ID: cdB34qLToC@terminal.example.com\r\n"
                     "CSeq: 997935768 NOTIFY\r\n"
                     "Contact: <sip:pres.example.com>\r\n"
                     "Event: presence\r\n"
                     "Subscription-State: active;expires=7200\r\n"
                     "Require: eventlist\r\n"
                     "Content-Type: multipart/related;type=\"application/rlmi+xml\";\r\n"
                     "    start=\"<nXYxAE@pres.example.com>\";boundary=\"50UBfW7LSCVLtggUPe5z\"\r\n"
                     "Content-Length: 1560\r\n"
                     "\r\n"
                     "--50UBfW7LSCVLtggUPe5z\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <nXYxAE@pres.example.com>\r\n"
                     "Content-Type: application/rlmi+xml;charset=\"UTF-8\"\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<list xmlns=\"urn:ietf:params:xml:ns:rmli\"\r\n"
                     "      uri=\"sip:adam-friends@pres.example.com\" version=\"1\"\r\n"
                     "      name=\"Buddy List at COM\" fullState=\"true\">\r\n"
                     "  <resource uri=\"sip:bob@example.com\" name=\"Bob Smith\">\r\n"
                     "    <instance id=\"juwigmtboe\" state=\"active\"\r\n"
                     "              cid=\"bUZBsM@pres.example.com\"/>\r\n"
                     "  </resource>\r\n"
                     "  <resource uri=\"sip:dave@example.com\" name=\"Dave Jones\">\r\n"
                     "    <instance id=\"hqzsuxtfyq\" state=\"active\"\r\n"
                     "              cid=\"ZvSvkz@pres.example.com\"/>\r\n"
                     "  </resource>\r\n"
                     "  <resource uri=\"sip:ed@example.net\" name=\"Ed at NET\" />\r\n"
                     "  <resource uri=\"sip:adam-friends@example.org\"\r\n"
                     "            name=\"My Friends at ORG\" />\r\n"
                     "</list>\r\n"
                     "\r\n"
                     "--50UBfW7LSCVLtggUPe5z\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <bUZBsM@pres.example.com>\r\n"
                     "Content-Type: application/cpim-pidf+xml;charset=\"UTF-8\"\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<presence xmlns=\"urn:ietf:params:xml:ns:cpim-pidf\"\r\n"
                     "    entity=\"sip:bob@example.com\">\r\n"
                     "  <tuple id=\"sg89ae\">\r\n"
                     "    <status>\r\n"
                     "      <basic>open</basic>\r\n"
                     "    </status>\r\n"
                     "    <contact priority=\"1.0\">sip:bob@example.com</contact>\r\n"
                     "  </tuple>\r\n"
                     "</presence>\r\n"
                     "\r\n"
                     "--50UBfW7LSCVLtggUPe5z\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <ZvSvkz@pres.example.com>\r\n"
                     "Content-Type: application/cpim-pidf+xml;charset=\"UTF-8\"\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<presence xmlns=\"urn:ietf:params:xml:ns:cpim-pidf\"\r\n"
                     "    entity=\"sip:dave@example.com\">\r\n"
                     "  <tuple id=\"slie74\">\r\n"
                     "    <status>\r\n"
                     "      <basic>closed</basic>\r\n"
                     "    </status>\r\n"
                     "  </tuple>\r\n"
                     "</presence>\r\n"
                     "\r\n"
                     "--50UBfW7LSCVLtggUPe5z--\r\n");

      const Data txt("NOTIFY sip:terminal.example.com SIP/2.0\r\n"
                     "Via: SIP/2.0/TCP pres.example.com;branch=z9hG4bK4EPlfSFQK1\r\n"
                     "Max-Forwards: 70\r\n"
                     "From: <sip:adam-buddies@pres.example.com>;tag=zpNctbZq\r\n"
                     "To: <sip:adam@example.com>;tag=ie4hbb8t\r\n"
                     "Call-ID: cdB34qLToC@terminal.example.com\r\n"
                     "CSeq: 997935769 NOTIFY\r\n"
                     "Contact: <sip:pres.example.com>\r\n"
                     "Event: presence\r\n"
                     "Subscription-State: active;expires=7200\r\n"
                     "Require: eventlist\r\n"
                     "Content-Type: multipart/related;type=\"application/rlmi+xml\";\r\n"
                     "    start=\"<2BEI83@pres.example.com>\";boundary=\"TfZxoxgAvLqgj4wRWPDL\"\r\n"
                     "Content-Length: 2862\r\n"
                     "\r\n"
                     "--TfZxoxgAvLqgj4wRWPDL\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <2BEI83@pres.example.com>\r\n"
                     "Content-Type: application/rlmi+xml;charset=\"UTF-8\"\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<list xmlns=\"urn:ietf:params:xml:ns:rmli\"\r\n"
                     "      uri=\"sip:adam-friends@pres.example.com\" version=\"2\"\r\n"
                     "      name=\"Buddy List at COM\" fullState=\"false\">\r\n"
                     "  <resource uri=\"sip:ed@example.net\" name=\"Ed at NET\">\r\n"
                     "    <instance id=\"sdlkmeopdf\" state=\"pending\"/>\r\n"
                     "  </resource>\r\n"
                     "  <resource uri=\"sip:adam-friends@example.org\"\r\n"
                     "            name=\"My Friends at ORG\">\r\n"
                     "    <instance id=\"cmpqweitlp\" state=\"active\"\r\n"
                     "              cid=\"1KQhyE@pres.example.com\"/>\r\n"
                     "  </resource>\r\n"
                     "</list>\r\n"
                     "\r\n"
                     "--TfZxoxgAvLqgj4wRWPDL\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <1KQhyE@pres.example.com>\r\n"
                     "Content-Type: multipart/signed;\r\n"
                     "    protocol=\"application/pkcs7-signature\";\r\n"
                     "    micalg=sha1;boundary=\"l3WMZaaL8NpQWGnQ4mlU\"\r\n"
                     "\r\n"
                     "--l3WMZaaL8NpQWGnQ4mlU\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <ZPvJHL@example.org>\r\n"
                     "Content-Type: multipart/related;type=\"application/rlmi+xml\";\r\n"
                     "    start=\"<Cvjpeo@example.org>\";boundary=\"tuLLl3lDyPZX0GMr2YOo\"\r\n"
                     "\r\n"
                     "--tuLLl3lDyPZX0GMr2YOo\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <Cvjpeo@example.org>\r\n"
                     "Content-Type: application/rlmi+xml;charset=\"UTF-8\"\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<list xmlns=\"urn:ietf:params:xml:ns:rmli\"\r\n"
                     "      uri=\"sip:adam-friends@example.org\" version=\"1\"\r\n"
                     "      name=\"Buddy List at ORG\" fullState=\"true\">\r\n"
                     "  <resource uri=\"sip:joe@example.org\" name=\"Joe Thomas\">\r\n"
                     "    <instance id=\"1\" state=\"active\" cid=\"mrEakg@example.org\"/>\r\n"
                     "  </resource>\r\n"
                     "  <resource uri=\"sip:mark@example.org\" name=\"Mark Edwards\">\r\n"
                     "    <instance id=\"1\" state=\"active\" cid=\"KKMDmv@example.org\"/>\r\n"
                     "  </resource>\r\n"
                     "</list>\r\n"
                     "\r\n"
                     "--tuLLl3lDyPZX0GMr2YOo\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <mrEakg@example.org>\r\n"
                     "Content-Type: application/cpim-pidf+xml;charset=\"UTF-8\"\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<presence xmlns=\"urn:ietf:params:xml:ns:cpim-pidf\"\r\n"
                     "    entity=\"sip:joe@example.org\">\r\n"
                     "  <tuple id=\"7823a4\">\r\n"
                     "    <status>\r\n"
                     "      <basic>open</basic>\r\n"
                     "    </status>\r\n"
                     "    <contact priority=\"1.0\">sip:joe@example.org</contact>\r\n"
                     "  </tuple>\r\n"
                     "</presence>\r\n"
                     "\r\n"
                     "--tuLLl3lDyPZX0GMr2YOo\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <KKMDmv@example.org>\r\n"
                     "Content-Type: application/cpim-pidf+xml;charset=\"UTF-8\"\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<presence xmlns=\"urn:ietf:params:xml:ns:cpim-pidf\"\r\n"
                     "    entity=\"sip:mark@example.org\">\r\n"
                     "  <tuple id=\"398075\">\r\n"
                     "    <status>\r\n"
                     "      <basic>closed</basic>\r\n"
                     "    </status>\r\n"
                     "  </tuple>\r\n"
                     "</presence>\r\n"
                     "\r\n"
                     "--tuLLl3lDyPZX0GMr2YOo--\r\n"
                     "\r\n"
                     "--l3WMZaaL8NpQWGnQ4mlU\r\n"
                     "Content-Transfer-Encoding: binary\r\n"
                     "Content-ID: <K9LB7k@example.org>\r\n"
                     "Content-Type: application/pkcs7-signature\r\n"
                     "\r\n"
                     "[PKCS #7 signature here]\r\n"
                     "\r\n"
                     "--l3WMZaaL8NpQWGnQ4mlU--\r\n"
                     "\r\n"
                     "--TfZxoxgAvLqgj4wRWPDL--\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      MultipartRelatedContents* mpc = dynamic_cast<MultipartRelatedContents*>(msg->getContents());
      assert(mpc);
      
      traverseMulti(mpc);
   }

   return 0;
}
