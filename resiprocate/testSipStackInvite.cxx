#include <sipstack/SipStack.hxx>
#include <sipstack/Transport.hxx>
#include <sipstack/Uri.hxx>
#include <util/Logger.hxx>

#include "sipstack/Helper.hxx"
#include "util/DataStream.hxx"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP
#define CRLF "\r\n"


int
main(int argc, char *argv[])
{
    Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

    InfoLog( << "Starting up, making stack");

    SipStack *theStack = new SipStack();
    theStack->addTransport(Transport::UDP, 5060);

    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // create address to send to
    struct sockaddr_in sa;

    sa.sin_family = PF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(5060);

    NameAddr me;
    me.uri().host() = "localhost";
    me.uri().port() = 5060;
    SipMessage* reg = Helper::makeRegister(me, me);
    Data encoded(2048, true);
    DataStream strm(encoded);
    reg->encode(strm);
    strm.flush();

    DebugLog(<<"size="<<encoded.size());
    
    // send the test message to the stack
    int err = sendto(fd, encoded.data(), encoded.size(), 0, (struct sockaddr*) & sa, sizeof(sa));

    DebugLog(<<"errno="<<errno);
    
    assert (err == 0);


    int count=0;
    while (1)
    {
       struct timeval tv;
       fd_set fdReadSet;
       int fdSetSize = 0;
       
       // Init the fd_set for the select()
       FD_ZERO(&fdReadSet);
       
       fdSetSize = 0;
       theStack->buildFdSet(&fdReadSet, &fdSetSize);
       
       // block on fdset
       tv.tv_sec = 0;
       tv.tv_usec = 1000 * theStack->getTimeTillNextProcess();

       // get the sip message that we just sent and process it
       err = select(fdSetSize, &fdReadSet, 0, 0, &tv);
       assert (err == 0);
       
       theStack->process(&fdReadSet);
       SipMessage* sipMessage = theStack->receive();
            
       if (sipMessage) 
       {
          count++;
          assert (count == 1);
          
          // InfoLog( << "got message " << *sipMessage);
          cout << "to header is " << sipMessage->header(h_To) << endl;
          cout << "to user is " << sipMessage->header(h_To).uri().user() << endl;
          cout << "cseq sequence is " << sipMessage->header(h_CSeq).sequence() << endl;
          cout << "cseq method is " << sipMessage->header(h_CSeq).method() << endl;
          cout << "contact header is " << sipMessage->header(h_Contacts).front() << endl;
          cout << "contact uri " << sipMessage->header(h_Contacts).front().uri().user() << endl;
          cout << "request line " << sipMessage->header(h_RequestLine) << endl;
          cout << "request line uri user" << sipMessage->header(h_RequestLine).uri().user() << endl;
       }

       theStack->process(&fdReadSet);
       usleep(20);
       DebugLog(<<"spin");
    }

    return 0;

}
