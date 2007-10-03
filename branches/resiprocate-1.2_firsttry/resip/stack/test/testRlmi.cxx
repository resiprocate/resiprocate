#include <iostream>
#include <memory>

#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/Rlmi.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/test/TestSupport.hxx"
#include "rutil/ParseBuffer.hxx"

using namespace resip;
using namespace std;

int
main()
{
   {
      const Data txt("NOTIFY sip:terminal.example.com SIP/2.0\r\n"
                     "Via: SIP/2.0/TCP pres.example.com;branch=z9hG4bKMgRenTETmm\r\n"
                     "Max-Forwards: 70\r\n"
                     "From: <sip:adam-buddies@pres.example.com>;tag=zpNctbZq\r\n"
                     "To: <sip:adam@example.com>;tag=ie4hbb8t\r\n"
                     "Call-ID: cdB34qLToC@terminal.example.com\r\n"
                     "CSeq: 997935768 NOTIFY\r\n"
                     "Contact: <sip:pres.example.com>\r\n"
                     "Event: presence\r\n"
                     "Subscription-State: active;expires=7200\r\n"
                     "Require: eventlist\r\n"
                     "Content-Type: application/rlmi+xml;charset=\"UTF-8\"\r\n"
                     "Content-Length: 681\r\n"
                     "\r\n"
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<list xmlns=\"urn:ietf:params:xml:ns:rmli\"\r\n"
                     "      uri=\"sip:adam-friends@pres.example.com\" version=\"1\"\r\n"
                     "      name=\"Buddy List at COM\" fullState=\"true\">\r\n"
                     "  <resource uri=\"sip:bob@example.com\" name=\"Bob Smith\">\r\n"
                     "    <instance id=\"juwigmtboe\" state=\"active\"\r\n"
                     "              cid=\"bUZBsM@pres.example.com\"/>\r\n"
                     "  </resource>\r\n"
                     "  <resource uri=\"sip:dave@example.com\" name=\"Dave Jones\">\r\n"
                     "    <instance id=\"hqzsuxtfyq\" state=\"active\"\r\n"
                     "              cid=\"ZvSvkz@pres.example.com\"/>\r\n"
                     "  </resource>\r\n"
                     "  <resource uri=\"sip:ed@example.net\" name=\"Ed at NET\" />\r\n"
                     "  <resource uri=\"sip:adam-friends@example.org\"\r\n"
                     "            name=\"My Friends at ORG\" />\r\n"
                     "</list>\r\n"
                     "\r\n");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      Rlmi* rlmi = dynamic_cast<Rlmi*>(msg->getContents());

      assert(rlmi);

      cerr << rlmi->get() << endl;
   }
}      

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
