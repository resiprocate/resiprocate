#include "resip/dum/RedirectManager.hxx"
#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class testRedirectManager : public RedirectManager
{
public:
   testRedirectManager() {};
   virtual ~testRedirectManager() {};

   void testContactOrdering()
   {
      Data origRequestTxt("INVITE sip:192.168.2.92:5100;q=1 SIP/2.0\r\n"
         "To: <sip:yiwen_AT_meet2talk.com@whistler.gloo.net>\r\n"
         "From: Jason Fischl<sip:jason_AT_meet2talk.com@whistler.gloo.net>;tag=ba1aee2d\r\n"
         "Via: SIP/2.0/UDP 192.168.2.220:5060;branch=z9hG4bK-c87542-da4d3e6a.0-1--c87542-;rport=5060;received=192.168.2.220;stid=579667358\r\n"
         "Via: SIP/2.0/UDP 192.168.2.15:5100;branch=z9hG4bK-c87542-579667358-1--c87542-;rport=5100;received=192.168.2.15\r\n"
         "Call-ID: 6c64b42fce01b007\r\n"
         "CSeq: 2 INVITE\r\n"
         "Route: <sip:proxy@192.168.2.220:5060;lr>\r\n"
         "Contact: <sip:192.168.2.15:5100>\r\n"
         "Content-Length: 0\r\n"
         "\r\n");
      Data redirectResponseTxt(
         "SIP/2.0 302 Moved temporarily\r\n"
         "Via: SIP/2.0/UDP 10.10.0.14:15060;branch=z9hG4bK-524287-1---ca637f133a902b1f;rport\r\n"
         "Contact: <sip:4086484899@10.12.0.129:5060;transport=udp;user=phone>;q=0.5\r\n"
         "Contact: <sip:4086484899@10.12.0.130:5060;transport=udp;user=phone>;q=0.33\r\n"
         "Contact: <sip:4086484899@10.12.0.127:5060;transport=udp;user=phone>\r\n"
         "Contact: <sip:4086484899@10.12.0.131:5060;transport=udp;user=phone>;q=0.17\r\n"
         "Contact: <sip:4086484899@10.12.0.128:5060;transport=udp;user=phone>;q=1.0\r\n"
         "To: <sip:4086484899@10.11.0.20>;tag=722127811-1255463299834\r\n"
         "From: <sip:4086484890@10.10.0.14:15060;user=phone>;tag=7fd85658\r\n"
         "Call-ID: NjQ3MTc4OTdlMjE2NmYyYWY2OGQzNDBlMzhmMjBmMGQ.\r\n"
         "CSeq: 1 INVITE\r\n"
         "Content-Length: 0\r\n"
         "\r\n"
         );
      SipMessage* origrequest = SipMessage::make(origRequestTxt, false);
      SipMessage* redirect = SipMessage::make(redirectResponseTxt, false);
      if(origrequest && redirect)
      {
         TargetSet ts(*origrequest, mOrdering);
         ts.addTargets(*redirect);
         SipMessage request;
         bool result;
         result = ts.makeNextRequest(request);
         assert(result);
         InfoLog(<< "Next Request: " << request);
         assert(request.header(h_RequestLine).uri().host() == "10.12.0.127");
         result = ts.makeNextRequest(request);
         assert(result);
         InfoLog(<< "Next Request: " << request);
         assert(request.header(h_RequestLine).uri().host() == "10.12.0.128");
         result = ts.makeNextRequest(request);
         assert(result);
         InfoLog(<< "Next Request: " << request);
         assert(request.header(h_RequestLine).uri().host() == "10.12.0.129");
         result = ts.makeNextRequest(request);
         assert(result);
         InfoLog(<< "Next Request: " << request);
         assert(request.header(h_RequestLine).uri().host() == "10.12.0.130");
         result = ts.makeNextRequest(request);
         assert(result);
         InfoLog(<< "Next Request: " << request);
         assert(request.header(h_RequestLine).uri().host() == "10.12.0.131");
      }
   }
};

int 
main (int argc, char** argv)
{
   testRedirectManager redirectManager;

   redirectManager.testContactOrdering();
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
