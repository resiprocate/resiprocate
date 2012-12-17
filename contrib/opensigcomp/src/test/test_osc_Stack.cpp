/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */


#include <iostream>
#include "TestList.h"
#include "torture_tests.h"

#include "Stack.h"
#include "StateHandler.h"
#include "SigcompMessage.h"
#include "DeflateCompressor.h"
#include "StateChanges.h"
#include "TcpStream.h"
#include "SipDictionary.h"


typedef struct
{
  bool fromAlice;
  char *value;
} message_t;

message_t message[] =
{
  { 
    true,

    "REGISTER sip:estacado.net SIP/2.0\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-et736vsjirav;rport\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=6to4gh7t5j\r\n"
    "To: \"Adam Roach\" <sip:2145550500@estacado.net>\r\n"
    "Call-ID: 3c26700c1adb-lu1lz5ri5orr@widget3000\r\n"
    "CSeq: 215196 REGISTER\r\n"
    "Max-Forwards: 70\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>;q=1.0;"
      "+sip.instance=\"<urn:uuid:2e5fdc76-00be-4314-8202-1116fa82a473>\""
      ";audio;mobility=\"fixed\";duplex=\"full\";description=\"widget3000\""
      ";actor=\"principal\";events=\"dialog\";methods=\"INVITE,ACK,CANCEL,"
      "BYE,REFER,OPTIONS,NOTIFY,SUBSCRIBE,PRACK,MESSAGE,INFO\"\r\n"
    "Supported: gruu\r\n"
    "Allow-Events: dialog\r\n"
    "Authorization: Digest username=\"2145550500\",realm=\"estacado.net\","
      "nonce=\"4191a4cd\",uri=\"sip:estacado.net\","
      "response=\"4e37bd0095dfc8667dbee0c3bb5ffd44\",algorithm=md5\r\n"
    "Expires: 60\r\n"
    "Content-Length: 0\r\n"
    "\r\n"

  },
  { 
    false,

    "SIP/2.0 200 OK\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-et736vsjirav\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=6to4gh7t5j\r\n"
    "To: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=as7f1d7e54\r\n"
    "Call-ID: 3c26700c1adb-lu1lz5ri5orr@widget3000\r\n"
    "CSeq: 215196 REGISTER\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER\r\n"
    "Expires: 60\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>;expires=60\r\n"
    "Date: Fri, 06 Jan 2006 18:26:28 GMT\r\n"
    "Content-Length: 0\r\n"
    "\r\n"

  },
  { 
    true,

    "INVITE sip:2145550444@estacado.net;user=phone SIP/2.0\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-6vi6sa58smfx;rport\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 1 INVITE\r\n"
    "Max-Forwards: 70\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>\r\n"
    "P-Key-Flags: resolution=\"31x13\", keys=\"4\"\r\n"
    "Accept: application/sdp\r\n"
    "Allow: INVITE, ACK, CANCEL, BYE, REFER, OPTIONS, NOTIFY, SUBSCRIBE, "
      "PRACK, MESSAGE, INFO\r\n"
    "Allow-Events: talk, hold, refer\r\n"
    "Supported: timer, 100rel, replaces, callerid\r\n"
    "Session-Expires: 3600\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 466\r\n"
    "\r\n"
    "v=0\r\n"
    "o=root 1411917766 1411917766 IN IP4 172.17.1.247\r\n"
    "s=call\r\n"
    "c=IN IP4 172.17.1.247\r\n"
    "t=0 0\r\n"
    "m=audio 61586 RTP/AVP 0 8 3 18 4 9 101\r\n"
    "k=base64:0XKVWY0qqljRgu1C5HlBIug4puMhqo022Hxow5KcKtE=\r\n"
    "a=rtpmap:0 pcmu/8000\r\n"
    "a=rtpmap:8 pcma/8000\r\n"
    "a=rtpmap:3 gsm/8000\r\n"
    "a=rtpmap:18 g729/8000\r\n"
    "a=rtpmap:4 g723/8000\r\n"
    "a=rtpmap:9 g722/8000\r\n"
    "a=rtpmap:101 telephone-event/8000\r\n"
    "a=fmtp:101 0-15\r\n"
    "a=ptime:20\r\n"
    "a=encryption:optional\r\n"
    "a=alt:1 0.9 : user 9kksj== 172.17.1.247 61586\r\n"
    "a=sendrecv\r\n"

  },
  { 
    false,

    "SIP/2.0 407 Proxy Authentication Required\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-6vi6sa58smfx\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>;"
      "tag=as3f1ef6ee\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 1 INVITE\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER\r\n"
    "Contact: <sip:2145550444@172.16.1.5>\r\n"
    "Proxy-Authenticate: Digest realm=\"estacado.net\", nonce=\"0665e6a2\"\r\n"
    "Content-Length: 0\r\n"
    "\r\n"

  },
  { 
    true,

    "ACK sip:2145550444@estacado.net;user=phone SIP/2.0\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-6vi6sa58smfx;rport\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>;"
      "tag=as3f1ef6ee\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 1 ACK\r\n"
    "Max-Forwards: 70\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>\r\n"
    "Content-Length: 0\r\n"
    "\r\n"

  },
  { 
    true,

    "INVITE sip:2145550444@estacado.net;user=phone SIP/2.0\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-mvomytni0j9l;rport\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 2 INVITE\r\n"
    "Max-Forwards: 70\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>\r\n"
    "P-Key-Flags: resolution=\"31x13\", keys=\"4\"\r\n"
    "Accept: application/sdp\r\n"
    "Allow: INVITE, ACK, CANCEL, BYE, REFER, OPTIONS, NOTIFY, SUBSCRIBE, "
      "PRACK, MESSAGE, INFO\r\n"
    "Allow-Events: talk, hold, refer\r\n"
    "Supported: timer, 100rel, replaces, callerid\r\n"
    "Session-Expires: 3600\r\n"
    "Proxy-Authorization: Digest username=\"2145550500\","
      "realm=\"estacado.net\",nonce=\"0665e6a2\","
      "uri=\"sip:2145550444@estacado.net;user=phone\","
      "response=\"b33f66e6612daffeee07099387fcf95a\",algorithm=md5\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 466\r\n"
    "\r\n"
    "v=0\r\n"
    "o=root 1411917766 1411917766 IN IP4 172.17.1.247\r\n"
    "s=call\r\n"
    "c=IN IP4 172.17.1.247\r\n"
    "t=0 0\r\n"
    "m=audio 61586 RTP/AVP 0 8 3 18 4 9 101\r\n"
    "k=base64:0XKVWY0qqljRgu1C5HlBIug4puMhqo022Hxow5KcKtE=\r\n"
    "a=rtpmap:0 pcmu/8000\r\n"
    "a=rtpmap:8 pcma/8000\r\n"
    "a=rtpmap:3 gsm/8000\r\n"
    "a=rtpmap:18 g729/8000\r\n"
    "a=rtpmap:4 g723/8000\r\n"
    "a=rtpmap:9 g722/8000\r\n"
    "a=rtpmap:101 telephone-event/8000\r\n"
    "a=fmtp:101 0-15\r\n"
    "a=ptime:20\r\n"
    "a=encryption:optional\r\n"
    "a=alt:1 0.9 : user 9kksj== 172.17.1.247 61586\r\n"
    "a=sendrecv\r\n"

  },
  { 
    false,

    "SIP/2.0 100 Trying\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-mvomytni0j9l\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>;"
      "tag=as6d8c19c7\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 2 INVITE\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER\r\n"
    "Contact: <sip:2145550444@172.16.1.5>\r\n"
    "Content-Length: 0\r\n"
    "\r\n"

  },
  { 
    false,

    "SIP/2.0 200 OK\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-mvomytni0j9l\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>;"
      "tag=as6d8c19c7\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 2 INVITE\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER\r\n"
    "Contact: <sip:2145550444@172.16.1.5>\r\n"
    "Content-Type: application/sdp\r\n"
    "Content-Length: 212\r\n"
    "\r\n"
    "v=0\r\n"
    "o=root 3086 3086 IN IP4 172.16.1.5\r\n"
    "s=session\r\n"
    "c=IN IP4 172.16.1.5\r\n"
    "t=0 0\r\n"
    "m=audio 14322 RTP/AVP 0 101\r\n"
    "a=rtpmap:0 PCMU/8000\r\n"
    "a=rtpmap:101 telephone-event/8000\r\n"
    "a=fmtp:101 0-16\r\n"
    "a=silenceSupp:off - - - -\r\n"

  },
  { 
    true,

    "ACK sip:2145550444@172.16.1.5 SIP/2.0\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-q4jlg3rw6hog;rport\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>;"
      "tag=as6d8c19c7\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 2 ACK\r\n"
    "Max-Forwards: 70\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>\r\n"
    "Content-Length: 0\r\n"
    "\r\n"

  },
  { 
    true,

    "BYE sip:2145550444@172.16.1.5 SIP/2.0\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-mhsswpesd0a7;rport\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>;"
      "tag=as6d8c19c7\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 3 BYE\r\n"
    "Max-Forwards: 70\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>\r\n"
    "Content-Length: 0\r\n"
    "\r\n"

  },
  { 
    false,

    "SIP/2.0 200 OK\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-mhsswpesd0a7\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=4at3wehz8c\r\n"
    "To: \"Voicemail\" <sip:2145550444@estacado.net;user=phone>;"
      "tag=as6d8c19c7\r\n"
    "Call-ID: 3c58339ed1f6-lvfoul2ixa8h@widget3000\r\n"
    "CSeq: 3 BYE\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER\r\n"
    "Contact: <sip:2145550444@172.16.1.5>\r\n"
    "Content-Length: 0\r\n"
  },
  { false, 0 }
};

bool test_osc_Stack_oneCallUdp()
{
  // Set up Alice's environment
  osc::StateHandler aliceSh(8192,64,8192,2);
  osc::Stack aliceStack(aliceSh);
  aliceStack.addCompressor(new osc::DeflateCompressor(aliceSh));

  // Set up Bob's environment
  osc::StateHandler bobSh(8192,64,8192,2);
  osc::Stack bobStack(bobSh);
  bobStack.addCompressor(new osc::DeflateCompressor(bobSh));

  osc::SigcompMessage *sm;
  osc::StateChanges *sc;

  osc::byte_t output[8192];
  size_t outputSize;

  int compartmentId = 0x12345678;

  for (int i = 0; message[i].value; i++)
  {
    if (message[i].fromAlice)
    {
      sm = aliceStack.compressMessage(
             reinterpret_cast<osc::byte_t*>(message[i].value),
             strlen(message[i].value), compartmentId);

      outputSize = bobStack.uncompressMessage(
                     sm->getDatagramMessage(), sm->getDatagramLength(),
                     output, sizeof(output), sc);

      bobStack.provideCompartmentId(sc, compartmentId);

      // Check for NACK
      osc::SigcompMessage *n = bobStack.getNack();
      if (n) { std::cout << (*n) << std::endl;}
    }
    else
    {
      sm = bobStack.compressMessage(
             reinterpret_cast<osc::byte_t*>(message[i].value),
             strlen(message[i].value), compartmentId);

      outputSize = aliceStack.uncompressMessage(
                     sm->getDatagramMessage(), sm->getDatagramLength(),
                     output, sizeof(output), sc);

      aliceStack.provideCompartmentId(sc, compartmentId);

      // Check for NACK
      osc::SigcompMessage *n = aliceStack.getNack();
      if (n) { std::cout << (*n) << std::endl;}
    }

    int origSize = strlen(message[i].value);
    int compSize = sm->getDatagramLength();

    char mt[40];
    char *start;
    char *end;
    strncpy (mt, message[i].value, sizeof(mt));
    if (strstr(mt, "SIP") == mt)
    {
      start = strchr(mt, ' ')+1;
    }
    else
    {
      start = mt;
    }
    end = strchr(start, ' ');
    *end = '\0';

    std::cout << "    - Message " << std::setw(2) << (i+1) 
              << " [" << std::setw(8) << start << "]"
              << ": (" 
              << (message[i].fromAlice?"a->b":"b->a")
              << "), from " 
              << std::setw(4) << origSize << " to " 
              << std::setw(4) << compSize << " bytes (" 
              << std::setw(3) << ((compSize * 100) / origSize)
              << "% original)"
              << std::endl;
    TEST_ASSERT_EQUAL(outputSize, strlen(message[i].value));
    TEST_ASSERT_EQUAL_BUFFERS(message[i].value, output, outputSize);

    delete (sm);
  }

  return true;
}

bool
test_osc_Stack_oneCallTcp()
{
  // Set up Alice's environment
  osc::StateHandler aliceSh(8192 * 2,64,8192,2);
  osc::Stack aliceStack(aliceSh);
  aliceStack.addCompressor(new osc::DeflateCompressor(aliceSh));
  osc::TcpStream aliceStream;

  // Set up Bob's environment
  osc::StateHandler bobSh(8192 * 2,64,8192,2);
  osc::Stack bobStack(bobSh);
  bobStack.addCompressor(new osc::DeflateCompressor(bobSh));
  osc::TcpStream bobStream;

  osc::SigcompMessage *sm;
  osc::StateChanges *sc;

  osc::byte_t output[8192];
  size_t outputSize;

  int compartmentId = 0x12345678;

  for (int i = 0; message[i].value; i++)
  {
    if (message[i].fromAlice)
    {
      sm = aliceStack.compressMessage(
             reinterpret_cast<osc::byte_t*>(message[i].value),
             strlen(message[i].value), compartmentId, true);

      bobStream.addData(sm->getStreamMessage(), sm->getStreamLength());

      outputSize = bobStack.uncompressMessage(bobStream,
                                              output, sizeof(output), sc);

      bobStack.provideCompartmentId(sc, compartmentId);

      // Check for NACK
      osc::SigcompMessage *n = bobStack.getNack();
      if (n) { std::cout << (*n) << std::endl;}
    }
    else
    {
      sm = bobStack.compressMessage(
             reinterpret_cast<osc::byte_t*>(message[i].value),
             strlen(message[i].value), compartmentId, true);

      aliceStream.addData(sm->getStreamMessage(), sm->getStreamLength());

      outputSize = aliceStack.uncompressMessage(aliceStream,
                     output, sizeof(output), sc);

      aliceStack.provideCompartmentId(sc, compartmentId);

      // Check for NACK
      osc::SigcompMessage *n = aliceStack.getNack();
      if (n) { std::cout << (*n) << std::endl;}
    }

    int origSize = strlen(message[i].value);
    int compSize = sm->getDatagramLength();

    char mt[40];
    char *start;
    char *end;
    strncpy (mt, message[i].value, sizeof(mt));
    if (strstr(mt, "SIP") == mt)
    {
      start = strchr(mt, ' ')+1;
    }
    else
    {
      start = mt;
    }
    end = strchr(start, ' ');
    *end = '\0';

    std::cout << "    - Message " << std::setw(2) << (i+1) 
              << " [" << std::setw(8) << start << "]"
              << ": (" 
              << (message[i].fromAlice?"a->b":"b->a")
              << "), from " 
              << std::setw(4) << origSize << " to " 
              << std::setw(4) << compSize << " bytes (" 
              << std::setw(3) << ((compSize * 100) / origSize)
              << "% original)"
              << std::endl;
    TEST_ASSERT_EQUAL(outputSize, strlen(message[i].value));
    TEST_ASSERT_EQUAL_BUFFERS(message[i].value, output, outputSize);

    delete (sm);
  }

  return true;
}

/**
  @todo Add code to check NACK details -- will require
        additional fields on torture test structure.
*/
bool runStackTortureTest(osc::Stack &s, int i)
{
  osc::byte_t output[8192];
  osc::StateChanges *sc;
  size_t result;

  static osc::TcpStream tcpStream;

  std::cout << "    - [" << i << "] "<< tortureTest[i].name << std::endl;

  if (tortureTest[i].streamInput)
  {
    tcpStream.addData((osc::byte_t*)tortureTest[i].sigcompMessage,
                      tortureTest[i].sigcompMessageLength);
    
    result = s.uncompressMessage(tcpStream, output, sizeof(output), sc);
  }
  else
  {
    result = s.uncompressMessage((osc::byte_t*)(tortureTest[i].sigcompMessage),
                                 tortureTest[i].sigcompMessageLength,
                                 output, sizeof(output), sc);
  }

  TEST_ASSERT_EQUAL_BUFFERS(output, 
                            (osc::byte_t*)tortureTest[i].expectedOutput,
                            result);
  TEST_ASSERT_EQUAL(s.getStatus(), tortureTest[i].failure);

  osc::SigcompMessage *nack = s.getNack();
  if (tortureTest[i].failure)
  {
    TEST_ASSERT(nack);
    TEST_ASSERT_EQUAL(nack->getNackReason(), tortureTest[i].failure);

    // There's no easy way from the outside to verify the TCP
    // nack generation.
    if (!tortureTest[i].streamInput)
    {
      osc::Buffer message((osc::byte_t*)(tortureTest[i].sigcompMessage),
                           tortureTest[i].sigcompMessageLength);
      osc::Buffer hash;
      message.getSha1Digest(hash);
      TEST_ASSERT_EQUAL_BUFFERS(hash.data(), nack->getNackSha1().digest, 20);
    }
    delete nack;
  }
  else
  {
    TEST_ASSERT(!nack);
    s.provideCompartmentId(sc, tortureTest[i].compartmentId);
  }

  return true;
}

// Test generation of NACKs (version 1, version 2, each error type)
bool
test_osc_Stack_localNack()
{

  // Version 1 should never generate NACKs.
  {
    osc::StateHandler sh1(8192 * 2,64,8192,1);
    osc::Stack stack1(sh1);
    osc::TcpStream stream1;
    osc::byte_t garbageIn[] = {0xff,0x12,0x34,0x56};
    osc::byte_t garbageOut[1024];
    osc::StateChanges *sc;

    size_t result = stack1.uncompressMessage(garbageIn, sizeof(garbageIn),
                                             garbageOut, sizeof(garbageOut),
                                             sc);
    TEST_ASSERT_EQUAL(result, 0);
    TEST_ASSERT_EQUAL(stack1.getNack(), 0);
    TEST_ASSERT_EQUAL(stack1.getStatus(), osc::MESSAGE_TOO_SHORT);
  }

  osc::StateHandler sh(2048,cyclesPerBit,16384,2);
  osc::Stack stack(sh);

  osc::State *sipDict = new osc::SipDictionary();
  sh.addState(sipDict);

  bool result = true;

  // The torture tests do a pretty good job of testing almost all the
  // NACK codes. This is good enough for now.
  for (int i = 0; tortureTest[i].name; i++)
  {
    bool flag = runStackTortureTest(stack, i);
    if (!flag)
    {
      std::cout << "      !!! Stack Torture Test Failure" << std::endl;
    }
    result &= flag;
  }

  return result;
}

bool
test_osc_Stack_remoteNack()
{
  int compartmentId = 0x12345678;
  osc::StateHandler sh(8192 * 4,64,8192,2);
  osc::Stack stack(sh);
  stack.addCompressor(new osc::DeflateCompressor(sh));

  osc::byte_t output[8192];
  size_t outputSize;
  osc::StateChanges *sc;

  osc::SigcompMessage *sm1;
  osc::SigcompMessage *sm2;
  osc::SigcompMessage *sm3;
  osc::SigcompMessage *nack1;
  osc::SigcompMessage *sm4;
  osc::SigcompMessage *nack2;
  osc::SigcompMessage *sm5;

  // Compress a first message
  sm1 = stack.compressMessage(
          reinterpret_cast<osc::byte_t*>(message[1].value),
          strlen(message[1].value), compartmentId, true);

  std::cout << "    - [sm1] Compressed to " << sm1->getDatagramLength()
            << " bytes" << std::endl;
  // sm1->dump(std::cout,8);

  // We'll decompress the message for the purposes of conveying proper
  // capability information back to the Compartment.
  outputSize = stack.uncompressMessage(
                     sm1->getDatagramMessage(), sm1->getDatagramLength(),
                     output, sizeof(output), sc);
  stack.provideCompartmentId(sc, compartmentId);

  // Compress the same message a second time; should be much smaller.
  sm2 = stack.compressMessage(
          reinterpret_cast<osc::byte_t*>(message[1].value),
          strlen(message[1].value), compartmentId, true);

  std::cout << "    - [sm2] Compressed to " << sm2->getDatagramLength()
            << " bytes" << std::endl;
  // sm2->dump(std::cout,8);

  TEST_ASSERT(sm2->getDatagramLength() < sm1->getDatagramLength());

  // Compress a different message
  sm3 = stack.compressMessage(
          reinterpret_cast<osc::byte_t*>(message[3].value),
          strlen(message[3].value), compartmentId, true);

  std::cout << "    - [sm3] Compressed to " << sm3->getDatagramLength()
            << " bytes" << std::endl;
  // sm3->dump(std::cout,8);

  // Now, the other side complains that it can't find the state
  // that had been created by sm2 (and sm2 tried to use)
  nack1 = new osc::SigcompMessage(osc::STATE_NOT_FOUND,
                                  0,0,
                                  *sm3,
                                  sm3->getStateId(),
                                  sm3->getStateIdLength());
  std::cout << "    - [nack1] Simulating NACK (STATE_NOT_FOUND)" << std::endl;
  // nack1->dump(std::cout,8);

  outputSize = stack.uncompressMessage(
                     nack1->getDatagramMessage(), nack1->getDatagramLength(),
                     output, sizeof(output), sc);
  TEST_ASSERT_EQUAL(outputSize,0);


  // Try to recompress sm3 again. Since the state from sm2 and sm3
  // aren't available, it will try to re-use the state from sm1 (which
  // was also used by sm2).
  sm4 = stack.compressMessage(
          reinterpret_cast<osc::byte_t*>(message[3].value),
          strlen(message[3].value), compartmentId, true);

  std::cout << "    - [sm4] Compressed to " << sm4->getDatagramLength()
            << " bytes" << std::endl;
  TEST_ASSERT_EQUAL(sm2->getStateIdLength(),sm4->getStateIdLength());
  TEST_ASSERT_EQUAL_BUFFERS(sm4->getStateId(),
                            sm2->getStateId(), 
                            sm2->getStateIdLength());
  // sm4->dump(std::cout,8);

  // Try a catastrophic error
  std::cout << "    - [nack2] Simulating NACK (INVALID_OPCODE)" << std::endl;
  nack2 = new osc::SigcompMessage(osc::INVALID_OPCODE,
                                  0,0,
                                  *sm4);
  outputSize = stack.uncompressMessage(
                     nack2->getDatagramMessage(), nack2->getDatagramLength(),
                     output, sizeof(output), sc);

  // The subsequent message compression should be just like when
  // we first started up and had no state (since we have no state)
  sm5 = stack.compressMessage(
          reinterpret_cast<osc::byte_t*>(message[1].value),
          strlen(message[1].value), compartmentId, true);

  std::cout << "    - [sm5] Compressed to " << sm1->getDatagramLength()
            << " bytes" << std::endl;

  TEST_ASSERT_EQUAL(sm1->getDatagramLength(), sm5->getDatagramLength());

  delete (sm1);
  delete (sm2);
  delete (sm3);
  delete (sm4);
  delete (sm5);
  delete (nack1);
  delete (nack2);

  return true;
}


static bool StackTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_Stack_oneCallUdp,
                                     "test_osc_Stack_oneCallUdp") &&
  osc::TestList::instance()->addTest(test_osc_Stack_oneCallTcp,
                                     "test_osc_Stack_oneCallTcp") &&
  osc::TestList::instance()->addTest(test_osc_Stack_localNack,
                                     "test_osc_Stack_localNack") &&
  osc::TestList::instance()->addTest(test_osc_Stack_remoteNack,
                                     "test_osc_Stack_remoteNack");
