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

#include "ProfileStack.h"
#include <assert.h>
#include <iostream>

#include "Stack.h"
#include "StateHandler.h"
#include "SigcompMessage.h"
#include "DeflateCompressor.h"
#include "StateChanges.h"
#include "TcpStream.h"
#include "SipDictionary.h"

char reqTemplate[]=
    "REGISTER sip:estacado.net SIP/2.0\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-%s;rport\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=6to4gh7t5j\r\n"
    "To: \"Adam Roach\" <sip:2145550500@estacado.net>\r\n"
    "Call-ID: 3c26700c1adb-%s@widget3000\r\n"
    "CSeq: %d REGISTER\r\n"
    "Max-Forwards: 70\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>;q=1.0;"
      "+sip.instance=\"<urn:uuid:2e5fdc76-00be-4314-8202-1116fa82a473>\""
      ";audio;mobility=\"fixed\";duplex=\"full\";description=\"widget3000\""
      ";actor=\"principal\";events=\"dialog\";methods=\"INVITE,ACK,CANCEL,"
      "BYE,REFER,OPTIONS,NOTIFY,SUBSCRIBE,PRACK,MESSAGE,INFO\"\r\n"
    "Supported: gruu\r\n"
    "Allow-Events: dialog\r\n"
    "Authorization: Digest username=\"2145550500\",realm=\"estacado.net\","
      "nonce=\"%8.8X\",uri=\"sip:estacado.net\","
      "response=\"%8.8X%8.8X%8.8X%8.8X\",algorithm=md5\r\n"
    "Expires: 60\r\n"
    "Content-Length: 0\r\n"
    "\r\n"
;

char respTemplate[]=
    "SIP/2.0 200 OK\r\n"
    "Via: SIP/2.0/UDP 172.17.1.247:2078;branch=z9hG4bK-%s\r\n"
    "From: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=6to4gh7t5j\r\n"
    "To: \"Adam Roach\" <sip:2145550500@estacado.net>;tag=as7f1d7e54\r\n"
    "Call-ID: 3c26700c1adb-%s@widget3000\r\n"
    "CSeq: %d REGISTER\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER\r\n"
    "Expires: 60\r\n"
    "Contact: <sip:2145550500@172.17.1.247:2078;line=cbqgzeby>;expires=60\r\n"
    "Content-Length: 0\r\n"
    "\r\n"
;

char *genString(int index)
{
  static char string [2][16];
  for (int i = 0; i < 15; i++)
  {
    string[index][i] = (rand() % 26) + 'a';
  }
  string[index][15] = 0;
  return string[index];
}

int
main(int argc, char **argv)
{
  DEBUG_STACK_FRAME;
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <iterations>" << std::endl;
    exit(__LINE__);
  }
  int iterations = atoi(argv[1]);
  if (iterations <= 0)
  {
    std::cout << "Iterations must be a positive integer, not " << argv[1]
              << std::endl;
    exit(__LINE__);
  }

  std::cout << "Using ~" << sizeof(reqTemplate) << " byte requests" 
           << std::endl;
  std::cout << "Using ~" << sizeof(respTemplate) << " byte responses" 
           << std::endl;

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

  osc::byte_t input[8192];
  size_t inputSize;

  char * string1 = genString(0);
  char * string2 = genString(1);
  int cseq = rand();

  for (int i = 0; i < iterations; i++)
  {
    string1 = genString(0);
    cseq++;

    if (!(i%10))
    {
      compartmentId++;
      string2 = genString(1);
    }

    if (i%2)
    {
      inputSize = snprintf((char *)input, sizeof(input), reqTemplate,
                           string1, string2, cseq,
                           rand(), rand(), rand(), rand());
      sm = aliceStack.compressMessage(
             input, inputSize, compartmentId);

      outputSize = bobStack.uncompressMessage(
                     sm->getDatagramMessage(), sm->getDatagramLength(),
                     output, sizeof(output), sc);

      assert(!bobStack.getNack());
      bobStack.provideCompartmentId(sc, compartmentId);
    }
    else
    {
      inputSize = snprintf((char *)input, sizeof(input), respTemplate,
                           string1, string2, cseq);
      sm = bobStack.compressMessage(
             input, inputSize, compartmentId);

      outputSize = aliceStack.uncompressMessage(
                     sm->getDatagramMessage(), sm->getDatagramLength(),
                     output, sizeof(output), sc);

      assert(!aliceStack.getNack());
      aliceStack.provideCompartmentId(sc, compartmentId);
    }

  }

  return 0;
}
