#include <iostream>
#include <memory>

#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/UnknownHeaderType.hxx"
#include "resiprocate/UnknownParameterType.hxx"
#include "resiprocate/os/Logger.hxx"

#include "resiprocate/test/tassert.h"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
#define CRLF "\r\n"

int
main(int argc, char** argv)
{
  
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);
   
   {
     tassert_init(1); (void)tassert_stack_ptr;
     tassert_reset();
     char * txt =(              
       "SIP/2.0 489 Bad Event" CRLF
       "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1--c87542-" CRLF
       "CSeq: 1 SUBSCRIBE" CRLF
       "Allow-Events: " CRLF
       "Call-ID:  f354ce714fb8a95c" CRLF
       "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
       "To:  <sip:RjS@127.0.0.1:5060>" CRLF
       CRLF
       );
     TestSupport::prettyPrint(txt,strlen(txt));

     auto_ptr<SipMessage> response(TestSupport::makeMessage(txt,true));

     cerr << *response << endl;
     
     tassert(response->exists(h_AllowEvents));

     char * txt2 =(              
       "SIP/2.0 489 Bad Event" CRLF
       "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1--c87542-" CRLF
       "CSeq: 1 SUBSCRIBE" CRLF
       "Call-ID:  f354ce714fb8a95c" CRLF
       "From:  <sip:RjS@127.0.0.1:5070>;tag=59e7dd57" CRLF
       "To:  <sip:RjS@127.0.0.1:5060>" CRLF
       "Allow-Events: " CRLF
       CRLF
       );

     SipMessage * r2 = TestSupport::makeMessage(txt2,true);

     tassert(r2->exists(h_AllowEvents) );

     tassert_verify(1);
     tassert_report();
     return 0;
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
