#include "resiprocate/sipstack/SipStack.hxx"
#include "resiprocate/sipstack/Transport.hxx"
#include "resiprocate/sipstack/Uri.hxx"
#include "resiprocate/sipstack/Helper.hxx"
#include "resiprocate/sipstack/test/TestSupport.hxx"
#include "resiprocate/util/Logger.hxx"
#include "resiprocate/util/DataStream.hxx"

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

   FdSet fdset;
   client->process(fdset);

   // read the message off the stack
   Data fromStack;
   getFromWire(fromStack);

   InfoLog(<< "Received from wire " << fromStack);

   SipMessage* message = TestSupport::makeMessage(fromStack);
      
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
      client->process(fdset);
      usleep(20);
   }
}
