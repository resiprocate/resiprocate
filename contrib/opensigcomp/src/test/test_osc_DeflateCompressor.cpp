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
#include "DeflateCompressor.h"
#include "TestList.h"

#include "Compartment.h"
#include "State.h"
#include "Types.h"
#include "Udvm.h"
#include "StateChanges.h"
#include "StateHandler.h"
#include "NackCodes.h"

#include "inflate.h"

//static osc::byte_t message1[] = "abbabbabbfoooooo";

static osc::byte_t register_msg[] = 
  "REGISTER sip:alpaca.estacado.net SIP/2.0\r\n"
  "Via: SIP/2.0/UDP 172.17.1.247:2057;branch=z9hG4bK-9qfkur6jl59c;rport\r\n"
  "From: \"Adam@alpaca\" <sip:2145550491@alpaca.estacado.net>;"
    "tag=l67kjdzmv8\r\n"
  "To: \"Adam@alpaca\" <sip:2145550491@alpaca.estacado.net>\r\n"
  "Call-ID: 3c26700bd6d8-12dgq55g3po6@widget3000\r\n"
  "CSeq: 12574 REGISTER\r\n"
  "Max-Forwards: 70\r\n"
  "Contact: <sip:2145550491@172.17.1.247:2057;line=l4lzrndz>;q=1.0"
    ";+sip.instance=\"<urn:uuid:4c196bc5-093f-426d-bf21-bee29bad7e81>\""
    ";audio;mobility=\"fixed\";duplex=\"full\";description=\"widget3000\""
    ";actor=\"principal\";events=\"dialog\""
    ";methods=\"INVITE,ACK,CANCEL,BYE,REFER,OPTIONS,NOTIFY,SUBSCRIBE,"
               "PRACK,MESSAGE,INFO\"\r\n"
  "Supported: gruu\r\n"
  "Allow-Events: dialog\r\n"
  "Expires: 3600\r\n"
  "Content-Length: 0\r\n\r\n";

static osc::byte_t invite_msg[] = 
  "INVITE sip:alpaca.estacado.net SIP/2.0\r\n"
  "From: \"Adam@alpaca\" <sip:2145550491@alpaca.estacado.net>;"
    "tag=l67kjdzmv8\r\n"
  "To: \"Robert@alpaca\" <sip:2145550494@alpaca.estacado.net>\r\n"
  "Call-ID: 3c26700bd6d8-d8gl28tlgnsp@widget3000\r\n"
  "CSeq: 12574 INVITE\r\n"
  "Via: SIP/2.0/UDP 172.17.1.247:2057;branch=z9hG4bK-dkg035kf09s4;rport\r\n"
  "Max-Forwards: 70\r\n"
  "Contact: <sip:2145550491@172.17.1.247:2057;line=l4lzrndz>;q=1.0"
    ";+sip.instance=\"<urn:uuid:4c196bc5-093f-426d-bf21-bee29bad7e81>\""
    ";audio;mobility=\"fixed\";duplex=\"full\";description=\"widget3000\""
    ";actor=\"principal\";events=\"dialog\""
    ";methods=\"INVITE,ACK,CANCEL,BYE,REFER,OPTIONS,NOTIFY,SUBSCRIBE,"
               "PRACK,MESSAGE,INFO\"\r\n"
  "Supported: gruu\r\n"
  "Allow-Events: dialog\r\n"
  "Expires: 3600\r\n"
  "Content-Type: application/sdp\r\n"
  "Content-Length: 150\r\n\r\n"
  "v=0\r\n"
  "o=+12003004005 2656980195 1 IN IP4 72.1.129.109\r\n"
  "s=-\r\n"
  "c=IN IP4 72.1.129.109\r\n"
  "t=0 0\r\n"
  "m=audio 5062 RTP/AVP 96\r\n"
  "a=rtpmap:96 speex/8000\r\n"
  "a=ptime:20\r\n" ;

bool compressMessageTwice(const osc::byte_t *message,
                           osc::Compressor *dc,
                           osc::StateHandler &sh,
                           osc::Compartment &udvm_compartment,
                           osc::Compartment &compressor_compartment)
{
  osc::SigcompMessage *sm = 0;

//  osc::byte_t decoded[1+strlen((char *)message) * 4];
  osc::byte_t decoded[65536];
  size_t decodeLength;

  sm = dc->compress(compressor_compartment, message, 
                    1+strlen((char *)message), false);
  TEST_ASSERT(sm);
  std::cout << "    - Compressed " << 1+strlen((char *)message) << " bytes to " 
            << sm->getInputLength() << " bytes." << std::endl;

  size_t firstCompressedSize = sm->getInputLength();

  {
    osc::Udvm udvm(sh, 16384);
    osc::BitBuffer outputBuffer(decoded, sizeof(decoded));

    udvm.init(*sm);
    udvm.setOutputBuffer(outputBuffer);
    udvm.execute();

    if (udvm.isFailed())
    {
      std::cout << "  UDVM Status: " << osc::s_nackCode[udvm.getNackCode()]
                << " at " << udvm.getNackFile() << ":" << udvm.getNackLine() 
                << std::endl;
      std::cout << *(udvm.getNack(*sm)) << std::endl;
    }
    TEST_ASSERT(!udvm.isFailed());

    decodeLength = outputBuffer.getBufferSize()/8;
    TEST_ASSERT_EQUAL(decodeLength, 1+strlen((char *)message));

    TEST_ASSERT_EQUAL_BUFFERS(decoded, message, decodeLength);

    osc::StateChanges *sc = udvm.getProposedStates();

    // Mark the remote state as acknowledged
    compressor_compartment.ackRemoteState(sc->getState(0)->getStateId());

    // Accept the proposed state
    sh.processChanges(*sc, udvm_compartment);
  }

  // Now, try to compress the same input again.
  memset(decoded, 0, sizeof(decoded));
  delete(sm);
  sm = dc->compress(compressor_compartment, message, 
                    1+strlen((char *)message), false);
  std::cout << "    - Compressed " << 1+strlen((char *)message) << " bytes to " 
            << sm->getInputLength() << " bytes." << std::endl;

  TEST_ASSERT (sm->getInputLength() < firstCompressedSize);

  {
    osc::Udvm udvm(sh, 16384);
    osc::BitBuffer outputBuffer(decoded, sizeof(decoded));

    udvm.init(*sm);
    udvm.setOutputBuffer(outputBuffer);
    udvm.execute();

    if (udvm.isFailed())
    {
      std::cout << "  UDVM Status: " << osc::s_nackCode[udvm.getNackCode()]
                << " at " << udvm.getNackFile() << ":" << udvm.getNackLine() 
                << std::endl;
      std::cout << *(udvm.getNack(*sm)) << std::endl;
    }
    TEST_ASSERT(!udvm.isFailed());

    osc::StateChanges *sc = udvm.getProposedStates();

    // Mark the remote state as acknowledged
    compressor_compartment.ackRemoteState(sc->getState(0)->getStateId());

    decodeLength = outputBuffer.getBufferSize()/8;
    TEST_ASSERT_EQUAL(decodeLength, 1+strlen((char *)message));

    TEST_ASSERT_EQUAL_BUFFERS(decoded, message, decodeLength);

    // Accept the proposed state
    sh.processChanges(*sc, udvm_compartment);
  }
  delete(sm);
  return true;
}

bool test_osc_DeflateCompressor()
{
  osc::byte_t id = 4;
  osc::StateHandler sh(2048, 64, 8192, 2);
  osc::compartment_id_t cid(&id, sizeof(id));
  osc::Compartment compartment(cid, 8192);
  sh.useSipDictionary();

  osc::SigcompMessage *sm = 0;

  osc::Compressor *dc = new osc::DeflateCompressor(sh);
  osc::byte_t decoded[sizeof(register_msg) * 4];
  size_t decodeLength;

  //////////////////////////////////////////////////////////////////////
  // Decompress the message manually
  sm = dc->compress(compartment, register_msg, 
                    1+strlen((char *)register_msg), false);

  osc::BitBuffer &bb = sm->getBitBuffer();

  // Discard first 16 bits (8 byte capabilities, 1 bit sip
  // dictionary flag, 3 bit buffer size code, 4 bit serial #)
  bb.readMsbFirst(16);

  decodeLength = inflate(bb, decoded, sizeof(decoded));  
  TEST_ASSERT_EQUAL(decodeLength, 1+strlen((char *)register_msg));
  TEST_ASSERT_EQUAL_BUFFERS(decoded, register_msg, decodeLength);
  delete(sm);
  sm = 0;

  //////////////////////////////////////////////////////////////////////
  // Decompress the message using the UDVM

  bool status = compressMessageTwice(register_msg,
                                     dc, sh, compartment, compartment);

  delete(dc);

  return status;
}


/*
  This tests that growing the deflate buffer works properly
  after the remote endpoint indicates an increase in the size
  of SMS or DMS available.
*/
bool test_osc_DeflateCompressor_regression1()
{
  osc::byte_t id = 4;
  osc::StateHandler compressor_sh(8192, 64, 8192, 2, 4,
                                  2048, 16, 8192, 2);

  osc::StateHandler udvm_sh(8192, 64, 8192);

  compressor_sh.useSipDictionary();
  udvm_sh.useSipDictionary();

  osc::compartment_id_t cid(&id, sizeof(id));

  osc::Compartment udvm_compartment(cid, 8192);
  osc::Compartment compressor_compartment(cid, 8192);

  osc::Compressor *dc = new osc::DeflateCompressor(compressor_sh);

  bool status = compressMessageTwice(register_msg,
                                     dc, udvm_sh, 
                                     udvm_compartment,
                                     compressor_compartment);

  if (!status) { return status; }

  // Now, mark the remote side has having significantly better
  // capabilities; this should resize the compression buffer.
  compressor_compartment.setCpbDmsSms(0x63);

  status = compressMessageTwice(invite_msg, dc, udvm_sh, 
                                 udvm_compartment,
                                 compressor_compartment);

  delete dc;

  if (!status) { return status; }

  return true;
}



static bool DeflateCompressorTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_DeflateCompressor,
                                     "test_osc_DeflateCompressor") &&
  osc::TestList::instance()->addTest(test_osc_DeflateCompressor_regression1,
                                     "test_osc_DeflateCompressor_regression1");
