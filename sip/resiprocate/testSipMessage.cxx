#include <sipstack/SipMessage.hxx>
#include <sipstack/Preparse.hxx>
#include <sipstack/Uri.hxx>

#include <iostream>
#include <sstream>

using namespace Vocal2;
using namespace std;

int
main()
{
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
      SipMessage message;
      
      Preparse parse(message, txt, strlen(txt));
      parse.process();
      
      cerr << "Encode from unparsed: " << endl;
      message.encode(cerr);

      message.header(h_To).uri().user();
      message.header(h_From).uri().user();
      message.header(h_MaxForwards).value();
      message.header(h_Contacts).empty();
      message.header(h_CallId).value();
      message.header(h_CSeq).sequence();
      message.header(h_Vias).empty();
      message.header(h_Expires).value();

      cerr << "Encode from parsed: " << endl;
      message.encode(cerr);

      message.header(h_Contacts).front().uri().user() = "jason";

      cerr << "Encode after messing: " << endl;
      message.encode(cerr);
   }
   
   return 0;
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
      SipMessage message;
      
      Preparse parse(message, txt, strlen(txt));
      parse.process();
      
      message.encode(cerr);
      
      Data v = message.header(h_CallId).value();
      assert(v == "843817637684230@998sdasdh09");
      //StatusLine& foo = message.header(h_StatusLine);
      //RequestLine& bar = message.header(h_RequestLine);
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
      SipMessage message;
      
      Preparse parse(message, txt, strlen(txt));
      parse.process();
      
          
      Data v = message.header(h_CallId).value();
      cerr << "Call-ID is " << v << endl;

      message.encode(cerr);
  
      //StatusLine& foo = message.header(h_StatusLine);
      //RequestLine& bar = message.header(h_RequestLine);
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
      SipMessage message;
      
      Preparse parse(message, txt, strlen(txt));
      parse.process();
      assert(message.getRawHeader(Headers::From));
      assert(&message.header(h_From));
      assert(message.header(h_From).exists(p_tag) == false);
      assert(message.header(h_From).exists(p_mobility) == false);
      assert(message.header(h_From).uri().param(p_tag) == "456248");
      assert(message.header(h_From).uri().param(p_mobility) == "hobble");

      message.encode(cerr);
  
      //StatusLine& foo = message.header(h_StatusLine);
      //RequestLine& bar = message.header(h_RequestLine);
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

      SipMessage message;
      
      Preparse parse(message, txt, strlen(txt));
      parse.process();

      assert(message.isRequest());
      assert(message.isResponse() == false);

      assert(message.exists(h_To));
      assert(message.header(h_To).uri().user() == "user");
      assert(message.header(h_To).uri().host() == "company.com");
      assert(message.header(h_To).uri().exists(p_tag) == false);

      assert(message.exists(h_From));
      assert(message.header(h_From).uri().user() == "user");
      assert(message.header(h_From).uri().host() == "company.com");
      assert(message.header(h_From).uri().param(p_tag) == "3411345");

      assert(message.exists(h_MaxForwards));
      assert(message.header(h_MaxForwards).value() == 8);
      assert(message.header(h_MaxForwards).exists(p_tag) == false);

      assert(message.exists(h_Contacts));
      assert(message.header(h_Contacts).empty() == false);
      assert(message.header(h_Contacts).front().uri().user() == "user");
      assert(message.header(h_Contacts).front().uri().host() == "host.company.com");
      assert(message.header(h_Contacts).front().uri().port() == 5060);

      assert(message.exists(h_CallId));
      assert(message.header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      assert(message.exists(h_CSeq));
      assert(message.header(h_CSeq).sequence() == 8);
      assert(message.header(h_CSeq).method() == REGISTER);

      assert(message.exists(h_Vias));
      assert(message.header(h_Vias).empty() == false);
      assert(message.header(h_Vias).front().protocolName() == "SIP");
      assert(message.header(h_Vias).front().protocolVersion() == "2.0");
      assert(message.header(h_Vias).front().transport() == "UDP");
      assert(message.header(h_Vias).front().sentHost() == "135.180.130.133");
      assert(message.header(h_Vias).front().sentPort() == 5060);

      assert(message.exists(h_Expires));
      assert(message.header(h_Expires).value() = 353245);

      cerr << "Headers::Expires enum = " << h_Expires.getTypeNum() << endl;
      
      stringstream str;
      message.encode(cerr);
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

      SipMessage message;
      
      Preparse parse(message, txt, strlen(txt));
      parse.process();

      assert(message.header(h_MaxForwards).value() == 8);
      message.getRawHeader(Headers::Max_Forwards)->getParserContainer()->encode(cerr) << endl;
   }
}

