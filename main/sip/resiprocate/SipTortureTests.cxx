#include "sipstack/SipMessage.hxx"
#include "sipstack/Helper.hxx"
#include "sipstack/Uri.hxx"
#include "sipstack/Helper.hxx"

#include <iostream>
#include <memory>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      assert(ParameterTypes::getType("tag", 3) == ParameterTypes::tag);
   }

   {
      char* b = "shared buffer";
      HeaderFieldValue h1(b, strlen(b));
      HeaderFieldValue h2(h1);
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
      auto_ptr<SipMessage> message1(Helper::makeMessage(txt1));

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
      auto_ptr<SipMessage> message2(Helper::makeMessage(txt2));

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
      auto_ptr<SipMessage> message1(Helper::makeMessage(txt1));

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
      auto_ptr<SipMessage> message2(Helper::makeMessage(txt2));

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
      auto_ptr<SipMessage> message(Helper::makeMessage(txt));
      
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
      auto_ptr<SipMessage> message(Helper::makeMessage(txt));
      
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
      auto_ptr<SipMessage> message(Helper::makeMessage(txt));
      
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
      auto_ptr<SipMessage> message(Helper::makeMessage(txt));
          
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
      auto_ptr<SipMessage> message(Helper::makeMessage(txt));
      
      assert(message->getRawHeader(Headers::From));
      assert(&message->header(h_From));
      assert(message->header(h_From).exists(p_tag));
      assert(message->header(h_From).exists(p_mobility));
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

      auto_ptr<SipMessage> message(Helper::makeMessage(txt));

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
      assert(message->header(h_Expires).value() = 353245);

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

      auto_ptr<SipMessage> message(Helper::makeMessage(txt));

      assert(message->header(h_MaxForwards).value() == 8);
      message->getRawHeader(Headers::Max_Forwards)->getParserContainer()->encode(cerr) << endl;
   }

   {
      cerr << "2.1 INVITE Parser Torture Test Message" << endl;
      
      char *txt = ("INVITE sip:called@called-company.com SIP/2.0\r\n"
                   "TO :\n"
                   " sip:called@called-company.com ;       tag      = 1918181833n\r\n"
                   "From     : \"Caller Name \\\\\\\"\" <sip:caller@caller-company.com\n"
                   "  ;\n"
                   "  tag = 98asjd8\r\n"
                   "Max-Forwards: 8\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8\n"
                   "   INVITE\r\n"
                   "Via  : SIP  /   2.0\n" 
                   " /UDP\n" 
                   "    135.180.130.133;branch=z9hG4bKkdjuw\r\n" 
                   "Subject : \r\n"
                   "NewFangledHeader:   newfangled value\n" 
                   " more newfangled value \r\n" 
                   "Content-Type: application/sdp \r\n" 
                   "v:  SIP  / 2.0  / TCP     1192.168.156.222   ;\n" 
                   "  branch  =   9ikj8  , \n"
                   "SIP  /    2.0   / UDP  192.168.255.111   ; hidden\r\n" 
                   "m:\"Quoted string\\\"\\\"\"<sip:caller@caller-company.com>; newparam =\n" 
                   "newvalue ;\n" 
                   "secondparam = secondvalue  ; q = 0.33,\n" 
                   "tel:4443322 \r\n"
                   "\r\n"
                   "v=0\r\n" 
                   "o=mhandley 29739 7272939 IN IP4 126.5.4.3 \r\n"
                   "s=-\r\n" 
                   "c=IN IP4 135.180.130.88 \r\n"
                   "t=0 0 \r\n"
                   "m=audio 492170 RTP/AVP 0 12 \r\n"
                   "m=video 3227 RTP/AVP 31 \r\n"
                   "a=rtpmap:31 LPC \r\n");

      auto_ptr<SipMessage> message(Helper::makeMessage(txt));

      assert(message->isRequest());
      assert(message->isResponse() == false);

      assert(message->exists(h_To));
      assert(message->header(h_To).uri().user() == "called");
      assert(message->header(h_To).uri().host() == "called-company.com");
      assert(message->header(h_To).uri().param(p_tag) == "1918181833n");

      assert(message->exists(h_From));
      assert(message->header(h_From).uri().user() == "caller");
      assert(message->header(h_From).uri().host() == "caller-company.com");
      assert(message->header(h_From).uri().param(p_tag) == "98asjd8");

      assert(message->exists(h_MaxForwards));
      assert(message->header(h_MaxForwards).value() == 8);
      assert(message->header(h_MaxForwards).exists(p_tag) == false);

      assert(message->exists(h_Contacts) == false);
//      assert(message->header(h_Contacts).empty() == false);
//      assert(message->header(h_Contacts).front().uri().user() == "user");
//      assert(message->header(h_Contacts).front().uri().host() == "host.company.com");
//      assert(message->header(h_Contacts).front().uri().port() == 0);

      assert(message->exists(h_CallId));
      assert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      assert(message->exists(h_CSeq));
      assert(message->header(h_CSeq).sequence() == 8);
      assert(message->header(h_CSeq).method() == INVITE);

      assert(message->exists(h_Vias));
      assert(message->header(h_Vias).empty() == false);
      assert(message->header(h_Vias).front().protocolName() == "SIP");
      assert(message->header(h_Vias).front().protocolVersion() == "2.0");
      assert(message->header(h_Vias).front().transport() == "UDP");
      assert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      assert(message->header(h_Vias).front().sentPort() == 0);
      assert(message->header(h_Vias).front().exists(p_branch));
      assert(message->header(h_Vias).front().param(p_branch).transactionId() == "9ikj8");

      assert(message->exists(h_Subject));
      assert(message->header(h_Subject).value().empty());

      //TODO: Need to check the value of this header
      assert(message->exists("NewFangledHeader"));

      //TODO: Need to check the ContentType header value
      assert(message->exists(h_ContentType));

      
   }

   {
      cerr << "2.2 INVITE with Proxy-Require and Require" << endl;
      
      char *txt = ("INVITE sip:called@called-company.com SIP/2.0\r\n"
                   "To: sip:called@called-company.com\r\n"
                   "From: sip:caller@caller-company.com;tag=242etr\r\n"
                   "Max-Forwards: 6\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "Require: newfeature1, newfeature2\r\n"
                   "Proxy-Require: newfeature3, newfeature4\r\n" 
                   "CSeq: 8 INVITE\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n");

      auto_ptr<SipMessage> message(Helper::makeMessage(txt));

      assert(message->isRequest());
      assert(message->isResponse() == false);

      assert(message->exists(h_To));
      assert(message->header(h_To).uri().user() == "called");
      assert(message->header(h_To).uri().host() == "called-company.com");
      assert(!message->header(h_To).exists(p_tag));

      assert(message->exists(h_From));
      assert(message->header(h_From).uri().user() == "caller");
      assert(message->header(h_From).uri().host() == "caller-company.com");
      assert(message->header(h_From).exists(p_tag));
      assert(message->header(h_From).uri().param(p_tag) == "242etr");

      assert(message->exists(h_MaxForwards));
      assert(message->header(h_MaxForwards).value() == 8);
      assert(message->header(h_MaxForwards).exists(p_tag) == false);

      assert(message->exists(h_Contacts) == false);

      assert(message->exists(h_CallId));
      assert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      //TODO: Check Values
      assert(message->exists(h_Requires));

      //TODO: Check values
      assert(message->exists(h_ProxyRequires));

      assert(message->exists(h_CSeq));
      assert(message->header(h_CSeq).sequence() == 8);
      assert(message->header(h_CSeq).method() == INVITE);

      assert(message->exists(h_Vias));
      assert(message->header(h_Vias).empty() == false);
      assert(message->header(h_Vias).front().protocolName() == "SIP");
      assert(message->header(h_Vias).front().protocolVersion() == "2.0");
      assert(message->header(h_Vias).front().transport() == "UDP");
      assert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      assert(message->header(h_Vias).front().sentPort() == 0);
      assert(message->header(h_Vias).front().exists(p_branch));
      assert(message->header(h_Vias).front().param(p_branch).transactionId() == "z9hG4bKkdjuw");

   }

   {
      cerr << "2.3 INVITE with Unknown Schemes in URIs" << endl;
      
      char *txt = ("INVITE name:John_Smith SIP/2.0\r\n"
                   "To: isbn:2983792873\r\n"
                   "From: <http://www.cs.columbia.edu>;tag=3234233\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 INVITE\r\n"
                   "Max-Forwards: 7\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Content-Type: application/sdp\r\n" 
                   "\r\n"
                   "v=0\r\n" 
                   "o=mhandley 29739 7272939 IN IP4 126.5.4.3 \r\n"
                   "s=- \r\n"
                   "c=IN IP4 135.180.130.88 \r\n"
                   "t=0 0\r\n"
                   "m=audio 492170 RTP/AVP 0 12 \r\n"
                   "m=video 3227 RTP/AVP 31 \r\n"
                   "a=rtpmap:31 LPC \r\n");

      auto_ptr<SipMessage> message(Helper::makeMessage(txt));

      assert(message->isRequest());
      assert(message->isResponse() == false);

      assert(message->header(h_RequestLine).getSipVersion() == "SIP/2.0");

      assert(message->exists(h_To));
      assert(!message->header(h_To).exists(p_tag));

      assert(message->exists(h_From));
      assert(message->header(h_From).uri().param(p_tag) == "3234233");

      assert(message->exists(h_CallId));
      assert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      assert(message->exists(h_CSeq));
      assert(message->header(h_CSeq).sequence() == 8);
      assert(message->header(h_CSeq).method() == INVITE);

      assert(message->exists(h_MaxForwards));
      assert(message->header(h_MaxForwards).value() == 7);
      assert(message->header(h_MaxForwards).exists(p_tag) == false);

      assert(message->exists(h_Contacts) == false);

      assert(message->exists(h_Vias));
      assert(message->header(h_Vias).empty() == false);
      assert(message->header(h_Vias).front().protocolName() == "SIP");
      assert(message->header(h_Vias).front().protocolVersion() == "2.0");
      assert(message->header(h_Vias).front().transport() == "UDP");
      assert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      assert(message->header(h_Vias).front().sentPort() == 0);
      assert(message->header(h_Vias).front().exists(p_branch));
      assert(message->header(h_Vias).front().param(p_branch).transactionId() == "z9hG4bKkdjuw");

      //TODO: Check value 
      assert(message->exists(h_ContentType));

   }

   {
      cerr << "2.4 REGISTER with Y2038 Test (This tests for Absolute Time in Expires)"<<endl;
      
      char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: Sat, 01 Dec 2040 16:00:00 GMT\r\n");

      auto_ptr<SipMessage> message(Helper::makeMessage(txt));

      assert(message->isRequest());
      assert(message->isResponse() == false);

      assert(message->header(h_RequestLine).getSipVersion() == "SIP/2.0");

      assert(message->exists(h_To));
      assert(message->header(h_To).uri().user() == "user");
      assert(message->header(h_To).uri().host() == "company.com");
      assert(!message->header(h_To).exists(p_tag));

      assert(message->exists(h_From));
      assert(message->header(h_From).uri().user() == "user");
      assert(message->header(h_From).uri().host() == "company.com");
      assert(message->header(h_From).exists(p_tag));
      assert(message->header(h_From).uri().param(p_tag) == "3411345");

      assert(message->exists(h_MaxForwards));
      assert(message->header(h_MaxForwards).value() == 8);
      assert(message->header(h_MaxForwards).exists(p_tag) == false);

      assert(message->exists(h_Contacts) == false);
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
      assert(message->header(h_Vias).front().exists(p_branch));
      assert(message->header(h_Vias).front().param(p_branch).transactionId() == "z9hG4bKkdjuw");

      assert(message->exists(h_Expires));
      /* 
       * Quoted from RFC 3261 
       * The "expires" parameter of a Contact header field value indicates how
       * long the URI is valid.  The value of the parameter is a number
       * indicating seconds.  If this parameter is not provided, the value of
       * the Expires header field determines how long the URI is valid.
       * Malformed values SHOULD be treated as equivalent to 3600.
       * This provides a modest level of backwards compatibility with RFC
       * 2543, which allowed absolute times in this header field.  If an
       * absolute time is received, it will be treated as malformed, and
       * then default to 3600.
       */
      assert(message->header(h_Expires).value() = 3600);


      // Asking for something when it doesnt exist
      message->header(h_ContentLength).value();

   }

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
