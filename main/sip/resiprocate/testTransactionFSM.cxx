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

#include <util/Data.hxx>
#include <util/Logger.hxx>
#include <util/DataStream.hxx>

#include <sipstack/Helper.hxx>
#include <sipstack/MethodTypes.hxx>
#include <sipstack/Preparse.hxx>
#include <sipstack/SipStack.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/Symbols.hxx>
#include <sipstack/TestTransport.hxx>
#include <sipstack/Transport.hxx>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP

// --------------------------------------------------

typedef struct {
	bool isMethod;
	bool fromWire;
	char *methodName;
	int statusCode;
	time_t expiry;
	MethodTypes method;
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
	cerr << "Usage: " << ProgramName << " <testfileofsipmessagesandstuff>"
	     << endl;
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
    while (1)
    {
	TestBufType& cbuf = TestOutBuffer::instance().getBuf();
	int len = cbuf.size();
	if (len < 1)
	{
	    break;
	}
	char *newbuf = new char[len+1];
	TestBufType::const_iterator iter = cbuf.begin();
	for (int i = 0; iter != cbuf.end(); iter++, i++)
	{
	    newbuf[i] = *iter;
	}
	newbuf[len] = 0;
	message = Helper::makeMessage(newbuf);
	for (list<WaitNode*>::iterator i = WaitQueue.begin();
	     i != WaitQueue.end();
	     i++)
	{
	    if ((*i)->isMethod && message->isRequest())
	    {
		if (getMethodType((*i)->methodName) ==
		    message->header(h_RequestLine).getMethod())
		{
		    // We matched something we expected.
		    delete newbuf;
		    newbuf = 0;
		    delete message;
		    WaitQueue.erase(i);
		    break;
		}
	    }
	    else if (!(*i)->isMethod && message->isResponse())
	    {
		if ((*i)->statusCode ==
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
	    delete message;
	}
    }

    // Now go through the data at the TU.
    while ((message = client->receive()))
    {
	for (list<WaitNode*>::iterator i = WaitQueue.begin();
	     i != WaitQueue.end();
	     i++)
	{
	    if ((*i)->isMethod && message->isRequest())
	    {
		if (getMethodType((*i)->methodName) ==
		    message->header(h_RequestLine).getMethod())
		{
		    // We matched something we expected.
		    delete message;
		    message = 0;
		    WaitQueue.erase(i);
		    break;
		}
	    }
	    else if (!(*i)->isMethod && message->isResponse())
	    {
		if ((*i)->statusCode ==
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
	if ((*i)->expiry < time(NULL))
	{
	    cerr << "Error: timeout waiting for ";
	    if ((*i)->isMethod)
	    {
		cerr << "method " << (*i)->methodName << endl;
	    }
	    else
	    {
		cerr << "status " << (*i)->statusCode << endl;
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

    if (!strcasecmp(start, "inject_wire"))
    {
	isWireInject = true;
    }
    else if (!strcasecmp(start, "inject_tu"))
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
    thisWait->statusCode = 0;
    thisWait->methodName = 0;

    if (!strncasecmp(start, "expect_wire", strlen("expect_wire")))
    {
	thisWait->fromWire = true;
    }
    else if (!strncasecmp(start, "expect_tu", strlen("expect_tu")))
    {
	thisWait->fromWire = false;
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
	    thisWait->isMethod = true;
	    thisWait->method = getMethodType(start, now-start);
	}
	else if (!strncasecmp(start, "status", strlen("status")))
	{
	    TestSpecParseBuf->skipToOneOf("0123456789");
	    thisWait->isMethod = false;
	    thisWait->statusCode = TestSpecParseBuf->integer();
	}
	else if (!strncasecmp(start, "timeout", strlen("timeout")))
	{
	    TestSpecParseBuf->skipToOneOf("0123456789");
	    thisWait->expiry = TestSpecParseBuf->integer() + time(NULL);
	}
	TestSpecParseBuf->skipWhitespace();
	start = TestSpecParseBuf->position();
    }

    WaitQueue.push_front(thisWait);
    // alarm(thisWait->expiry <= 0 ? 1 : thisWait->expiry);

    TestSpecParseBuf->skipToOneOf("}");
    TestSpecParseBuf->skipChar();

    // For debugging purposes...
    cerr << "-> Expecting " << endl;
    cerr << "   fromWire = " << (thisWait->fromWire == true) << endl;
    cerr << "   isMethod = " << (thisWait->isMethod == true) << endl;
    cerr << "   statusCode = " << (thisWait->statusCode) << endl;
    cerr << "   expiry = " << (thisWait->expiry) << endl;
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
    fd_set fdReadSet;
    FD_ZERO(&fdReadSet);

    signal(SIGALRM, processTimeouts);

    while (processClause())
    {
	client->process(&fdReadSet);
    }

    if (!WaitQueue.empty())
    {
	cerr << "Warning: ending with expect clauses outstanding." << endl;
    }

    return 0;
}
