// How this works:
//
// The first argument to this program needs to be a file that has a test in it.
//
// Clauses in this file can be: inject_wire, expect_wire, expect_tu,
// inject_tu, and delay.  A keyword needs to be by itself on a line.  They
// need to be in this form
//
// inject_wire (or inject_tu)
// {
// ...put a SIP message here...
// }
//
// expect_wire (or expect_tu)
// {
// status=100 (or method=REGISTER)
// timeout=32
// }
//
// delay
// {
// timeout=48
// }
//
// The inject_* clauses inject the SIP message described at the "wire"
// or the "tu" level.  The expect_* clauses indicate that the test
// specification expects to read a certain response (distinguished only
// by its response code) or request (distinguished only by its method
// name).  The expect_* clauses don't block but will queue the
// expectation for the duration of the timeout specified.  When you
// really want to sit and wait for a message, insert a delay clause.

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <list>
#include <iostream>
#include <fstream>
#include <string>

#include "util/Data.hxx"
#include "util/DataStream.hxx"
#include "util/Logger.hxx"
#include "util/Socket.hxx"

#include "sipstack/Helper.hxx"
#include "sipstack/MethodTypes.hxx"
#include "sipstack/Preparse.hxx"
#include "sipstack/SipStack.hxx"
#include "sipstack/SipMessage.hxx"
#include "sipstack/Symbols.hxx"
#include "sipstack/TestTransport.hxx"
#include "sipstack/Transport.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP

// --------------------------------------------------

typedef struct {
    time_t mExpiry;
    bool mIsRequest;
    bool mIsTransport;
    int mResponseCode;
    MethodTypes mMethod;
} WaitNode;

// --------------------------------------------------

char* ProgramName = 0;
char* TestSpecBuf = 0;
ParseBuffer* TestSpecParseBuf = 0;
list<WaitNode*> WaitQueue;
SipStack* client = 0;

void
exitusage()
{
	cerr << "Usage: " << ProgramName << " ";
	cerr << "<testfileofsipmessagesandstuff>" << endl;
	exit(1);
}

void
processTimeouts(int arg)
{
    assert(!WaitQueue.empty());
    SipMessage* message = 0;

#if defined(CRAP) || 1
    // This should:
    // 1. Take all messages from the "wire" and "tu" fifos
    // 2. For each message, look through the WaitQueue and see
    //    if the new message matches something we were waiting for.
    //    If yes, through away the queue entry, else raise a warning.
    // 3. When all messages from the "wire" and "tu" have been
    //    examined, see if anything in the queue has expired.
    //    If yes, warn, else just continue.

    // First go through the "wire" data
    TestBufType& cbuf = TestOutBuffer::instance().getBuf();
    while (!cbuf.empty())
    {
	int len = cbuf.size();
	char *newbuf = new char[len+1];
	for (int i = 0; i < len; i++)
	{
	    newbuf[i] = cbuf.front();
	    cbuf.pop_front();
	}
	newbuf[len] = 0;
	message = Helper::makeMessage(newbuf);
	for (list<WaitNode*>::iterator i = WaitQueue.begin();
	     i != WaitQueue.end();
	     i++)
	{
	    if ((*i)->mIsRequest && message->isRequest())
	    {
		if ((*i)->mMethod == message->header(h_RequestLine).getMethod())
		{
		    // We matched something we expected.
		    delete newbuf;
		    newbuf = 0;
		    delete message;
		    WaitQueue.erase(i);
		    break;
		}
	    }
	    else if (!(*i)->mIsRequest && message->isResponse())
	    {
		if ((*i)->mResponseCode ==
		    message->header(h_StatusLine).responseCode())
		{
		    // We matched something we expected.
		    delete newbuf;
		    newbuf = 0;
		    delete message;
		    WaitQueue.erase(i);
		    break;
		}
	    }
	}
	if (newbuf)
	{
	    cerr << "Warning: unexpected message seen on the wire." << endl;
	    delete newbuf;
	}
    }

    // Now go through the data at the TU.
    while ((message = client->receive()))
    {
	for (list<WaitNode*>::iterator i = WaitQueue.begin();
	     i != WaitQueue.end();
	     i++)
	{
	    if ((*i)->mIsRequest && message->isRequest())
	    {
		if ((*i)->mMethod ==
		    message->header(h_RequestLine).getMethod())
		{
		    // We matched something we expected.
		    delete message;
		    message = 0;
		    WaitQueue.erase(i);
		    break;
		}
	    }
	    else if (!(*i)->mIsRequest && message->isResponse())
	    {
		if ((*i)->mResponseCode ==
		    message->header(h_StatusLine).responseCode())
		{
		    // We matched something we expected.
		    delete message;
		    message = 0;
		    WaitQueue.erase(i);
		    break;
		}
	    }
	}
	if (message)
	{
	    cerr << "Warning: unexpected message seen at TU." << endl;
	    delete message;
	}
    }

    // Check to see if some expect clauses have timed out
    for (list<WaitNode*>::iterator i = WaitQueue.begin();
	 i != WaitQueue.end();
	 i++)
    {
	if ((*i)->mExpiry < time(NULL))
	{
	    cerr << "Error: timeout waiting for ";
	    if ((*i)->mIsRequest)
	    {
		cerr << "mMethod " << (*i)->mMethod << endl;
	    }
	    else
	    {
		cerr << "mResponseCode " << (*i)->mResponseCode << endl;
	    }
	    WaitQueue.erase(i);
	}
    }
#endif

    signal(SIGALRM, processTimeouts);
}

void
processInject()
{
    const char* start = TestSpecParseBuf->position();
    const char* now;
    bool isWireInject = false;

    if (!strncasecmp(start, "inject_wire", strlen("inject_wire")))
    {
	isWireInject = true;
    }
    else if (!strncasecmp(start, "inject_tu", strlen("inject_tu")))
    {
	isWireInject = false;
    }

    TestSpecParseBuf->skipToOneOf("{");
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();

    start = TestSpecParseBuf->position();
    now = TestSpecParseBuf->skipToOneOf("}");
    *const_cast<char*>(now) = 0;
    cerr << "-> Injecting " << endl << start;
    TestSpecParseBuf->skipChar();
    SipMessage* message = Helper::makeMessage(start);
    if (isWireInject)
    {
	client->mStateMacFifo.add(message);
    }
    else
    {
	client->send(*message);
    }
}

void
processExpect()
{
    const char* start = TestSpecParseBuf->position();
    const char* now;
    WaitNode* thisWait = new WaitNode;
    assert(thisWait);
    thisWait->mResponseCode = 0;

    if (!strncasecmp(start, "expect_wire", strlen("expect_wire")))
    {
	thisWait->mIsTransport = true;
    }
    else if (!strncasecmp(start, "expect_tu", strlen("expect_tu")))
    {
	thisWait->mIsTransport = false;
    }

    TestSpecParseBuf->skipToOneOf("{");
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();
    start = TestSpecParseBuf->position();

    // We will want to get two of these in an expect_ clause.
    for (int i = 0; i < 2; i++)
    {
	TestSpecParseBuf->skipToOneOf("=");
	TestSpecParseBuf->skipChar();
	TestSpecParseBuf->skipWhitespace();
	if (!strncasecmp(start, "method", strlen("method")))
	{
	    start = TestSpecParseBuf->position();
	    now = TestSpecParseBuf->skipToOneOf(ParseBuffer::Whitespace);
	    thisWait->mIsRequest = true;
	    thisWait->mMethod = getMethodType(start, now-start);
	}
	else if (!strncasecmp(start, "status", strlen("status")))
	{
	    TestSpecParseBuf->skipToOneOf("0123456789");
	    thisWait->mIsRequest = false;
	    thisWait->mResponseCode = TestSpecParseBuf->integer();
	}
	else if (!strncasecmp(start, "timeout", strlen("timeout")))
	{
	    TestSpecParseBuf->skipToOneOf("0123456789");
	    thisWait->mExpiry = TestSpecParseBuf->integer() + time(NULL);
	}
	TestSpecParseBuf->skipWhitespace();
	start = TestSpecParseBuf->position();
    }

    WaitQueue.push_front(thisWait);
    // alarm(thisWait->mExpiry <= 0 ? 1 : thisWait->mExpiry);

    TestSpecParseBuf->skipToOneOf("}");
    TestSpecParseBuf->skipChar();

    // For debugging purposes...
    cerr << "-> Expecting " << endl;
    cerr << "   mIsTransport = " << (thisWait->mIsTransport == true) << endl;
    cerr << "   mIsRequest = " << (thisWait->mIsRequest == true) << endl;
    cerr << "   mResponseCode = " << (thisWait->mResponseCode) << endl;
    cerr << "   mExpiry = " << (thisWait->mExpiry) << endl;
}

void
processDelays()
{
    TestSpecParseBuf->skipToOneOf("{");
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();

    TestSpecParseBuf->skipToOneOf("0123456789");
    int sleepLength = TestSpecParseBuf->integer();

    // We sleep this way to avoid conflict with SIGALRM from alarm().
    struct timespec ts, remainder;
    ts.tv_sec = sleepLength;
    ts.tv_nsec = 0;
    while (nanosleep(&ts, &remainder) < 0)
    {
	ts = remainder;
    }

    TestSpecParseBuf->skipToOneOf("}");
    TestSpecParseBuf->skipChar();
    cerr << "-> Delaying " << sleepLength << endl;
}

int
processClause()
{
    TestSpecParseBuf->skipWhitespace();

    // Look for 'i'/'e'/'d' for inject... or expect... or delay respectively.
    const char* now = TestSpecParseBuf->skipToOneOf("ied");
    switch (*now)
    {
    case 'i':
	processInject();
	break;
    case 'e':
	processExpect();
	break;
    case 'd':
	processDelays();
	break;
    default:
	cerr << "Warning: error parsing test specification." << endl;
	TestSpecParseBuf->skipToOneOf("}");
	TestSpecParseBuf->skipChar();
    }

    TestSpecParseBuf->skipWhitespace();
    return !TestSpecParseBuf->eof();
}

int
main(int argc, char *argv[])
{
    ProgramName = argv[0];

    if (NULL == argv[1]) {
	exitusage();
    }

    struct stat buf;
    if (stat(argv[1], &buf) < 0)
    {
	cerr << "Error: " << strerror(errno) << endl;
	exitusage();
    }

    ifstream testSpec;
    testSpec.open(argv[1], ifstream::in);
    if (!testSpec.is_open())
    {
	cerr << "Error: could not open "<< argv[1] << endl;
	exitusage();
    }

    TestSpecBuf = new char[buf.st_size+1];
    assert(TestSpecBuf);
    testSpec.read(TestSpecBuf, buf.st_size);
    TestSpecParseBuf = new ParseBuffer(TestSpecBuf, buf.st_size);
    assert(TestSpecParseBuf);

    client = new SipStack();
    assert(client);
    client->addTransport(Transport::TestReliable, 5060);
    FdSet clientFdSet;

    signal(SIGALRM, processTimeouts);

    while (processClause())
    {
	client->process(clientFdSet);
    }

    if (!WaitQueue.empty())
    {
	cerr << "Warning: ending with expect clauses outstanding." << endl;
    }

    return 0;
}
