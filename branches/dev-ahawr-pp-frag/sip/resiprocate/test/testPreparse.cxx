#include <iostream>
#include <iomanip>
#include <stdio.h>

#define DEBUG_HELPERS
#include "sip2/sipstack/Preparse.hxx"
#undef DEBUG_HELPERS

#include "sip2/util/Logger.hxx"
#include "sip2/sipstack/SipMessage.hxx"

#include <ctype.h>

using namespace Vocal2;
using namespace  std;

#define VOCAL_SUBSYSTEM Subsystem::APP

#define CRLF "\r\n"

const char reallyShortMessage[] =
"INVITE sip:a@b.com SIP/2.0" CRLF
"From: <sip:c@d.org:5060>;tag=t1" CRLF
"To: <sip:a@b.com:5060>;tag=t2" CRLF
"Call-ID: xx"CRLF
"CSeq: 1 INVITE" CRLF
CRLF
"Sample Body" CRLF
CRLF;

const char wrappy[] = 
"OPTIONS sip:a@b.c SIP/2.0"CRLF
"Via:"CRLF
"    <sip:b@c.com:5060>,"CRLF
"    <sip:d@e.com:5060>,"CRLF
"    <sip:f@g.com:5060>,"CRLF
"    <sip:h@i.com:5060>,"CRLF
"    <sip:j@k.com:5060>"CRLF
"To    : <sip:a@b.com>"CRLF
"From  : <sip:j@k.com>"CRLF
"Subject : "CRLF
"Call-ID: "CRLF
"\txx"CRLF
"CSeq  : "CRLF
"        1 "CRLF
"        OPTIONS"CRLF CRLF;

const char shortMessage[] =
    "INVITE sip:alice@atlanta.com:5060 SIP/2.0" CRLF
    "From: \"Bob\" <sip:bob@baker.org:5060>;tag=first-tag" CRLF
    "To: <sip:alice@atlanta.com:5060>;tag=1a1b1f1H33n" CRLF
    "Call-ID: cid-deadbeef"CRLF
    "CSeq: 1 INVITE" CRLF
    "Contact: <sip:bob@myphone.baker.org:6666>" CRLF
    "Content-Length: 152" CRLF
    "NewFangledHeader: newfangled" CRLF
    CRLF
    "v=0" CRLF
    "o=mhandley 29739 7272939 IN IP4 126.5.4.3" CRLF
    "s=" CRLF
    "c=IN IP4 135.180.130.88" CRLF
    "m=audio 49210 RTP/AVP 0 12" CRLF
    "m=video 3227 RTP/AVP 31" CRLF
    "a=rtpmap:31 LPC/8000" CRLF
    CRLF
;

    const char testData[] =
    "INVITE sip:vivekg@chair.dnrc.bell-labs.com:5060 SIP/2.0" CRLF
    "Via: SIP/2.0/UDP 135.180.130.133:5060" CRLF
    "Via: SIP/2.0/TCP 12.3.4.5:5060;branch=9ikj8" CRLF
    "Via: SIP/2.0/UDP 1.2.3.4:5060;hidden" CRLF
    "From: \"J Rosenberg \\\\\\\"\"<sip:jdrosen@lucent.com:5060>;tag=98asjd8" CRLF
    "To: <sip:vivekg@chair.dnrc.bell-labs.com:5060>;tag=1a1b1f1H33n" CRLF
    "Call-ID: 0ha0isndaksdj@10.1.1.1" CRLF
    "Subject:"CRLF
    "        This is my subject for you to hate\\\"\\\""CRLF
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
        "Subject : "CRLF
        "        My subject."CRLF
        "CSeq: 1826 REGISTER" CRLF
        "Contact: <sip:bob@192.0.2.4>" CRLF
        "Contact: <sip:qoq@192.0.2.4>" CRLF
        "Expires: 7200" CRLF
        "Content-Length: 0" CRLF CRLF
        ;
      char *tortureMsg = ("INVITE sip:called@called-company.com SIP/2.0\r\n"
                   "TO :\n"
                   " sip:called@called-company.com ;       tag      = 1918181833n\r\n"
                   "From     : \"Caller Name \\\\\\\"\" <sip:caller@caller-company.com>\n"
                   "  ;\n"
                   "  tag = 98asjd8\r\n"
                   "Max-Forwards: 8\r\n"
                   "Call-ID: 0ha0isndaksdj@10.0.0.1\r\n"
                   "CSeq: 8\n"
                   "   INVITE\r\n"
                   "Via  : SIP  /   2.0\n" 
                   " /UDP\n" 
                   "    135.180.130.133;branch=z9hG4bKkdjuw\r\n" 
                   "Subject : \r\n"
                   "NewFangledHeader:   newfangled value\n" 
                   " more newfangled value \r\n" 
                   "Content-Type: application/sdp \r\n" 
                   "v:  SIP  / 2.0  / TCP     1192.168.156.222   ;\n" 
                   "  branch  =   9ikj8  , \n"
                   "SIP  /    2.0   / UDP  192.168.255.111   ; hidden\r\n" 
                   "m:\"Quoted string\\\"\\\"\"<sip:caller@caller-company.com>; newparam =\n" 
                   "newvalue ;\n" 
                   "secondparam = secondvalue  ; q = 0.33,\n" 
                   "tel:4443322 \r\n"
                   "\r\n"
                   "v=0\r\n" 
                   "o=mhandley 29739 7272939 IN IP4 126.5.4.3 \r\n"
                   "s=-\r\n" 
                   "c=IN IP4 135.180.130.88 \r\n"
                   "t=0 0 \r\n"
                   "m=audio 492170 RTP/AVP 0 12 \r\n"
                   "m=video 3227 RTP/AVP 31 \r\n"
                   "a=rtpmap:31 LPC \r\n");

extern Data statusName(PreparseState::BufferAction s);

const int chPerRow = 20;

void labels(int len, int row)
{
    int start = chPerRow*row;
    cout << ' ';
    for(int i = 0; i < chPerRow && start+i < len; i++)
    {
        cout << setw(3) << start+i << ' ';
    }
    cout << endl;
}

void banner(int len, int row)
{
    int chThisRow = 0;
    
    if (row >= len/chPerRow)
        chThisRow = len%chPerRow;
    else
        chThisRow = chPerRow;

    if (chThisRow < 1) return;
    
    cout << "+";
    for(int i = 0 ; i < chThisRow; i++)
    {
        cout << "---+";
    }
    cout << endl;
    return;
}

void data(const char * p , int len, int row)
{
    cout << "|";
    for(int c = 0; c < chPerRow; c++)
    {
        int o = row*chPerRow + c;
        if (o >= len) break;
        char ch = p[o];
        
        if (isalnum(ch) || ispunct(ch) || ch == ' ' )
        {
            cout << ' ' << (char)ch << ' ';
        }
        else if ( ch == '\t' )
        {
            cout << " \\t";
        }
        else if ( ch >= '\t' || ch <= '\r')
        {
            cout << " \\" << "tnvfr"[ch-'\t'];
        }
        else
        {
            cout << 'x' << hex << ch << dec;
        }
        cout << '|';
        
        
    }
    cout << endl;
    
}

void pp(const char * p, int len)
{
    int row = 0;
    
    for ( row = 0 ; row <= len/chPerRow ; row++)
    {
        // do this row's banner
        banner(len,row);
        // do this row's data
        data(p,len,row);
        // do this row's banner
        banner(len,row);
        // do this row's counts
        labels(len,row);
        
    }
}

void od(const char * p , int len)
{
    int i;
    
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

const int nTests = 1;

int tests[nTests];

void rantest(int n)
{
    assert(n>0);
    assert(n<=nTests);
    
    tests[n-1]++;
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

static int readRemain = 0;
static const char * readData = 0;


void fakeResetRead(const char *data, int len)
{
    readRemain = len;
    readData =  data;
}

int fakeRead(char *p, int n)
{
    int m = 0;
    // read n bytes into buffer at p.
    while(readRemain > 0 && m < n)
    {
        *p++ = *readData++;
        --readRemain;
        ++m;
    }
    return m;
}


void doTest1()
{
    InfoLog(<<"doTest1");
    
    SipMessage* msg = new SipMessage();
    Preparse pre;

    size_t len = 1;

    PreparseState::BufferAction status = PreparseState::NONE;

    
    // Create a SipMessage
    // take the buffer from the ''transport'' layer and
    // pass it into the preparser.
    // use the same logic as the transports will need 
    // while the status is fragment, re-enter with a new buffer
    // containing the old stuff, as directed.

    len = 1;

    
    size_t discard = 0;
    size_t used = 0;
    size_t start = 0;
    bool chunkMine = true;

    size_t readQuant = 1;
    size_t chunkSize = readQuant;

    // set the ''reader'' to use the test message

//    fakeResetRead(reallyShortMessage,strlen(reallyShortMessage));
    fakeResetRead(testData,strlen(testData));
//    fakeResetRead(tortureMsg,strlen(tortureMsg));
//    fakeResetRead(wrappy,strlen(wrappy));
    
   pp(wrappy,strlen(wrappy));

    // get the first chunk

    char * chunk = new char[chunkSize];
    chunkSize = fakeRead(chunk, readQuant);
    DebugLog(<<"initial chunk size is " << chunkSize);
    DebugLog(<<"initial chunk memory is 0x" << hex << (unsigned long)chunk<<dec);
    
    do
    {
        
        InfoLog(<<"Preparsing " << chunkSize << " bytes");
        InfoLog(<<"len   : " << chunkSize);
        InfoLog(<<"start : " << start);

        pp(chunk,chunkSize);
        
        pre.process(*msg,chunk,chunkSize,start,used,discard,status);

        DebugLog(<<"used   : " << used);
        DebugLog(<<"discard: " << discard);
        DebugLog(<<"status :" << statusName(status));

        if (status & PreparseState::preparseError)
        {
            if (chunkMine)
            {
                DebugLog(<<"deleting chunk 0x"<<hex<<(unsigned long)chunk<<dec);
                delete [] chunk;
            }
            chunk = 0;
            
            CritLog(<<"preparserError -- unexpected");
            assert(~(status & PreparseState::preparseError));
            assert(("whoops you goofed -- should never see this",0));
            
        }
        
        if (status & PreparseState::dataAssigned)
        {
            // something used ... need to add this to the message
            DebugLog(<<"dataAssigned");
            DebugLog(<<*msg);
            if (!chunkMine)
            {
                DebugLog(<<"Duplicate dataAssigned --ignoring");
            }
            else
            {
                DebugLog(<<"addBuffer(0x"<<hex<<(unsigned long)chunk<<dec<<") : something used");
                msg->addBuffer(chunk);
                chunkMine = false;
                // handed to SipMsg
            }
        }

        if (status & PreparseState::fragmented)
        {
            // need to call again with more data,
            // there is an optimization here ...
            // we can reuse the chunk if
            // the PP said it didn't assign any
            // data to the last one..
            DebugLog(<<"Fragmented");

            // Get more memory.

            char * newChunk = new char[chunkSize-discard + readQuant];
            memcpy(newChunk, chunk+discard, chunkSize-discard);

            if (! ( status & PreparseState::dataAssigned))
            {
                // we didn't use the last one ... 
                DebugLog(<<"delete chunk (0x"<<hex<<(unsigned long)chunk<<dec<<")");
                delete [] chunk;
            }
            
            chunk = newChunk;
            DebugLog(<<"new chunk (0x"<<hex<<(unsigned long)chunk<<dec<<")");
            
            chunkMine = true;
            int nBytes = fakeRead(newChunk + chunkSize - discard, readQuant);
            chunkSize = chunkSize - discard + nBytes;

            DebugLog(<<"read in " << readQuant << " more bytes, chunkSize: " << chunkSize);
            start = used - discard;
        }
        else
        {
            // not fragmented just get another buffer
            start = 0;
            DebugLog(<<"NO FRAG -- reading next chunk.");
            if (chunk)
            {
                if (!chunkMine)
                {
                    DebugLog(<<"getting new chunk; last one owned by SipMsg");
                }
                // could reuse it here ... we choose to dispose of it.
                else
                {
                    DebugLog(<<"delete unused chunk size="<<chunkSize);
                    DebugLog(<<"delete chunk = 0x"<<hex<<(unsigned long)chunk<<dec);
                    delete [] chunk;
                    chunk = 0;
                    
                }
            }
            chunk = new char[readQuant];
            chunkMine = true;
            chunkSize = readQuant;
            chunkSize = fakeRead(chunk,chunkSize);
            DebugLog(<<"new chunk = 0x"<<hex<<(unsigned long)chunk<<dec);
            DebugLog(<<"Read fresh " << chunkSize << " bytes.");
        }
        
        
            
        
        
    }
    while (~ status & PreparseState::headersComplete);

    InfoLog(<< "Read in a message");
    InfoLog(<<*msg);
    
#if 0
    od(buffer,len);
    
    pre.process(*msg, buffer, len, used, status);

    InfoLog(<<"used == " << used);
    
    assert(used == 1); // the position of the last header start 
    
    InfoLog(<<"status == fragment?");
    
    assert ( status == PreparseState::fragment);

    InfoLog(<<"TEST1 -- Passed");
#endif

    delete msg;
    
    rantest(1);
    
    return;
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

    doTest1(); // fragging one char at a time

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
