#include <iostream>
#include <iomanip>
#include <stdio.h>

#include "sipstack/Preparse.hxx"
#include "util/Logger.hxx"
#include "sipstack/SipMessage.hxx"

#include <ctype.h>

using namespace Vocal2;
using namespace  std;

#define VOCAL_SUBSYSTEM Subsystem::APP

#define CRLF "\r\n"

    const char testData[] =
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
    CRLF
        "INVITE sip:B@outside.com SIP/2.0" CRLF
        "Via: SIP/2.0/UDP pc.inside.com:5060;branch=z9hG4bK0000" CRLF
        "Max-Forwards: 70" CRLF
        "From: sip:A@inside.com;tag=12345" CRLF
        "To: You <sip:B@outside.com>" CRLF
        "Call-ID: 0123456789@pc.inside.com" CRLF
        "CSeq: 1 INVITE" CRLF
        "Contact: <sip:A@pc.inside.com>" CRLF
        "Content-Length: 0" CRLF
        CRLF
        "REGISTER sip:registrar.biloxi.com SIP/2.0" CRLF
        "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=first" CRLF
        "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=second" CRLF
        "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=third" CRLF
        "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fourth" CRLF
        "Via: SIP/2.0/UDP bobspc.biloxi.com:5060;branch=fifth" CRLF
        "Max-Forwards: 70" CRLF
        "To: Bob <sip:bob@biloxi.com>" CRLF
        "From: Bob <sip:bob@biloxi.com>;tag=456248" CRLF
        "Call-ID: 843817637684230@998sdasdh09" CRLF
        "CSeq: 1826 REGISTER" CRLF
        "Contact: <sip:bob@192.0.2.4>" CRLF
        "Contact: <sip:qoq@192.0.2.4>" CRLF
        "Expires: 7200" CRLF
        "Content-Length: 0" CRLF CRLF
        ;

const char *crlftest = (
    "REGISTER sip:register.com SIP/2.0" CRLF
    "Call-ID: cid" CRLF CRLF 
    );
void od(const char * p, int len)
{
    cout << hex;
    int i = 0;
    
    for (i = 0 ; i < len ; i++)
    {
        if ((i&0xf)==0)
        {
            cout << dec << i << hex << ":\t[ ";
        }
        cout << setw(2) << (unsigned short) p[i] << ' ';

        char c = ' ';
        
        
        if (isalnum(p[i]) || ispunct(p[i])) c = p[i];

        cout << c << ' ';
        
        if ((i&0xf) == 0xf) cout << ']' << endl;
    }

    if ((i&0xf) != 0xf) cout << ']' << endl;
    
    cout << dec;
}

const int nTests = 6;

int tests[nTests];

void rantest(int n)
{
    assert(n>=0);
    assert(n<nTests);
    
    tests[n]++;
}

void inittest()
{
    for(int i = 0 ; i < nTests ; i++) tests[i] = 0;
}


void
checkalltests()
{
    for(int i = 0 ; i < nTests ; i++) assert(tests[i]);
}


void doTest1()
{
    CritLog(<<"doTest1");
    
    SipMessage* msg = new SipMessage();
    Preparse pre;

    int used = 0;
    int len = 100; // (less than the location of the first CRLFCRLF) 
    const char *buffer = strdup(testData);

    PreparseState::TransportAction status = PreparseState::NONE;

    od(buffer,len);
    
    pre.process(*msg, buffer, len, used, status);

    InfoLog(<<"used == 97");
    
    assert(used == 97); // the position of the last header start 
    
    InfoLog(<<"status == fragment?");
    
    assert ( status == PreparseState::fragment);

    InfoLog(<<"TEST1 -- Passed");
    return;
}
    
void
doTest2()
{
    CritLog(<<"doTest2");
    
    SipMessage* msg = new SipMessage();
    Preparse pre;
    const int boundary = strlen(testData); // _index_ of LF
    
    int used = 0;
    int len = boundary; // a point with CRLF @ end of header.
    // there are len chars including the LF

    const char *buffer = testData;
   
    PreparseState::TransportAction status = PreparseState::NONE;

    od(buffer,len);
    
    pre.process(*msg, buffer, len, used, status);

    InfoLog(<<"used ==" << boundary);
    
    assert(used == boundary); // the position of the last header start 
    
    InfoLog(<<"status == fragment?");
    
    assert ( status == PreparseState::fragment);

    InfoLog(<<"TEST2 -- Passed");

    return;

    
}

void
doTest3()
{
   // Build a SipMessage one char per PP call

    assert(0);
}

void
doTest4()
{
   // Split a request line.
   const char *testData[2] = 
      {
         "INVITE sip:vivekg@cha",
         "ir.dnrc.bell-labs.com:5060 SIP/2.0" CRLF
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
      };


    assert(0);
}

void
doTest6()
{
    Preparse pre;
    
    SipMessage msg;
    
    int k;
    int len = strlen(crlftest);
    char *p = strdup(crlftest);
    
    msg.addBuffer(p);
    
    // From test case provided by DCM.

    PreparseState::TransportAction status;
    
    pre.process(msg, p, len, k, status);
    
    assert(status == PreparseState::headersComplete);
    assert( k == len);

}




int
main(int argc, char *argv[])
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
   
    inittest();

    //doTest1(); // frag'd header
    //doTest2(); // no frag'd header, but NOT End of headers
    //doTest3(); // end at end
    //doTest4(); // pp error
    //doTest5(); // build sipmessage one byte at a time.
    doTest6(); // trailing CRLFCRLF eating test.

    checkalltests();
    
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
