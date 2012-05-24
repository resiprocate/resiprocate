#include <iostream>
#include <memory>

#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/test/TestSupport.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   {
      const Data txt("To: <sip:fluffy@h1.cisco1.sipit.net:5060>\r\n"
                     "From: <sip:user@localhost:5080>;tag=2122f945\r\n"
                     "Via: SIP/2.0/UDP 212.157.205.40:5080;branch=z9hG4bK-c87542-472657511-2--c87542-;rport=5080;received=212.157.205.40\r\n"
                     "Call-ID: 5e50445050086b2e\r\n"
                     "CSeq: 1 MESSAGE\r\n"
                     "Contact: <sip:user@212.157.205.40:5080>\r\n"
                     "Max-Forwards: 70\r\n"
                     "Content-Disposition: attachment;handling=required;filename=smime.p7\r\n"
                     "Content-Type: application/pkcs7-mime;smime-type=enveloped-data;name=smime.p7m\r\n"
                     "User-Agent: SIPimp.org/0.2.3 (curses)\r\n"
                     "Content-Length: 385\r\n"
                     "\r\n"
                     "0%82%01}%06%09*%86H%86%f7%0d%01%07%03%a0%82%01n0%82%01j%02%01%001%82%01%160%\r\n"
                     "82%01%12%02%01%000{0p1%0b0%\r\n"
                     "09%06%03U%04%06%13%02US1%130%11%06%03U%04%08%13%0aCalifornia1%110%0f%06%03U%\r\n"
                     "04%07%13%08San Jose1%0e0%0c\r\n"
                     "%06%03U%04%0a%13%05sipit1)0'%06%03U%04%0b%13 Sipit Test Certificate\r\n"
                     "Authority%02%07U%01%81%02I%00v0%0d%\r\n"
                     "06%09*%86H%86%f7%0d%01%01%01%05%00%04%81%80%80nYR%ba%a1%14%9eV%1c%9b<%f3%80%\r\n"
                     "a7%c3%92%b8%0e%10%7f%n%f2%9\r\n"
                     "1(%83%f1n%94%18dS}yi?%11B%cf%d6%00x1%d0jf$%8f%f5%e0l%d3~%b1%1e%e6%db%b7%af%1\r\n"
                     "0w%ca%fc%b9%19%f9r%b6%8a4%f\r\n"
                     "0%d1,t%83%de+%b0%1f%8b%05'%c6%af%c5%dc9%1d%cb%9f7!%08%15%d2A%0b%f2y\"%03%84%e\r\n"
                     "3R%16%c6%15f%baf%e1/P%04Os%\r\n"
                     "90%ea%e9%a1%a8H1%a2%ad%99%a70K%06%09*%86H%86%f7%0d%01%07%010%14%06%08*%86H%8\r\n"
                     "6%f7%0d%03%07%04%08%d7%a3%1\r\n"
                     "4%02%8fO\"%ef%80(%d1_%05%9e%118@%87%b0%a4%87%e45%e4[1%8b%e6%d2%b2%e4%d3.%08%\r\n"
                     "93%16%f0%cf(%86%d3%10%b6%ff\r\n"
                     "%cf%88G%81W");

      auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));

      Pkcs7Contents* pkc = dynamic_cast<Pkcs7Contents*>(msg->getContents());
      assert(pkc);
   }

   cerr << "All OK" << endl;
   return 0;
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
