#include <sipstack/SipMessage.hxx>
#include <sipstack/Preparse.hxx>

#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      
      char *txt = "REGISTER sip:registrar.biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\nMax-Forwards: 70\r\nTo: Bob <sip:bob@biloxi.com>\r\nFrom: Bob <sip:bob@biloxi.com>;tag=456248\r\nCall-ID: 843817637684230@998sdasdh09\r\nCSeq: 1826 REGISTER\r\nContact: <sip:bob@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";
      SipMessage message;
      
      Preparse parse(message, txt, strlen(txt));
      parse.process();
      
      message.encode(cerr);
      
      Data v = message.header(h_CallId).value();
      cerr << "Call-ID is " << v << endl;
      //StatusLine& foo = message.header(h_StatusLine);
      //RequestLine& bar = message.header(h_RequestLine);
      //cerr << bar.getMethod() << endl;
   }
   {
      
      char *txt = "REGISTER sip:registrar.biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=z9hG4bKnashds7\r\nMax-Forwards: 70\r\nTo: Bob <sip:bob@biloxi.com>\r\nFrom: Bob <sip:bob@biloxi.com>;tag=456248\r\nCall-ID: 843817637684230@998sdasdh09\r\nCSeq: 1826 REGISTER\r\nContact: <sip:bob@192.0.2.4>\r\nExpires: 7200\r\nContent-Length: 0\r\n\r\n";
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
   
}
