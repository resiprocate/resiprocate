#include <iostream>
#include <memory>

#include "TestSupport.hxx"
#include "resiprocate/Contents.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/UnknownHeaderType.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Logger.hxx"
#include "tassert.h"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::APP

#define BUGTRAILINGSPACE " "

void 
test1()
{
  CritLog(<< "2.1 INVITE Parser Torture Test Message" );
      
      char *txt = ("INVITE sip:called@called-company.com SIP/2.0\r\n"
                   "TO :\r\n"
                   " sip:called@called-company.com ;       tag      = 1918181833n\r\n"
                   "From     : \"Caller Name \\\\\\\"\" <sip:caller@caller-company.com>\r\n"
                   "  ;\r\n"
                   "  tag = 98asjd8\r\n"
                   "Max-Forwards: 8\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8\r\n"
                   "   INVITE\r\n"
                   "Via  : SIP  /   2.0\r\n" 
                   " /UDP\n" 
                   "    135.180.130.133;branch=z9hG4bKkdjuw\r\n" 
                   "Subject : \r\n"
                   "NewFangledHeader:   newfangled value\r\n" 
                   " more newfangled value \r\n" 
                   "Content-Type: application/sdp \r\n" 
                   "v:  SIP  / 2.0  / TCP     1192.168.156.222   ;\r\n" 
                   "  branch  =   9ikj8  , \r\n"
                   " SIP  /    2.0   / UDP  192.168.255.111   ; hidden\r\n" 
                   "m:\"Quoted string\\\"\\\"\"<sip:caller@caller-company.com>; newparam =\r\n" 
                   " newvalue ;\r\n" 
                   "  secondparam = secondvalue  ; q = 0.33,\r\n" 
                   " tel:4443322 \r\n"
                   "\r\n"
                   "v=0\r\n" 
                   "o=mhandley 29739 7272939 IN IP4 126.5.4.3 \r\n"
                   "s=-\r\n" 
                   "c=IN IP4 135.180.130.88 \r\n"
                   "t=0 0 \r\n"
                   "m=audio 492170 RTP/AVP 0 12 \r\n"
                   "m=video 3227 RTP/AVP 31 \r\n"
                   "a=rtpmap:31 LPC \r\n\r\n");



      SipMessage* msg = TestSupport::makeMessage(txt);
      

      tassert_reset();
      tassert(msg);

      auto_ptr<SipMessage> message(msg);

      tassert(message->isRequest());
      tassert(message->isResponse() == false);

      tassert(message->exists(h_To));
      tassert(message->header(h_To).uri().user() == "called");
      tassert(message->header(h_To).uri().host() == "called-company.com");
      tassert(message->header(h_To).param(p_tag) == "1918181833n");

      tassert(message->exists(h_From));
      tassert(message->header(h_From).uri().user() == "caller");
      tassert(message->header(h_From).uri().host() == "caller-company.com");

      tassert(message->header(h_From).param(p_tag) == "98asjd8");

      tassert(message->exists(h_MaxForwards));
      tassert(message->header(h_MaxForwards).value() == 8);
      tassert(message->header(h_MaxForwards).exists(p_tag) == false);

      // !ah! compact headers not working.
      // skip if ''not present'' -- actually check by unknown hdr... :-)
      if (message->exists(h_Contacts))
      {
        
        tassert(message->exists(h_Contacts) == true);
        tassert(message->header(h_Contacts).empty() == false);
        tassert(message->header(h_Contacts).front().uri().user() == "caller");
        tassert(message->header(h_Contacts).front().uri().host() == "caller-company.com");
        tassert(message->header(h_Contacts).front().uri().port() == 0);
        
      }
      else
      {
         CritLog(<<"TODO: Compact headers .. doing by unknown interface!!");
         tassert(message->exists(h_Contacts) == false);
         tassert(message->exists(UnknownHeaderType("m")) == true);
         tassert(message->header(UnknownHeaderType("m")).empty() == false);
         //!ah! temporary until compact headers fixed.
         tassert(message->header(UnknownHeaderType("m")).front().value() == 
                 "\"Quoted string\\\"\\\"\"<sip:caller@caller-company.com>;"
                 " newparam =   newvalue ;    secondparam = secondvalue  ; q"
                 " = 0.33,   tel:4443322 ");
      }

      tassert(message->exists(h_CallId));
      tassert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      tassert(message->exists(h_CSeq));
      tassert(message->header(h_CSeq).sequence() == 8);
      tassert(message->header(h_CSeq).method() == INVITE);

      tassert(message->exists(h_Vias));
      tassert(message->header(h_Vias).empty() == false);
      tassert(message->header(h_Vias).front().protocolName() == "SIP");
      tassert(message->header(h_Vias).front().protocolVersion() == "2.0");
      tassert(message->header(h_Vias).front().transport() == "UDP");
      tassert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      tassert(message->header(h_Vias).front().sentPort() == 0);
      tassert(message->header(h_Vias).front().exists(p_branch));
      tassert(message->header(h_Vias).front().param(p_branch).hasMagicCookie());
      tassert(message->header(h_Vias).front().param(p_branch).getTransactionId() == "kdjuw");
      tassert(message->exists(h_Subject));
      tassert(message->header(h_Subject).value().empty());

      tassert(message->exists(UnknownHeaderType("NewFangledHeader")));
      tassert(message->header(UnknownHeaderType("NewFangledHeader")).front().value() == "newfangled value   more newfangled value"BUGTRAILINGSPACE);
      //TODO: Need to check the ContentType header value
      tassert(message->exists(h_ContentType));
      CritLog(<<"TODO:Check content type"); // << *message);
      tassert_verify(1);
   }

   void 
	   test2()
   {
      CritLog( << "2.2 INVITE with Proxy-Require and Require");
       
      char *txt = ("INVITE sip:called@called-company.com SIP/2.0\r\n"
                   "To: sip:called@called-company.com\r\n"
                   "From: sip:caller@caller-company.com;tag=242etr\r\n"
                   "Max-Forwards: 6\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "Require: newfeature1, newfeature2\r\n"
                   "Proxy-Require: newfeature3, newfeature4\r\n" 
                   "CSeq: 8 INVITE\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      tassert_reset();
      
      tassert(message->isRequest());
      tassert(message->isResponse() == false);

      tassert(message->exists(h_To));
      tassert(message->header(h_To).uri().user() == "called");
      tassert(message->header(h_To).uri().host() == "called-company.com");
      tassert(!message->header(h_To).exists(p_tag));

      tassert(message->exists(h_From));
      tassert(message->header(h_From).uri().user() == "caller");
      tassert(message->header(h_From).uri().host() == "caller-company.com");
      tassert(message->header(h_From).exists(p_tag));
      tassert(message->header(h_From).param(p_tag) == "242etr");

      tassert(message->exists(h_MaxForwards));
      tassert(message->header(h_MaxForwards).value() == 6);
      tassert(message->header(h_MaxForwards).exists(p_tag) == false);

      tassert(message->exists(h_Contacts) == false);

      tassert(message->exists(h_CallId));
      tassert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      //TODO: Check Values
      CritLog(<<"TODO: Check values h_Requires");
      tassert(message->exists(h_Requires));

      //TODO: Check values
      CritLog(<<"TODO: Check values h_ProxyRequires");
      tassert(message->exists(h_ProxyRequires));

      tassert(message->exists(h_CSeq));
      tassert(message->header(h_CSeq).sequence() == 8);
      tassert(message->header(h_CSeq).method() == INVITE);

      tassert(message->exists(h_Vias));
      tassert(message->header(h_Vias).empty() == false);
      tassert(message->header(h_Vias).front().protocolName() == "SIP");
      tassert(message->header(h_Vias).front().protocolVersion() == "2.0");
      tassert(message->header(h_Vias).front().transport() == "UDP");
      tassert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      tassert(message->header(h_Vias).front().sentPort() == 0);
      tassert(message->header(h_Vias).front().exists(p_branch));

      // !ah! -- transaction id DOES NOT include the magic cookie
      // (removed z9hG4bK)

      tassert(message->header(h_Vias).front().param(p_branch).getTransactionId() == "kdjuw");
      tassert_verify(2);

   }

void 
test3()
{
       CritLog( << "2.3 INVITE with Unknown Schemes in URIs");
       
      
      char *txt = ("INVITE name:John_Smith SIP/2.0\r\n"
                   "To: isbn:2983792873\r\n"
                   "From: <sip:www.cs.columbia.edu>;tag=3234233\r\n" // was http//
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
                   "a=rtpmap:31 LPC \r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      
      tassert(message->isRequest());
      tassert(message->isResponse() == false);


      // !ah! this will assert(0) in Uri.cxx -- so let's skip this for now
      CritLog(<<"TODO: fix generic Uri handling.");
      tassert_push();
      tassert(message->header(h_RequestLine).getSipVersion() == "SIP/2.0");
      tassert_pop();

      
      tassert(message->exists(h_To));
      tassert(!message->header(h_To).exists(p_tag));

      tassert(message->exists(h_From));

      tassert(message->header(h_From).param(p_tag) == "3234233");


      tassert(message->exists(h_CallId));
      tassert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      tassert(message->exists(h_CSeq));
      tassert(message->header(h_CSeq).sequence() == 8);
      tassert(message->header(h_CSeq).method() == INVITE);

      tassert(message->exists(h_MaxForwards));
      tassert(message->header(h_MaxForwards).value() == 7);
      tassert(message->header(h_MaxForwards).exists(p_tag) == false);

      tassert(message->exists(h_Contacts) == false);

      tassert(message->exists(h_Vias));
      tassert(message->header(h_Vias).empty() == false);
      tassert(message->header(h_Vias).front().protocolName() == "SIP");
      tassert(message->header(h_Vias).front().protocolVersion() == "2.0");
      tassert(message->header(h_Vias).front().transport() == "UDP");
      tassert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      tassert(message->header(h_Vias).front().sentPort() == 0);
      tassert(message->header(h_Vias).front().exists(p_branch));
      tassert(message->header(h_Vias).front().param(p_branch).getTransactionId() == "kdjuw");

      //TODO: Check value 
      CritLog(<<"TODO: Check value of ContentType");
      tassert(message->exists(h_ContentType));
      tassert_verify(3);
      
   }

void 
test4()
{
      CritLog( << "2.4 REGISTER with Y2038 Test (This tests for Absolute Time in Expires)");
       
      char *txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                   "To: sip:user@company.com\r\n"
                   "From: sip:user@company.com;tag=3411345\r\n"
                   "Max-Forwards: 8\r\n"
                   "Contact: sip:user@host.company.com\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8 REGISTER\r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                   "Expires: Sat, 01 Dec 2040 16:00:00 GMT\r\n\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      tassert_reset();
      
      tassert(message->isRequest());
      tassert(message->isResponse() == false);

      tassert(message->header(h_RequestLine).getSipVersion() == "SIP/2.0");

      tassert(message->exists(h_To));
      tassert(message->header(h_To).uri().user() == "user");
      tassert(message->header(h_To).uri().host() == "company.com");
      tassert(!message->header(h_To).exists(p_tag));

      tassert(message->exists(h_From));
      tassert(message->header(h_From).uri().user() == "user");
      tassert(message->header(h_From).uri().host() == "company.com");
      tassert(message->header(h_From).exists(p_tag));
      tassert(message->header(h_From).param(p_tag) == "3411345");

      tassert(message->exists(h_MaxForwards));
      tassert(message->header(h_MaxForwards).value() == 8);
      tassert(message->header(h_MaxForwards).exists(p_tag) == false);

      tassert(message->exists(h_Contacts));
      tassert(message->header(h_Contacts).empty() == false);
      tassert(message->header(h_Contacts).front().uri().user() == "user");
      tassert(message->header(h_Contacts).front().uri().host() == "host.company.com");
      tassert(message->header(h_Contacts).front().uri().port() == 0);

      tassert(message->exists(h_CallId));
      tassert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      tassert(message->exists(h_CSeq));
      tassert(message->header(h_CSeq).sequence() == 8);
      tassert(message->header(h_CSeq).method() == REGISTER);

      tassert(message->exists(h_Vias));
      tassert(message->header(h_Vias).empty() == false);
      tassert(message->header(h_Vias).front().protocolName() == "SIP");
      tassert(message->header(h_Vias).front().protocolVersion() == "2.0");
      tassert(message->header(h_Vias).front().transport() == "UDP");
      tassert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      tassert(message->header(h_Vias).front().sentPort() == 0);
      tassert(message->header(h_Vias).front().exists(p_branch));
      tassert(message->header(h_Vias).front().param(p_branch).getTransactionId() == "kdjuw");

      tassert(message->exists(h_Expires));
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

      tassert(message->header(h_Expires).value() == 3600);
      // Asking for something when it doesnt exist
      tassert(message->exists(h_ContentLength) == false);
      
      message->header(h_ContentLength).value();

      tassert_verify(4);
   }

void 
test5()
   {
      CritLog( << "2.5    INVITE with inconsistent Accept and message body");
      
      char *txt = ("INVITE sip:user@company.com SIP/2.0 \r\n"
                   "To: sip:j_user@company.com \r\n"
                   "From: sip:caller@university.edu;tag=234 \r\n"
                   "Max-Forwards: 5 \r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1 \r\n"
                   "Accept: text/newformat \r\n"
                   "CSeq: 8 INVITE \r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw \r\n"
                   "Content-Type: application/sdp \r\n"
                   "\r\n"
                   "v=0 \r\n"
                   "c=IN IP4 135.180.130.88 \r\n"
                   "m=audio 492170 RTP/AVP 0 12 \r\n"
                   "m=video 3227 RTP/AVP 31 \r\n"
                   "a=rtpmap:31 LPC "
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      tassert_reset();

      tassert(message->isRequest());
      tassert(!message->isResponse());

      tassert(message->header(h_RequestLine).getMethod() == INVITE);
      tassert(message->header(h_RequestLine).getSipVersion() == "SIP/2.0");

      tassert(message->header(h_RequestLine).uri().host() == "company.com");
      tassert(message->header(h_RequestLine).uri().user() == "user");
      tassert(message->header(h_RequestLine).uri().scheme() == "sip");
      tassert(message->header(h_RequestLine).uri().port() == 0);
      tassert(message->header(h_RequestLine).uri().password() == "");

      tassert(message->header(h_To).uri().host() == "company.com");
      tassert(message->header(h_To).uri().user() == "j_user");
      tassert(message->header(h_To).uri().scheme() == "sip");
      tassert(message->header(h_To).uri().port() == 0);
      tassert(message->header(h_To).uri().password() == "");

      tassert(message->header(h_From).uri().host() == "university.edu");
      tassert(message->header(h_From).uri().user() == "caller");
      tassert(message->header(h_From).uri().scheme() == "sip");
      tassert(message->header(h_From).uri().port() == 0);
      tassert(message->header(h_From).uri().password() == "");
      // The tag is a tag on From: not the uri...
      tassert(message->header(h_From).param(p_tag) == "234");

      tassert(message->header(h_MaxForwards).value() == 5);

      tassert(message->header(h_CallId).value() == "0ha0isndaksdj@10.0.0.1");

      tassert(message->header(h_Accepts).size() == 1);
      tassert(message->header(h_Accepts).front().type() == "text");
      tassert(message->header(h_Accepts).front().subType() == "newformat");

      tassert(message->header(h_CSeq).sequence() == 8);
      tassert(message->header(h_CSeq).method() == INVITE);

      tassert(message->header(h_Vias).size() == 1);
      tassert(message->header(h_Vias).front().protocolName() == "SIP");
      tassert(message->header(h_Vias).front().protocolVersion() == "2.0");
      tassert(message->header(h_Vias).front().transport() == "UDP");
      tassert(message->header(h_Vias).front().sentHost() == "135.180.130.133");
      tassert(message->header(h_Vias).front().sentPort() == 0);
      tassert(message->header(h_Vias).front().param(p_branch).hasMagicCookie());
      // !ah! this should go away when parser fixed.
      tassert(message->header(h_Vias).front().param(p_branch).getTransactionId() == "kdjuw");

      tassert(message->header(h_ContentType).type() == "application");
      tassert(message->header(h_ContentType).subType() == "sdp");

      tassert_verify(5);
      // .dlb. someday the body will gack on parse
   }

void test6()
{
      CritLog( << "2.6    INVITE with non-SDP message body ");
      
      char *txt = ("INVITE sip:user@company.com SIP/2.0\r\n"
                   "To: sip:j.user@company.com\r\n"
                   "From: sip:caller@university.edu;tag=8\r\n"
                   "Max-Forwards: 70 \r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1 \r\n"
                   "CSeq: 8 INVITE \r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw \r\n"
                   "Content-Type: application/newformat \r\n"
                   "\r\n"
                   "<audio> <pcmu port=\"443\"/> </audio> \r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      tassert_reset();

      tassert(message->isRequest());
      tassert(!message->isResponse());

      tassert(message->header(h_RequestLine).getMethod() == INVITE);
      tassert(message->header(h_RequestLine).getSipVersion() == "SIP/2.0");

      tassert(message->header(h_RequestLine).uri().host() == "company.com");
      tassert(message->header(h_RequestLine).uri().user() == "user");
      tassert(message->header(h_RequestLine).uri().scheme() == "sip");
      tassert(message->header(h_RequestLine).uri().port() == 0);
      tassert(message->header(h_RequestLine).uri().password() == "");

      tassert(message->header(h_To).uri().host() == "company.com");
      tassert(message->header(h_To).uri().user() == "j.user");
      tassert(message->header(h_To).uri().scheme() == "sip");
      tassert(message->header(h_To).uri().port() == 0);

      tassert(message->header(h_From).uri().host() == "university.edu");
      tassert(message->header(h_From).uri().user() == "caller");
      tassert(message->header(h_From).uri().scheme() == "sip");
      tassert(message->header(h_From).uri().port() == 0);

      tassert(message->header(h_ContentType).type() == "application");
      tassert(message->header(h_ContentType).subType() == "newformat");

      // !jf! should send this to a UAS and it should be rejected (don't
      // understand why) - says because it is not SDP

      tassert_verify(6);
}


void test7()
{
      CritLog( << "2.7    Unknown Method Message");
      
      char *txt = ("NEWMETHOD sip:user@company.com SIP/2.0 \r\n"
                   "To: sip:j.user@company.com \r\n"
                   "From: sip:caller@university.edu;tag=34525 \r\n"
                   "Max-Forwards: 6 \r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1 \r\n"
                   "CSeq: 8 NEWMETHOD \r\n"
                   "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw \r\n"
                   "Content-Type: application/sdp \r\n"
                   "\r\n"
                   "v=0\r\n"
                   "o=mhandley 29739 7272939 IN IP4 126.5.4.3 \r\n"
                   "s=-\r\n"
                   "c=IN IP4 135.180.130.88\r\n"
                   "m=audio 492170 RTP/AVP 0 12\r\n"
                   "m=video 3227 RTP/AVP 31\r\n"
                   "a=rtpmap:31 LPC \r\n"
                   "\r\n");

      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

      tassert_reset();

      tassert(message->isRequest());
      tassert(!message->isResponse());

      tassert(message->header(h_RequestLine).getMethod() == UNKNOWN);
      tassert(message->header(h_RequestLine).unknownMethodName() == "NEWMETHOD");
      tassert(message->header(h_RequestLine).getSipVersion() == "SIP/2.0");

      tassert(message->header(h_RequestLine).uri().host() == "company.com");
      tassert(message->header(h_RequestLine).uri().user() == "user");
      tassert(message->header(h_RequestLine).uri().scheme() == "sip");
      tassert(message->header(h_RequestLine).uri().port() == 0);
      tassert(message->header(h_RequestLine).uri().password() == "");

      tassert(message->header(h_To).uri().host() == "company.com");
      tassert(message->header(h_To).uri().user() == "j.user");
      tassert(message->header(h_To).uri().scheme() == "sip");
      tassert(message->header(h_To).uri().port() == 0);

      tassert(message->header(h_From).uri().host() == "university.edu");
      tassert(message->header(h_From).uri().user() == "caller");
      tassert(message->header(h_From).uri().scheme() == "sip");
      tassert(message->header(h_From).uri().port() == 0);

      tassert(message->header(h_ContentType).type() == "application");
      tassert(message->header(h_ContentType).subType() == "sdp");
      
      DebugLog( << "start map dump" );
      std::map<Mime, ContentsFactoryBase*>& m = Contents::getFactoryMap();
      std::map<Mime, ContentsFactoryBase*>::iterator i;
      i = m.begin();
      while ( i != m.end() )
      {
         DebugLog( << "first=" << i->first );
         i++;
      }
      DebugLog( << "done map dump" );

      Contents* c = message->getContents();
        
      SdpContents* sdp = dynamic_cast<SdpContents*>(c);
      
      sdp->session();
      
      DebugLog( << "got contents of type" << c->getType() );
      
      // A proxy should forward this using the same retransmission rules as 
      // BYE. A UAS should reject it with an error, and list the available 
      // methods in the response. 

      tassert_verify(7);
}


void test8()
{
   CritLog( << "2.8   Unknown Method with CSeq Error ");
   
   char *txt = ("NEWMETHOD sip:user@comapny.com SIP/2.0\r\n"
                "To: sip:j.user@company.com\r\n"
                "From: sip:caller@university.edu;tag=23411413\r\n"
                "Max-Forwards: 3\r\n"
                "Call-ID: 0ha0isndaksdj@10.0.1.1\r\n"
                "CSeq: 8 INVITE\r\n"
                "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                "Content-Type: application/sdp\r\n"
                "\r\n"
                "v=0\r\n"
                "o=mhandley 29739 7272939 IN IP4 126.5.4.3\r\n"
                "s=-\r\n"
                "c=IN IP4 135.180.130.88\r\n"
                "t=0 0\r\n"
                "m=audio 492170 RTP/AVP 0 12\r\n"
                "m=video 3227 RTP/AVP 31\r\n"
                "a=rtpmap:31 LPC\r\n");

   auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
   tassert_reset();
   tassert(message->isRequest());
   tassert(!message->isResponse());

   tassert(message->header(h_RequestLine).getMethod() == UNKNOWN);
   tassert(message->header(h_RequestLine).unknownMethodName() == "NEWMETHOD");
   tassert(message->header(h_CSeq).method() == INVITE);
   tassert(message->header(h_CSeq).method() != message->header(h_RequestLine).getMethod());
   tassert_verify(8);
}

void test9()
{
   CritLog( << "2.9    REGISTER with Unknown Authorization Scheme" );
   
   char* txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                "To: sip:j.user@company.com\r\n"
                "From: sip:j.user@company.com;tag=87321hj23128\r\n"
                "Max-Forwards: 8\r\n"
                "Call-ID: 0ha0isndaksdj@10.0.1.1\r\n"
                "CSeq: 8 REGISTER\r\n"
                "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                "Authorization: Super-PGP foo=ajsohdaosdh0asyhdaind08yasdknasd09asidhas0d8\r\n\r\n");
   auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
   tassert_reset();
   tassert(message->isRequest());
   tassert(!message->isResponse());
   
   tassert(message->header(h_RequestLine).getMethod() == REGISTER);
   tassert(message->header(h_Authorizations).front().scheme() == "Super-PGP");
   tassert_verify(9);
}


void test10()
{
   CritLog( << "2.10 Multiple SIP Request in a Single Message");
   
   char* txt = ("REGISTER sip:company.com SIP/2.0\r\n"
                "To: sip:j.user@company.com\r\n"
                "From: sip:j.user@company.com;tag=43251j3j324\r\n"
                "Max-Forwards: 8\r\n"
                "Call-ID: 0ha0isndaksdj@10.0.2.2\r\n"
                "Contact: sip:j.user@host.company.com\r\n"
                "CSeq: 8 REGISTER\r\n"
                "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                "Content-Length: 0\r\n\r\n"
                "INVITE sip:joe@company.com SIP/2.0\r\n"
                "To: sip:joe@company.com\r\n"
                "From: sip:caller@university.edu;tag=141334\r\n"
                "Max-Forwards: 8\r\n"
                "Call-ID: 0ha0isnda977644900765@10.0.0.1\r\n"
                "CSeq: 8 INVITE\r\n"
                "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                "Content-Type: application/sdp\r\n"
                "\r\n"
                "v=0\r\n"
                "o=mhandley 29739 7272939 IN IP4 126.5.4.3\r\n"
                "s=-\r\n"
                "c=IN IP4 135.180.130.88\r\n"
                "t=0 0\r\n"
                "m=audio 492170 RTP/AVP 0 12\r\n"
                "m =video 3227 RTP/AVP 31\r\n"
                "a=rtpmap:31 LPC\r\n"
                "\r\n");
   tassert_reset();
   try
   {
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
      tassert(0);
   }
   catch (ParseException& e)
   {
   }
   
   tassert_verify(10);
}

void test11()
{
   CritLog( << "2.11 INVITE missing Required Headers");

   char* txt = ("INVITE sip:user@company.com SIP/2.0\r\n"
                "CSeq: 0 INVITE\r\n"
                "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                "Content-Type: application/sdp\r\n"
                "\r\n"
                "v=0\r\n"
                "o=mhandley 29739 7272939 IN IP4 126.5.4.3\r\n"
                "s=-\r\n"
                "c=IN IP4 135.180.130.88\r\n"
                "t=0 0\r\n"
                "m=audio 492170 RTP/AVP 0 12\r\n"
                "m=video 3227 RTP/AVP 31\r\n"
                "a=rtpmap:31 LPC\r\n"
                "\r\n");
   auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
   tassert_reset();
   tassert(message->isRequest());
   tassert(!message->isResponse());

   tassert(!message->exists(h_CallId));
   tassert(!message->exists(h_From));
   tassert(!message->exists(h_To));
   tassert_verify(11);
}

void test12()
{
   CritLog( << "2.12 INVITE with Duplicate Required Headers");
   //with duplicate headers that are not multi, the first header is kept
   
   char* txt = ("INVITE sip:user@company.com SIP/2.0\r\n"
                "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                "Max-Forwards: 70\r\n"
                "CSeq: 0 INVITE\r\n"
                "Call-ID: 98asdh@10.1.1.1\r\n"
                "Call-ID: 98asdh@10.1.1.2\r\n"
                "From: sip:caller@university.edu;tag=3413415\r\n"
                "From: sip:caller@organization.org\r\n"
                "To: sip:user@company.com\r\n"
                "Content-Type: application/sdp\r\n"
                "\r\n"
                "v=0\r\n"
                "o=mhandley 29739 7272939 IN IP4 126.5.4.3\r\n"
                "s=-\r\n"
                "c=IN IP4 135.180.130.88\r\n"
                "t=0 0\r\n"
                "m=audio 492170 RTP/AVP 0 12\r\n"
                "m=video 3227 RTP/AVP 31\r\n"
                "a=rtpmap:31 LPC\r\n"
                "\r\n");
   
   tassert_reset();
   try
   {
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));
   }
   catch (ParseException& e)
   {
      tassert(0);
   }
   tassert_verify(12);
}


void test13()
{
   CritLog( << "2.13 INVITE with lots of header types");
   //with duplicate headers that are not multi, the first header is kept
   
   char* txt = ("INVITE sip:user@company.com SIP/2.0\r\n"
                "User-Agent: Lame Agent\r\n"
                "Via: SIP/2.0/UDP 135.180.130.133;branch=z9hG4bKkdjuw\r\n"
                "Max-Forwards: 70\r\n"
                "CSeq: 0 INVITE\r\n"
                "Call-ID: 98asdh@10.1.1.2\r\n"
                "From: sip:caller@university.edu;tag=3413415\r\n"
                "From: sip:caller@organization.org\r\n"
                "To: sip:user@company.com\r\n"
                "Content-Type: application/sdp\r\n"
                "\r\n"
                "v=0\r\n"
                "o=mhandley 29739 7272939 IN IP4 126.5.4.3\r\n"
                "s=-\r\n"
                "c=IN IP4 135.180.130.88\r\n"
                "t=0 0\r\n"
                "m=audio 492170 RTP/AVP 0 12\r\n"
                "m=video 3227 RTP/AVP 31\r\n"
                "a=rtpmap:31 LPC\r\n"
                "\r\n");
   
   tassert_reset();
   try
   {
      auto_ptr<SipMessage> message(TestSupport::makeMessage(txt));

       tassert(message->header(h_UserAgent).value() == "Lame Agent"); 
       message->header(h_UserAgent).value() = "Foo";
       tassert(message->header(h_UserAgent).value() == "Foo"); 
   }
   catch (ParseException& e)
   {
      tassert(0);
   }
   tassert_verify(13);
}


void test14()
{
   CritLog( << "2.14 Response with lots of headers");
   
   char* txt = ("SIP/2.0 200 OK\n\n"
                "To: <sip:fluffy@example.org>;tag=fb86ad2694115d75c77dce61523c9f07.ca6e\r\n"
                "From: <sip:fluffy@example.org>;tag=a1fd\r\n"
                "Via: SIP/2.0/UDP cj14:5002;branch=z9hG4bK-c87542-472987176-1;received=1.2.3.4\r\n"
                "Call-ID: 048cec32\r\n"
                "CSeq: 2 REGISTER\r\n"
                "Contact: <sip:fluffy@1.2.3.4:5002>;q=1.00;expires=1951\r\n"
                "Contact: <sip:fluffy@example.com:5002>;q=0.50;expires=10\r\n"
                "Contact: <sip:floppy@example.com:5002>;q=0.00;expires=100\r\n"
                "Server: The Server\r\n"
                "Content-Length: 0\r\n"
                "Warning: junk junk junk \r\n"
                "\r\n");
   
   tassert_reset();
   try
   {
      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt));
      
      tassert(  msg->header(h_Contacts).size() == 3 );
      
      resip::ParserContainer<resip::NameAddr>::iterator i =  msg->header(h_Contacts).begin();
      while ( i != msg->header(h_Contacts).end() )
      {
         DebugLog(<< "i=" << *i  );
         i++;
      }
   }
   catch (ParseException& e)
   {
   }
   tassert_verify(14);
}


int
main(int argc, char*argv[])
{
    Log::Level l = Log::DEBUG;
    
    if (argc > 1)
    {
        switch(*argv[1])
        {
            case 'd': l = Log::DEBUG;
                break;
            case 'i': l = Log::INFO;
                break;
            case 's': l = Log::DEBUG_STACK;
                break;
            case 'c': l = Log::CRIT;
                break;
        }
    }
    
    Log::initialize(Log::COUT, l, argv[0]);
    CritLog(<<"Test Driver Starting");
    tassert_init(14);
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();//this should fail as written
    test11();
    test12();
    test13();
    test14();


    tassert_report();

 CritLog(<<"Test Driver Done");
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
