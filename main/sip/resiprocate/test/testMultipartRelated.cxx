#include <iostream>
#include <memory>

#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/Rlmi.hxx"
#include "resiprocate/Pidf.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/ParseBuffer.hxx"

using namespace resip;
using namespace std;

int
main()
{
   {
      const Data txt("NOTIFY sip:terminal.example.com SIP/2.0\r\n"
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
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      MultipartRelatedContents* mpc = dynamic_cast<MultipartRelatedContents*>(msg->getContents());
      assert(mpc);
      assert(mpc->parts().size() == 3);

      for (MultipartRelatedContents::Parts::const_iterator i = mpc->parts().begin();
           i != mpc->parts().end(); ++i)
      {
         if (dynamic_cast<Pidf*>(*i))
         {
            cerr << "discovered a Pidf" << endl;
         } 
         else if (dynamic_cast<Rlmi*>(*i))
         {
            cerr << "discovered a Rlmi" << endl;
         }
         else if (dynamic_cast<MultipartRelatedContents*>(*i))
         {
            cerr << "discovered a multipart (recurse on your own!)" << endl;
         }
         else 
         {
            cerr << "Some other kind of content!" << endl;
         }
      }
   }

   return 0;
}
