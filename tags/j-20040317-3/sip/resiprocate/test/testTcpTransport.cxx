#include <iostream>
#include <memory>

#include "resiprocate/TcpTransport.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/ConnectionMap.hxx"
#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/Preparse.hxx"
#include "resiprocate/TransportMessage.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/test/TestSupport.hxx"
#include "resiprocate/test/Resolver.hxx"

#include <signal.h>

using namespace resip;
using namespace std;


char *onemsg = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfirst\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=ssecond\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sthird\r\n"
                "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfourth\r\n"
                "Max-Forwards: 7\r\n"
                "To: Speedy <sip:speedy@biloxi.com>\r\n"
                "From: Speedy <sip:speedy@biloxi.com>;tag=88888\r\n"
                "Call-ID: 88888@8888\r\n"
                "CSeq: 6281 REGISTER\r\n"
                "Contact: <sip:speedy@192.0.2.4>\r\n"
                "Contact: <sip:qoq@192.0.2.4>\r\n"
                "Expires: 2700\r\n"
                "Content-Length: 0\r\n\r\n");

char *longWithBody = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                      "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfirst\r\n"
                      "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=ssecond\r\n"
                      "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sthird\r\n"
                      "Via: SIP/2.0/UDP speedyspc.biloxi.com:5060;branch=sfourth\r\n"
                      "Max-Forwards: 7\r\n"
                      "To: Speedy <sip:speedy@biloxi.com>\r\n"
                      "From: Speedy <sip:speedy@biloxi.com>;tag=88888\r\n"
                      "Call-ID: 88888@8888\r\n"
                      "CSeq: 6281 REGISTER\r\n"
                      "Contact: <sip:speedy@192.0.2.4>\r\n"
                      "Contact: <sip:qoq@192.0.2.4>\r\n"
                      "Expires: 2700\r\n"
                      "Content-Type: text/plain\r\n"
                      "Content-Length: 500\r\n"
                      "\r\n"
                      "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkl");

char *smallMessage = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                      "Content-Length: 0\r\n\r\n");

char *smallMessageWithBody = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 17\r\n"
                              "\r\n"
                              "afffffefffffffffg");

char *smallMessageWithLargeBody = ("INVITE sip:registrar.ixolib.com SIP/2.0\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: 128\r\n"
                                   "\r\n"
                                   "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssf");

int 
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::DEBUG, argv[0]);
   
   cerr << "*!Connection::Connection & preparser tests!*" << endl;
   
   cerr << "Message in two chunks" << endl;   
#if 1
   {
      int chunk = strlen(onemsg) * 2 / 3;
         
      char* buffer = new char[chunk];
      memcpy(buffer, onemsg, chunk);
         
      Preparse preparse;
      SipMessage msg;
      cerr << "Preparsing:[";
      cerr.write(buffer, chunk);
      cerr << "]" << endl;
      if (preparse.process(msg, buffer, chunk) != 0)
      {
         assert(0);
      }
         
      assert(preparse.isFragmented());
      assert(!preparse.isHeadersComplete());
      assert(preparse.isDataAssigned());

      msg.addBuffer(buffer);
         
      char* partialHeader = new char[(chunk - preparse.nDiscardOffset()) + chunk];
      memcpy(partialHeader, buffer + preparse.nDiscardOffset(), chunk - preparse.nDiscardOffset());
      
      cerr.write(partialHeader, chunk - preparse.nDiscardOffset());
      cerr << endl << endl;

      memcpy(partialHeader + chunk - preparse.nDiscardOffset(), onemsg + preparse.nBytesUsed(),
             strlen(onemsg) - preparse.nDiscardOffset());
         
      cerr << "Preparsing:[";
      cerr.write(partialHeader, strlen(onemsg) - preparse.nDiscardOffset());
      cerr << "]" << endl;
      
      if (preparse.process(msg, partialHeader, chunk - preparse.nDiscardOffset() + strlen(onemsg) - preparse.nDiscardOffset()) != 0)
      {
         assert(0);
      }
         
      assert(!preparse.isFragmented());
      assert(preparse.isHeadersComplete());
      assert(preparse.isDataAssigned());

      msg.addBuffer(partialHeader);

      cerr << msg << endl;
   }

   {
      Data msgData(smallMessage);
      Fifo<Message> fifo;
      Connection conn;
      
      std::pair<char* const, size_t> writePair = conn.getWriteBuffer();

      memcpy(writePair.first, msgData.data(), msgData.size());
      
      conn.process(msgData.size(), fifo);
      
      assert(fifo.messageAvailable());
      SipMessage* msg = dynamic_cast<SipMessage*>(fifo.getNext());
      assert(msg);
      Data buf;
      {
         DataStream s(buf);
         s << *msg;
      }
      assert(buf == msgData);
   }

   {
      Data msgData(smallMessageWithBody);
      Fifo<Message> fifo;
      Connection conn;
      
      std::pair<char* const, size_t> writePair = conn.getWriteBuffer();

      memcpy(writePair.first, msgData.data(), msgData.size());
      
      conn.process(msgData.size(), fifo);
      
      assert(fifo.messageAvailable());
      SipMessage* msg = dynamic_cast<SipMessage*>(fifo.getNext());
      cerr << *msg << endl;
      assert(msg);
      Data buf;
      {
         DataStream s(buf);
         s << *msg;
      }
      assert(buf == msgData);
   }

   {
      Data msgData(smallMessageWithLargeBody);
      Fifo<Message> fifo;
      Connection conn;
      
      size_t pos = 0;
      while (pos < msgData.size())
      {
         std::pair<char* const, size_t> writePair = conn.getWriteBuffer();

         size_t writeSize = min(msgData.size() - pos, 
                                min((size_t)100, writePair.second));
         
         memcpy(writePair.first, msgData.data() + pos, writeSize);
         pos += writeSize;
         conn.process(writeSize, fifo);
      }
      assert(fifo.messageAvailable());
      SipMessage* msg = dynamic_cast<SipMessage*>(fifo.getNext());
      cerr << *msg << endl;
      assert(msg);
      Data buf;
      {
         DataStream s(buf);
         s << *msg;
      }
      assert(buf == msgData);
   }

   {
      Data msgData(onemsg);
      Fifo<Message> fifo;
      Connection conn;
      
      size_t pos = 0;
      while (pos < msgData.size())
      {
         std::pair<char* const, size_t> writePair = conn.getWriteBuffer();

         size_t writeSize = min(msgData.size() - pos, 
                                min((size_t)100, writePair.second));
         
         memcpy(writePair.first, msgData.data() + pos, writeSize);
         pos += writeSize;
         conn.process(writeSize, fifo);
      }
      assert(fifo.messageAvailable());
      SipMessage* msg = dynamic_cast<SipMessage*>(fifo.getNext());

      assert(msg);
      cerr << *msg << endl;

      // remove the received parameter to allow strict matching
      msg->header(h_Vias).front().remove(p_received);

      auto_ptr<SipMessage> shouldMatch(TestSupport::makeMessage(msgData));
      Data buf;
      {
         DataStream s(buf);
         s << *msg;
      }
      Data otherBuf;
      {
         DataStream s(otherBuf);
         s << *shouldMatch;
      }

      cerr << "!! " << buf << endl;
      cerr << "!! " << otherBuf << endl;
      assert(buf == otherBuf);
   }

   {
      Data msgData(longWithBody);
      Fifo<Message> fifo;
      Connection conn;
      
      size_t pos = 0;
      while (pos < msgData.size())
      {
         std::pair<char* const, size_t> writePair = conn.getWriteBuffer();

         size_t writeSize = min(msgData.size() - pos, 
                                min((size_t)100, writePair.second));
         
         memcpy(writePair.first, msgData.data() + pos, writeSize);
         pos += writeSize;
         conn.process(writeSize, fifo);
      }
      assert(fifo.messageAvailable());
      SipMessage* msg = dynamic_cast<SipMessage*>(fifo.getNext());
      cerr << *msg << endl;
      assert(msg);

      auto_ptr<SipMessage> shouldMatch(TestSupport::makeMessage(msgData));
      Data buf;
      {
         DataStream s(buf);
         s << *msg;
      }
      Data otherBuf;
      {
         DataStream s(otherBuf);
         s << *shouldMatch;
      }

      assert(buf == otherBuf);
   }

   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }

   cerr << "*!TcpConnection tests!*" << endl;
   {

      Data tryToSendThis(onemsg);
      Data transactionId = "2347sy8";

      Fifo<Message> srcFifo;
      Fifo<Message> destFifo;

      Data localHost = Resolver::getHostName();
      Resolver srcIp(localHost, 5070, TCP);
      Resolver destIp(localHost, 5080, TCP);
      
      TcpTransport src(localHost, 5070, "eth0", srcFifo);
      TcpTransport dest(localHost, 5080, "eth0", destFifo);

      src.send(destIp.mNextHops.front(), tryToSendThis, transactionId);

      int count = 0;
      while (!destFifo.messageAvailable() && count < 50)
      {
         FdSet srcFdset;
         src.buildFdSet(srcFdset);
         {
            int err = srcFdset.selectMilliSeconds(5);
            assert (err != -1);
         }
         src.process(srcFdset);

         FdSet destFdset;
         dest.buildFdSet(destFdset);
         {
            int err = destFdset.selectMilliSeconds(5);
            assert (err != -1);
         }
         dest.process(destFdset);
         count++;
      }
      if (count == 50)
      {
         cerr << "test failed" << endl;
         exit(-1);
      }
      
      cerr << "recieved message" << endl;

      Message* msg = destFifo.getNext();
      SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
      assert(sipMsg);
      cerr << *sipMsg << endl;

      cerr << "getting ok" << endl;
      assert(srcFifo.messageAvailable());
      msg = srcFifo.getNext();
      assert(msg);
      TransportMessage* okMsg = dynamic_cast<TransportMessage*>(msg);
      cerr << *okMsg << endl;

      delete sipMsg;
      delete okMsg;
   }
#endif
   {
      cerr << "second" << endl;
      Data tryToSendThis(longWithBody);
      Data transactionId = "2347sy8";

      Fifo<Message> srcFifo;
      Fifo<Message> destFifo;

      int srcPort = 5070;
      int dstPort = 5080;

      Data localHost = Resolver::getHostName();
      Resolver srcIp(localHost, srcPort, TCP);
      Resolver destIp(localHost, dstPort, TCP);
      
      TcpTransport src(localHost, srcPort, "eth0", srcFifo);
      TcpTransport dest(localHost, dstPort, "eth0", destFifo);

      src.send(destIp.mNextHops.front(), tryToSendThis, transactionId);

      int count = 0;
      while (!destFifo.messageAvailable() && count < 50)
      {
         FdSet srcFdset;
         src.buildFdSet(srcFdset);
         {
            int err = srcFdset.selectMilliSeconds(5);
            assert (err != -1);
         }
         src.process(srcFdset);

         FdSet destFdset;
         dest.buildFdSet(destFdset);
         {
            int err = destFdset.selectMilliSeconds(5);
            assert (err != -1);
         }
         dest.process(destFdset);
         count++;
      }

      {
         Message* msg = destFifo.getNext();
         SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
         assert(sipMsg);
         cerr << *sipMsg << endl;
         
         cerr << "getting ok" << endl;
         assert(srcFifo.messageAvailable());
         msg = srcFifo.getNext();
         assert(msg);
         TransportMessage* okMsg = dynamic_cast<TransportMessage*>(msg);
         cerr << *okMsg << endl;
         
         Data reply(smallMessageWithLargeBody);
         dest.send(sipMsg->getSource(), reply, transactionId);

         delete sipMsg;
         delete okMsg;
      }
      

      while (!srcFifo.messageAvailable() && count < 50)
      {

         FdSet destFdset;
         dest.buildFdSet(destFdset);
         {
            int err = destFdset.selectMilliSeconds(5);
            assert (err != -1);
         }
         dest.process(destFdset);

         FdSet srcFdset;
         src.buildFdSet(srcFdset);
         {
            int err = srcFdset.selectMilliSeconds(5);
            assert (err != -1);
         }
         src.process(srcFdset);

         count++;
      }
      
      if (count == 50)
      {
         cerr << "test failed" << endl;
         exit(-1);
      }
      
      cerr << "recieved message" << endl;
      {
         Message* msg = srcFifo.getNext();
         SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
         assert(sipMsg);
         cerr << *sipMsg << endl;
         
         cerr << "getting ok" << endl;
         assert(destFifo.messageAvailable());
         msg = destFifo.getNext();
         assert(msg);
         TransportMessage* okMsg = dynamic_cast<TransportMessage*>(msg);
         cerr << *okMsg << endl;
      }

   }

   cerr << endl << "All OK" << endl;
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
