#include <sipstack/SipStack.hxx>
#include <sipstack/Transport.hxx>
#include <sipstack/Uri.hxx>
#include <util/Logger.hxx>

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

    const char testmsg[] =
        "INVITE sip:B@outside.com SIP/2.0" CRLF
        "Via: SIP/2.0/UDP pc.inside.com:5060;branch=z9hG4bK0000" CRLF
        "Max-Forwards: 70" CRLF
        "From: sip:A@inside.com;tag=12345" CRLF
        "To: You <sip:B@outside.com>" CRLF
        "Call-ID: 0123456789@pc.inside.com" CRLF
        "CSeq: 1 INVITE" CRLF
        "Contact: <sip:A@pc.inside.com>" CRLF
        "Content-Length: 0" CRLF
        CRLF;

    Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

    InfoLog( << "Starting up, making stack");

    SipStack *theStack = new SipStack();

    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // create address to send to
    struct sockaddr_in sa;

    sa.sin_family = PF_INET;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_port = htons(5060);

    fd_set fdReadSet;

    int fdSetSize = 0;

    struct timeval tv;

    // Init the fd_set for the select()
    FD_ZERO(&fdReadSet);

    fdSetSize = 0;

    // Ask the stack to build the fd_set
    theStack->buildFdSet(&fdReadSet, &fdSetSize);

    assert(fdSetSize > 0);

    // block on fdset
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * theStack->getTimeTillNextProcess();

    // send the test message to the stack
    int err = sendto(fd, testmsg, sizeof(testmsg), 0, (struct sockaddr*) & sa, sizeof(sa));
    int e = errno;
    if (err < 0)
    {
        InfoLog( << "Error " << e << " " << strerror(e) << " in select");
        return -1;
    }


    // get the sip message that we just sent and process it
    err = select(fdSetSize, &fdReadSet, 0, 0, &tv);

    e = errno;
    SipMessage *sipMessage = 0;

    if (err < 0)
    {
        InfoLog( << "Error " << e << " " << strerror(e) << " in select");

    }
    else
    {
        theStack->process(&fdReadSet);
        sipMessage = theStack->receive();
        

        if (sipMessage) {
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

        while (3)
        {
            theStack->buildFdSet(&fdReadSet, &fdSetSize);
            theStack->process(&fdReadSet);
            usleep(20);
            DebugLog(<<"spin");
            
        }

    }


    return 0;

}
