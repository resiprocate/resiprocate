#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Preparse.hxx"
#include "resiprocate/Uri.hxx"

#include <iostream>
#include <sstream>

#include "resiprocate/os/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;
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
