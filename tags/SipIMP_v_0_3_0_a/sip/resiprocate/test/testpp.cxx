#include <iostream.h>

#include "resiprocate/Preparse.hxx"

#define CRLF "\r\n"


main()
{
  using namespace resip;

  const char testmsg[] = 
    "INVITE sip:vivekg@chair.dnrc.bell-labs.com:5060 SIP/2.0" CRLF
    "Via: SIP/2.0/UDP 135.180.130.133:5060" CRLF
    "Via: SIP/2.0/TCP 12.3.4.5:5060;branch=9ikj8" CRLF
    "Via: SIP/2.0/UDP 1.2.3.4:5060;hidden" CRLF
    "From: \"J Rosenberg \\\\\\\"\"<sip:jdrosen@lucent.com:5060>;tag=98asjd8" CRLF
    "To: <sip:vivekg@chair.dnrc.bell-labs.com:5060>;tag=1a1b1f1H33n" CRLF
    "Call-ID: 0ha0isndaksdj@10.1.1.1" CRLF
    "CSeq: 8 INVITE" CRLF
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


  cout << testmsg << endl;

   Preparse p(testmsg, sizeof(testmsg)/sizeof(*testmsg));

   return p.process() ? 0 : 1;
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
