#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/PlainContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      const Data txt("--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-ID: <1_950120.aaCC@XIson.com>\r\n"
                     "\r\n"
                     "25\r\n"
                     "10\r\n"
                     "34\r\n"
                     "10\r\n"
                     "25\r\n"
                     "21\r\n"
                     "26\r\n"
                     "10\r\n"
                     "--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Description: The fixed length records\r\n"
                     "Content-Transfer-Encoding: base64\r\n"
                     "Content-ID: <2_950120.aaCB@XIson.com>\r\n"
                     "\r\n"
                     "T2xkIE1hY0RvbmFsZCBoYWQgYSBmYXJtCkUgSS\r\n"
                     "BFIEkgTwpBbmQgb24gaGlzIGZhcm0gaGUgaGFk\r\n"
                     "IHNvbWUgZHVja3MKRSBJIEUgSSBPCldpdGggYS\r\n"
                     "BxdWFjayBxdWFjayBoZXJlLAphIHF1YWNrIHF1\r\n"
                     "YWNrIHRoZXJlLApldmVyeSB3aGVyZSBhIHF1YW\r\n"
                     "NrIHF1YWNrCkUgSSBFIEkgTwo=\r\n"
                     "\r\n"
                     "--example-1--");

      // "Content-Type: "
      const Data contentsTxt = ("Multipart/Related; boundary=example-1\r\n"
                                "        start=\"<950120.aaCC@XIson.com>\";\r\n"
                                "        type=\"Application/X-FixedRecord\"\r\n"
                                "     start-info=\"-o ps\"\r\n");

      ParseBuffer pb(contentsTxt.data(), contentsTxt.size());
      Mime contentType;
      contentType.parse(pb);

      HeaderFieldValue hfv(txt.data(), txt.size());
      MultipartRelatedContents mpc(&hfv, contentType);

      assert(mpc.parts().size() == 2);

      PlainContents *f = dynamic_cast<PlainContents*>(mpc.parts().front());
      assert(f);
      f->getBodyData();

      mpc.encode(cerr);
   }

   {
      const Data txt("INVITE sip:bob@biloxi.com SIP/2.0\r\n"
                     "To: <sip:bob@biloxi.com>\r\n"
                     "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                     "Call-ID: 314159\r\n"
                     "CSeq: 14 INVITE\r\n"
                     "Content-Type: Multipart/Related; boundary=example-1;"
                     "start=\"<950120.aaCC@XIson.com>\";"
                     "type=\"Application/X-FixedRecord\";start-info=\"-o ps\"\r\n"
                     "\r\n"
                     "--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-ID: <1_950120.aaCC@XIson.com>\r\n"
                     "\r\n"
                     "25\r\n"
                     "10\r\n"
                     "34\r\n"
                     "10\r\n"
                     "25\r\n"
                     "21\r\n"
                     "26\r\n"
                     "10\r\n"
                     "--example-1\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Description: The fixed length records\r\n"
                     "Content-Transfer-Encoding: base64\r\n"
                     "Content-ID: <2_950120.aaCB@XIson.com>\r\n"
                     "\r\n"
                     "T2xkIE1hY0RvbmFsZCBoYWQgYSBmYXJtCkUgSS\r\n"
                     "BFIEkgTwpBbmQgb24gaGlzIGZhcm0gaGUgaGFk\r\n"
                     "IHNvbWUgZHVja3MKRSBJIEUgSSBPCldpdGggYS\r\n"
                     "BxdWFjayBxdWFjayBoZXJlLAphIHF1YWNrIHF1\r\n"
                     "YWNrIHRoZXJlLApldmVyeSB3aGVyZSBhIHF1YW\r\n"
                     "NrIHF1YWNrCkUgSSBFIEkgTwo=\r\n"
                     "\r\n"
                     "--example-1--");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      MultipartRelatedContents* mpc = dynamic_cast<MultipartRelatedContents*>(msg->getContents());
      assert(mpc);
      assert(mpc->parts().size() == 2);
      PlainContents *f11 = dynamic_cast<PlainContents*>(mpc->parts().front());
      assert(f11);
      f11->getBodyData();
      MultipartRelatedContents::Parts::const_iterator i = ++mpc->parts().begin();
      PlainContents *f12 = dynamic_cast<PlainContents*>(*i);
      assert(f12);
      f12->getBodyData();

      Data buff;
      {
         DataStream str(buff);
         str << *msg;
      }

      {      
         auto_ptr<SipMessage> msg1(TestSupport::makeMessage(buff.c_str()));
         MultipartRelatedContents* mpc = dynamic_cast<MultipartRelatedContents*>(msg->getContents());
         assert(mpc);
         assert(mpc->parts().size() == 2);
         PlainContents *f21 = dynamic_cast<PlainContents*>(mpc->parts().front());
         assert(f21);
         f21->getBodyData();
         MultipartRelatedContents::Parts::const_iterator i = ++mpc->parts().begin();
         PlainContents *f22 = dynamic_cast<PlainContents*>(*i);
         assert(f22);
         f22->getBodyData();

         assert(f11->getBodyData() == f21->getBodyData());
         assert(f12->getBodyData() == f22->getBodyData());
      }
   }

   cerr << "All OK" << endl;
}

