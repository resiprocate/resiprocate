#include "resiprocate/sipstack/SipStack.hxx"
#include "resiprocate/sipstack/Transport.hxx"
#include "resiprocate/sipstack/Uri.hxx"
#include "resiprocate/sipstack/Helper.hxx"
#include "resiprocate/sipstack/UdpTransport.hxx"

#include "resiprocate/util/Logger.hxx"
#include "resiprocate/util/DataStream.hxx"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP
#define CRLF "\r\n"


SipStack* client=0;
Fifo<Message> received;
UdpTransport* server=0;

void
doit(int serverResponse, int expectedRetrans, int expectedClientResponse);


int
main(int argc, char *argv[])
{
    Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

    InfoLog( << "Starting up, making stack");

    client = new SipStack();
    client->addTransport(Transport::UDP, 5060);

    server = new UdpTransport("localhost", 5070, "default", received);

    
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
    doit(400, 1, 400);
    doit(0, 10, 408);
    
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
    Data encoded(2048, true);
    DataStream strm(encoded);
    reg->encode(strm);
    strm.flush();

    client->send(*reg);

    Data encodedResponse(2048, true);
    
    int count=0;
    while (1)
    {
       struct timeval tv;
       fd_set fdReadSet;
       int fdSetSize = 0;
       
       // Init the fd_set for the select()
       FD_ZERO(&fdReadSet);
       
       fdSetSize = 0;
       client->buildFdSet(&fdReadSet, &fdSetSize);
       server->buildFdSet(&fdReadSet, &fdSetSize);
       
       // block on fdset
       tv.tv_sec = 0;
       tv.tv_usec = 1000 * client->getTimeTillNextProcess();

       // get the sip message that we just sent and process it
       int err = select(fdSetSize, &fdReadSet, 0, 0, &tv);
       assert (err != -1);
       
       client->process(&fdReadSet);
       server->process(&fdReadSet);
       
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
             sa.sin_port = htons(5060);
            
             server->send(sa, encodedResponse);
          }
         
          delete sip;
            
       }

       usleep(20);
    }
}
