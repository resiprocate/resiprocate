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
       "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1-c87542-" CRLF
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
       "Via: SIP/2.0/UDP RjS.localdomain:5070;branch=z9hG4bK-c87542-899769382-1-c87542-" CRLF
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
