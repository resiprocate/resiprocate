#include "sip2/sipstack/MultipartMixedContents.hxx"
#include "sip2/sipstack/PlainContents.hxx"
#include "sip2/util/ParseBuffer.hxx"
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      const Data txt("Content-Type: Multipart/Related; boundary=example-1;"
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

      // "Content-Type: "
      const Data contentsTxt = ("Multipart/Related; boundary=example-1\r\n"
                                "        start=\"<950120.aaCC@XIson.com>\";\r\n"
                                "        type=\"Application/X-FixedRecord\"\r\n"
                                "     start-info=\"-o ps\"\r\n");

      ParseBuffer pb(contentsTxt.data(), contentsTxt.size());
      Mime contentType;
      contentType.parse(pb);

      HeaderFieldValue hfv(txt.data(), txt.size());
      MultipartMixedContents mpc(&hfv, contentType);

      assert(mpc.parts().size() == 2);

      PlainContents *f = dynamic_cast<PlainContents*>(mpc.parts().front());

      mpc.encode(cerr);
   }

   cerr << "All OK" << endl;
}

