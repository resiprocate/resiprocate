#include "resiprocate/SipStack.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Logger.hxx"

#include "resiprocate/os/DataStream.hxx"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::APP
#define CRLF "\r\n"

char *registerMessage = 
"REGISTER sip:test.com SIP/2.0" CRLF
"Via: SIP/2.0/UDP client.test.com:5060;branch=z9hG4bK-kcD23" CRLF
"Max-Forwards: 70" CRLF
"From: Me <sip:user@test.com>;tag=62e0154b" CRLF
"To: You <sip:you@other.com>" CRLF
"Call-ID: b7e6fb02f0e8413d" CRLF
"CSeq: 1 REGISTER" CRLF
"Contact: <sip:me@123.123.123.123>" CRLF
"Content-Length: 0" CRLF CRLF;


char *inviteMessage =
"INVITE sip:B@127.0.0.1 SIP/2.0" CRLF
"Via: SIP/2.0/UDP 127.0.0.1:5060;branch=z9hG4bK0000" CRLF
"Max-Forwards: 70" CRLF
"From: A <sip:A@127.0.0.1>;tag=12345" CRLF
"To: B <sip:B@127.0.0.1>" CRLF
"Call-ID: 0123456789@127.0.0.1" CRLF
"CSeq: 1 INVITE" CRLF
"Contact: <sip:A@127.0.0.1>" CRLF
"Content-Length: 0" CRLF CRLF;

int
main(int argc, char *argv[])
{
    char* message = inviteMessage;

    Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

    InfoLog( << "Starting up, making stack");

    SipStack *theStack = new SipStack();
    theStack->addTransport(UDP, 5060);

    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // create address to send to
    struct sockaddr_in sa;

    sa.sin_family = PF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(5060);

    DebugLog(<<"size="<<strlen(message) << endl << "message= " << endl << message );
    
    // send the test message to the stack
    int err = sendto(fd, message, strlen(message), 0, (struct sockaddr*) & sa, sizeof(sa));

    DebugLog(<<"errno="<<errno);
    
    assert (err == strlen(message));


    int count=0;
    while (1)
    {
       FdSet fdReadSet;
       
       theStack->buildFdSet(fdReadSet);
       
       fdReadSet.select(1000);
       
       theStack->process(fdReadSet);
       SipMessage* sipMessage = theStack->receive();
            
       if (sipMessage) 
       {
          count++;
          assert (count == 1);
          
          // InfoLog( << "got message " << *sipMessage);
          cout << "to header is " << sipMessage->header(h_To) << endl;
          cout << "to user is " << sipMessage->header(h_To).uri().user() << endl;
          cout << "cseq sequence is " << sipMessage->header(h_CSeq).sequence() << endl;
          cout << "cseq method is " << sipMessage->header(h_CSeq).method() << endl;
          cout << "contact header is " << sipMessage->header(h_Contacts).front() << endl;
          cout << "contact uri " << sipMessage->header(h_Contacts).front().uri().user() << endl;
          cout << "request line " << sipMessage->header(h_RequestLine) << endl;
          cout << "request line uri user" << sipMessage->header(h_RequestLine).uri().user() << endl;
       }

       usleep(20);
    }

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
