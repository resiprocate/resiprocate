#include <iostream>

#include "sip2/sipstack/TcpTransport.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/sipstack/ConnectionMap.hxx"
#include "sip2/util/Fifo.hxx"
#include "sip2/sipstack/Preparse.hxx"
#include "sip2/util/DataStream.hxx"
#include "sip2/util/Log.hxx"
#include "sip2/sipstack/test/TestSupport.hxx"

using namespace Vocal2;
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

char *smallMessageWithLargeBody = ("REGISTER sip:registrar.ixolib.com SIP/2.0\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: 128\r\n"
                                   "\r\n"
                                   "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssf");

int 
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::DEBUG, argv[0]);
/*
   {
      Data msgData(onemsg);
      
      Preparse preparse;
      SipMessage msg;
      int k;

      PreparseState::TransportAction status;
      preparse.process(msg, msgData.data(), msgData.size(), k, status);
      
      assert(status == PreparseState::headersComplete);
      cerr << "bytes used: " << k << ", message size: " << msgData.size() << endl;
      assert(k == msgData.size());
      cerr << msg << endl;
   }
   return 0;
*/
   
   cerr << "Message in two chunks" << endl;   
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
      ConnectionMap::Connection conn;
      
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
      ConnectionMap::Connection conn;
      
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
      ConnectionMap::Connection conn;
      
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
      ConnectionMap::Connection conn;
      
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
   {
      Data msgData(longWithBody);
      Fifo<Message> fifo;
      ConnectionMap::Connection conn;
      
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
}
