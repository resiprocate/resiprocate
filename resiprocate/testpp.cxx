#include <sipstack/Preparse.hxx>

#define CRLF "\r\n"


main()
{
  using namespace Vocal2;

  const char testmsg[] = 
    "INVITE sip:vivekg@chair.dnrc.bell-labs.com:5060 SIP/2.0" CRLF
    "Via: SIP/2.0/UDP 135.180.130.133:5060" CRLF
    "Via: SIP/2.0/TCP 12.3.4.5:5060;branch=9ikj8" CRLF
    "Via: SIP/2.0/UDP 1.2.3.4:5060;hidden" CRLF
    "From: \"J Rosenberg \\\\\\\"\"<sip:jdrosen@lucent.com:5060>;tag=98asjd8" CRLF
    "To: <sip:vivekg@chair.dnrc.bell-labs.com:5060>;tag=1a1b1f1H33n" CRLF
    "Call-ID: 0ha0isndaksdj@10.1.1.1" CRLF
    "CSeq: 8 INVITE"
    "Contact: \"Quoted string \\\"\\\"\" <sip:jdrosen@bell-labs.com:5060>" CRLF
      "Contact: tel:4443322" CRLF
    "Content-Type: application/sdp" CRLF
    "Content-Length: 152" CRLF
    "NewFangledHeader: newfangled value more newfangled value" CRLF
      CRLF
    "v=0" CRLF
    "o=mhandley 29739 7272939 IN IP4 126.5.4.3" CRLF
    "s=" CRLF
      "c=IN IP4 135.180.130.88" CRLF
    "m=audio 49210 RTP/AVP 0 12" CRLF
    "m=video 3227 RTP/AVP 31" CRLF
    "a=rtpmap:31 LPC/8000" CRLF
    CRLF;


   Preparse p(testmsg, sizeof(testmsg)/sizeof(*testmsg));

   return p.process() ? 0 : 1;
}
