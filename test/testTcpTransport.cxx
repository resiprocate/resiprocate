#include <iostream>

#include "sip2/sipstack/TcpTransport.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/sipstack/ConnectionMap.hxx"
#include "sip2/util/Fifo.hxx"
#include "sip2/sipstack/Preparse.hxx"
#include "sip2/sipstack/Helper.hxx"


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

int main()
{
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
   cerr << "Whole message in one chunk" << endl;
   {
      Data msgData(onemsg);
      Preparse preparse;
      Fifo<Message> fifo;
      ConnectionMap::Connection conn;
      conn.allocateBuffer(1024);
      
      assert(conn.mBytesRead == 0);
      assert(conn.mBufferSize == 1024);
      memcpy(conn.mBuffer + conn.mBytesRead, msgData.data(), msgData.size());
      
      conn.process(msgData.size(), fifo, preparse, 1024);
      
      if (fifo.messageAvailable())
      {
         SipMessage* msg = dynamic_cast<SipMessage*>(fifo.getNext());
         assert(msg);
         cerr << *msg << endl;
      }
      else
      {
         cerr << "Message was not inserted into fifo" << endl;
      }
   }

   cerr << "Message in two chunks" << endl;   
   {
      {
         int chunk = strlen(onemsg) / 2;
         
         char* buffer = new char[chunk];
         
         Preparse preparse;
         SipMessage msg;
         int k;
         

         size_t used = 0;
         size_t discard = 0;
         Preparse::Action status = PreparseConst::stNone;
         preparse.process(msg, buffer, chunk, 0, used, discard, status);
         
         //assert(status == PreparseState::fragment);
         //cerr << "bytes used: " << k << ", message size: " << msgData.size() << endl;
         //assert(k == msgData.size());
         cerr << msg << endl;
      }
      return 0;
      Data msgData(onemsg);
      Preparse preparse;
      Fifo<Message> fifo;
      ConnectionMap::Connection conn;

      conn.allocateBuffer(500);
      assert(conn.mBytesRead == 0);
      assert(conn.mBufferSize == 500);
      memcpy(conn.mBuffer + conn.mBytesRead, msgData.data(), 500);
      
      conn.process(500, fifo, preparse, 500);

      conn.allocateBuffer(500);
      memcpy(conn.mBuffer + conn.mBytesRead + 500, msgData.data() + 500, msgData.size() - 500);
      
      conn.process(msgData.size() - 500, fifo, preparse, 500);
      
      if (fifo.messageAvailable())
      {
         SipMessage* msg = dynamic_cast<SipMessage*>(fifo.getNext());
         assert(msg);
         cerr << *msg << endl;
      }
      else
      {
         cerr << "Message was not inserted into fifo" << endl;
      }
   }
}
