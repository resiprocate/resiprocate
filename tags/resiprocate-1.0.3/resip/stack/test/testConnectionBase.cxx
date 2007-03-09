#include <iostream>
#include <memory>

#include "rutil/DataStream.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ConnectionBase.hxx"

#include "resip/stack/Helper.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/test/TestSupport.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/UnknownHeaderType.hxx"
#include "resip/stack/UnknownParameterType.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Random.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
#define CRLF "\r\n"

class FakeTransport :  public Transport
{
   public:
      FakeTransport(Fifo<TransactionMessage>& rxFifo, 
                    int portNum, 
                    IpVersion version, 
                    const Data& interfaceObj,
                    const Data& tlsDomain = Data::Empty) :
         Transport(rxFifo, portNum, version, interfaceObj, tlsDomain)
      {}

      virtual TransportType transport() const { return TCP; }
      virtual bool isFinished() const { assert(0); return true; }
      virtual void process(FdSet& fdset) { assert(0); }
      virtual void buildFdSet( FdSet& fdset) { assert(0); }

      virtual bool isReliable() const { assert(0); return true; }
      virtual bool shareStackProcessAndSelect() const { assert(0); return true;}
      virtual void startOwnProcessing() { assert(0); }
      virtual bool hasDataToSend() const { assert(0); return false; }
      virtual unsigned int getFifoSize() const { assert(0); return 0; }

      virtual void transmit(const Tuple& dest, const Data& pdata, const Data& tid) { assert(0); }
};

class TestConnection : public ConnectionBase
{
   public:
      TestConnection(const Tuple& who, const Data& bytes, Fifo<TransactionMessage>& fifo) : 
         ConnectionBase(who),
         mTestStream(bytes),
         mStreamPos(0),
         mRxFifo(fifo)
      {}
      
      bool read(unsigned int minChunkSize, unsigned int maxChunkSize)
      {
         unsigned int chunk = Random::getRandom() % maxChunkSize;
         chunk = chunk < minChunkSize ? minChunkSize : chunk;
         chunk = chunk > maxChunkSize ? maxChunkSize : chunk;
         chunk = (chunk > (mTestStream.size() - mStreamPos) ) ? (mTestStream.size() - mStreamPos) : chunk;
         assert(chunk > 0);
         std::pair<char*, size_t> writePair = getWriteBuffer();
         memcpy(writePair.first, mTestStream.data() + mStreamPos, chunk);
         mStreamPos += chunk;
         assert(mStreamPos <= mTestStream.size());

         preparseNewBytes(chunk, mRxFifo);
         return mStreamPos != mTestStream.size();
      }
      
   private:
      Data mTestStream;
      unsigned int mStreamPos;
      Fifo<TransactionMessage>& mRxFifo;
};

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Info, argv[0]);

   Data bytes("INVITE sip:192.168.2.92:5100;q=1 SIP/2.0\r\n"
              "To: <sip:yiwen_AT_meet2talk.com@whistler.gloo.net>\r\n"
              "From: Jason Fischl<sip:jason_AT_meet2talk.com@whistler.gloo.net>;tag=ba1aee2d\r\n"
              "Via: SIP/2.0/UDP 192.168.2.220:5060;branch=z9hG4bK-c87542-da4d3e6a.0-1--c87542-;rport=5060;received=192.168.2.220;stid=579667358\r\n"
              "Via: SIP/2.0/UDP 192.168.2.15:5100;branch=z9hG4bK-c87542-579667358-1--c87542-;rport=5100;received=192.168.2.15\r\n"
              "Call-ID: 6c64b42fce01b007\r\n"
              "CSeq: 2 INVITE\r\n"
              "Route: <sip:proxy@192.168.2.220:5060;lr>\r\n"
              "Contact: <sip:192.168.2.15:5100>\r\n"
              "Content-Length: 0\r\n"
              "\r\n"
              "\r\n\r\n" //keepalive
              "INVITE sip:192.168.2.92:5100;q=1 SIP/2.0\r\n"
              "To: <sip:yiwen_AT_meet2talk.com@whistler.gloo.net>\r\n"
              "From: Jason Fischl<sip:jason_AT_meet2talk.com@whistler.gloo.net>;tag=ba1aee2d\r\n"
              "Via: SIP/2.0/UDP 192.168.2.220:5060;branch=z9hG4bK-c87542-da4d3e6a.0-1--c87542-;rport=5060;received=192.168.2.220;stid=579667358\r\n"
              "Via: SIP/2.0/UDP 192.168.2.15:5100;branch=z9hG4bK-c87542-579667358-1--c87542-;rport=5100;received=192.168.2.15\r\n"
              "Call-ID: 6c64b42fce01b007\r\n"
              "CSeq: 2 INVITE\r\n"
              "Record-Route: <sip:proxy@192.168.2.220:5060;lr>\r\n"
              "Contact: <sip:192.168.2.15:5100>\r\n"
              "Max-Forwards: 69\r\n"
              "Content-Type: application/sdp\r\n"
              "Content-Length: 307\r\n"
              "\r\n"
              "v=0\r\n"
              "o=M2TUA 1589993278 1032390928 IN IP4 192.168.2.15\r\n"
              "s=-\r\n"
              "c=IN IP4 192.168.2.15\r\n"
              "t=0 0\r\n"
              "m=audio 9000 RTP/AVP 103 97 100 101 0 8 102\r\n"
              "a=rtpmap:103 ISAC/16000\r\n"
              "a=rtpmap:97 IPCMWB/16000\r\n"
              "a=rtpmap:100 EG711U/8000\r\n"
              "a=rtpmap:101 EG711A/8000\r\n"
              "a=rtpmap:0 PCMU/8000\r\n"
              "a=rtpmap:8 PCMA/8000\r\n"
              "a=rtpmap:102 iLBC/8000\r\n");
   
   Fifo<TransactionMessage> testRxFifo;
   FakeTransport fake(testRxFifo, 5060, V4, Data::Empty);
   Tuple who(fake.getTuple());
   who.transport = &fake;
  
   int chunkRange = 700;
   unsigned int runs = 100;

   for (unsigned int i=0; i < runs; i++)
   {
      TestConnection cBase(who, bytes, testRxFifo);
      int minChunk = Random::getRandom() % chunkRange;
      int maxChunk = Random::getRandom() % chunkRange;
      if (maxChunk < minChunk) swap(maxChunk, minChunk);
      while(cBase.read(minChunk, maxChunk));      
   }
   assert(testRxFifo.size() == runs * 3);

   cerr << "\nTEST OK" << endl;
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
