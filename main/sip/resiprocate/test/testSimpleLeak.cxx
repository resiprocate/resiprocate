#include "resiprocate/sipstack/SipMessage.hxx"
#include "resiprocate/sipstack/Preparse.hxx"
#include "resiprocate/sipstack/Uri.hxx"

#include <iostream>
#include <sstream>

#include "resiprocate/util/Logger.hxx"
#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace Vocal2;
using namespace std;


int
main(int argc, char*argv[])
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

   DebugLog(<<"Start");

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
   int len = strlen(txt1);
   
   for(int x=0; x < 1000; x++)
   {
      char *d = new char[len];
      for(int i=0;i<len;i++)
         d[i] = txt1[i];
      
      SipMessage message1;
      message1.addBuffer(d);
      Preparse parse1(message1, txt1, strlen(txt1));
      while (parse1.process())
         ;
      
   }
}
