#include "sipstack/SipStack.hxx"
#include "sipstack/Transport.hxx"
#include "sipstack/Uri.hxx"
#include "sipstack/Helper.hxx"
#include "sipstack/TestTransport.hxx"

#include "util/Logger.hxx"
#include "util/DataStream.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP
#define CRLF "\r\n"


SipStack* client=0;
Fifo<Message> received;

void
doit(int serverResponse, int expectedRetrans, int expectedClientResponse);


int
main(int argc, char *argv[])
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

   DebugLog( << "Starting up, making stack");
   InfoLog( << "Starting up, making stack");

   client = new SipStack();
   client->addTransport(Transport::UDP, 5070);

   // Test 1: 
   // client sends a reg, server does nothing, client should retransmit 10
   // times, client should receive 408

   // Test 2: 
   // client sends a reg, server sends 100, client should retransmit 7 times
   // client should receive 408
    
   // Test 3:
   // client sends a reg, server sends 200, client shouldn't retransmit at all
   // client should receive 200

   // Test 4:
   // client sends a reg, server sends 400, client shouldn't retransmit at all
   // client should receive 400

   //doit(100, 7, 408);
   doit(200, 1, 200);
   //    doit(400, 1, 400);
   //doit(0, 10, 408);
    
   return 0;
}


void
doit(int serverResponse, int expectedRetrans, int expectedClientResponse)
{
   InfoLog (<< "Running test: " << serverResponse << " " << expectedRetrans << " " << expectedClientResponse);
   
   NameAddr me;
   me.uri().host() = "localhost";
   me.uri().port() = 5070;
   SipMessage* reg = Helper::makeRegister(me, me);
   InfoLog (<< "Sending: " << *reg);

   Data encoded(2048, true);
   DataStream strm(encoded);
   reg->encode(strm);
   strm.flush();


    
   client->send(*reg);   // send message down the stack


   fd_set fdReadSet;
       
   // Init the fd_set for the select()
   FD_ZERO(&fdReadSet);
       
   client->process(&fdReadSet);


   // read the message off the stack
   Data fromStack;
   getFromWire(fromStack);

   InfoLog(<< "Received from wire " << fromStack);

   SipMessage* message = Helper::makeMessage(fromStack);
      
   // send the response message

   SipMessage* response = Helper::makeResponse(*message, 100);
   InfoLog (<< "sending to wire = " << endl << *response);
            
   Data encodedResponse(2048, true);

   DataStream estrm(encodedResponse);
   response->encode(estrm);
   estrm.flush();

   //    sendToWire(encodedResponse);

   while (1)
   {
      client->process();
      usleep(20);
   }

#if 0
   int count=0;
   while (1)
   {
      fd_set fdReadSet;
      int fdSetSize = 0;
       
      // Init the fd_set for the select()
      FD_ZERO(&fdReadSet);
       
      client->process(&fdReadSet);
       
      SipMessage* sipMessage = client->receive();
            
      if (sipMessage) 
      {
         InfoLog( << "got message (client)" << *sipMessage);
         assert(sipMessage->isResponse());
         assert(sipMessage->header(h_StatusLine).responseCode() == expectedClientResponse);
         assert(count == expectedRetrans);
         return;
      }

      client->process(&fdReadSet);
      if (received.messageAvailable())
      {
         count++;
          
         SipMessage* sip = dynamic_cast<SipMessage*>(received.getNext());
         assert(sip);
         InfoLog( << "got message (server)" << *sip);

         if (serverResponse)
         {
            SipMessage* response = Helper::makeResponse(*sip, serverResponse);
            DebugLog (<< "server sending response = " << endl << *response);
            
            DataStream strm(encodedResponse);
            response->encode(strm);
            strm.flush();

            // create address to send to
            struct sockaddr_in sa;
            
            sa.sin_family = PF_INET;
            sa.sin_addr.s_addr = inet_addr("127.0.0.1");
            sa.sin_port = htons(5070);
            
         }
         
         delete sip;
            
      }

      usleep(20);
   }
#endif
}
