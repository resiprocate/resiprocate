#include <sipstack/SipMessage.hxx>
#include <sipstack/Preparse.hxx>
#include <sipstack/Uri.hxx>

#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      cerr << "Testing raw header transfer" << endl;
      
      char *txt1 = "REGISTER sip:registrar.biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\nMax-Forwards: 70\r\nTo: Bob <sip:bob@biloxi.com>\r\nFrom: Bob <sip:bob@biloxi.com>;tag=456248\r\nCall-ID: 843817637684230@998sdasdh09\r\nCSeq: 1826 REGISTER\r\nContact: <sip:bob@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      char *txt2 = "REGISTER sip:registrar.ixolib.com SIP/2.0\r\nVia: SIP/2.0/UDP qoqspc.ixolib.com:5060;branch=2222222222\r\nMax-Forwards: 70\r\nTo: Qoq <sip:qoq@ixolib.com>\r\nFrom: Qoq <sip:qoq@ixolib.com>;tag=456248\r\nCall-ID: 111111111111111\r\nCSeq: 6281 REGISTER\r\nContact: <sip:qoq@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      SipMessage message1;
      SipMessage message2;
      
      Preparse parse1(message1, txt1, strlen(txt1));
      parse1.process();

      Preparse parse2(message2, txt2, strlen(txt2));
      parse2.process();

      assert(message1.getRawHeader(Headers::CSeq)->getParserContainer() == 0);
      assert(message2.getRawHeader(Headers::CSeq)->getParserContainer() == 0);

      message1.setRawHeader(message2.getRawHeader(Headers::CSeq), Headers::CSeq);
      message1.encode(cerr) << endl;
      assert(message1.header(h_CSeq).sequence() == 6281);
   }

   {
      cerr << "Testing raw header transfer post parse" << endl;
      
      char *txt1 = "REGISTER sip:registrar.biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\nMax-Forwards: 70\r\nTo: Bob <sip:bob@biloxi.com>\r\nFrom: Bob <sip:bob@biloxi.com>;tag=456248\r\nCall-ID: 843817637684230@998sdasdh09\r\nCSeq: 1826 REGISTER\r\nContact: <sip:bob@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      char *txt2 = "REGISTER sip:registrar.ixolib.com SIP/2.0\r\nVia: SIP/2.0/UDP qoqspc.ixolib.com:5060;branch=2222222222\r\nMax-Forwards: 70\r\nTo: Qoq <sip:qoq@ixolib.com>\r\nFrom: Qoq <sip:qoq@ixolib.com>;tag=456248\r\nCall-ID: 111111111111111\r\nCSeq: 6281 REGISTER\r\nContact: <sip:qoq@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";

      SipMessage message1;
      SipMessage message2;
      
      Preparse parse1(message1, txt1, strlen(txt1));
      parse1.process();

      Preparse parse2(message2, txt2, strlen(txt2));
      parse2.process();

      assert(message1.getRawHeader(Headers::CSeq)->getParserContainer() == 0);
      assert(message2.getRawHeader(Headers::CSeq)->getParserContainer() == 0);

      // causes parse
      assert(message2.header(h_CSeq).sequence() == 6281);
      // should have a parsed header
      assert(message2.getRawHeader(Headers::CSeq)->getParserContainer());
      
      // should still work, but copies
      message1.setRawHeader(message2.getRawHeader(Headers::CSeq), Headers::CSeq);
      message1.encode(cerr) << endl;
      assert(message1.header(h_CSeq).sequence() == 6281);

      // should have a parsed header
      assert(message1.getRawHeader(Headers::CSeq)->getParserContainer());
   }
}
