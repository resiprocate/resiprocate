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

#include <sys/time.h>
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

#include "sip2/util/Data.hxx"
#include "sip2/util/DataStream.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/util/Socket.hxx"

#include "sip2/sipstack/Helper.hxx"
#include "sip2/sipstack/MethodTypes.hxx"
#include "sip2/sipstack/Preparse.hxx"
#include "sip2/sipstack/SipStack.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/sipstack/TestTransport.hxx"
#include "sip2/sipstack/Transport.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP

// --------------------------------------------------

typedef struct {
    struct timeval mExpiry;
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
    if (WaitQueue.empty())
    {
	return;
    }
    SipMessage* message = 0;

#if defined(UGLY) || 1
    // This should:
    // 1. Take all messages from the "wire" and "tu" fifos
    // 2. For each message, look through the WaitQueue and see
    //    if the new message matches something we were waiting for.
    //    If yes, through away the queue entry, else raise a warning.
    // 3. When all messages from the "wire" and "tu" have been
    //    examined, see if anything in the queue has expired.
    //    If yes, warn, else just continue.

    // First go through the "wire" data
    TestBufType& msgFifo = TestOutBuffer::instance().getBuf();
    while (msgFifo.messageAvailable())
    {
	Data* newMessage = msgFifo.getNext();
	message = Helper::makeMessage(*newMessage, true);
	for (list<WaitNode*>::iterator i = WaitQueue.begin();
	     i != WaitQueue.end();
	     /* don't increment */)
	{
	    if ((*i)->mIsRequest && message->isRequest())
	    {
		if ((*i)->mMethod == message->header(h_RequestLine).getMethod())
		{
		    // We matched something we expected.
		    delete newMessage;
		    newMessage = 0;
		    delete message;
		    delete *i;
		    WaitQueue.erase(i++);
		    break;
		}
		else
		{
		    ++i;
		}
	    }
	    else if (!(*i)->mIsRequest && message->isResponse())
	    {
		if ((*i)->mResponseCode ==
		    message->header(h_StatusLine).responseCode())
		{
		    // We matched something we expected.
		    delete newMessage;
		    newMessage = 0;
		    delete message;
		    delete *i;
		    WaitQueue.erase(i++);
		    break;
		}
		else
		{
		    ++i;
		}
	    }
	    else
	    {
		++i;
	    }
	}
	if (newMessage)
	{
	    DebugLog( << "Warning: unexpected message seen at the transport");
	    delete newMessage;
	}
	else
	{
	    DebugLog( << "Success: expected message seen on the wire");
	}
    }

    // Now go through the data at the TU.
    while ((message = client->receive()))
    {
	for (list<WaitNode*>::iterator i = WaitQueue.begin();
	     i != WaitQueue.end();
	     /* don't increment */)
	{
	    if ((*i)->mIsRequest && message->isRequest())
	    {
		if ((*i)->mMethod ==
		    message->header(h_RequestLine).getMethod())
		{
		    // We matched something we expected.
		    delete message;
		    message = 0;
		    delete *i;
		    WaitQueue.erase(i++);
		    break;
		}
		else
		{
		    ++i;
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
		    delete *i;
		    WaitQueue.erase(i++);
		    break;
		}
		else
		{
		    ++i;
		}
	    }
	    else
	    {
		++i;
	    }
	}
	if (message)
	{
	    DebugLog( << "Warning: unexpected message seen at the TU");
	    delete message;
	}
	else
	{
	    DebugLog( << "Success: expected message seen at TU");
	}
    }

    for (list<WaitNode*>::iterator i = WaitQueue.begin();
	 i != WaitQueue.end();
	 /* don't increment */)
    {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if ((*i)->mExpiry.tv_sec < tv.tv_sec ||
	   ((*i)->mExpiry.tv_sec == tv.tv_sec &&
	    (*i)->mExpiry.tv_usec < tv.tv_usec))
	{
	    if ((*i)->mIsRequest)
	    {
		DebugLog(<< "Error: timeout waiting for "
		         << MethodNames[(*i)->mMethod] << " method");
	    }
	    else
	    {
		DebugLog(<< "Error: timeout waiting for "
		         << (*i)->mResponseCode << " status code");
	    }
	    delete *i;
	    WaitQueue.erase(i++);
	}
	/*
	else
	{
	    cerr << "Still waiting for: ";
	    if ((*i)->mIsRequest)
	    {
		cerr << MethodNames[(*i)->mMethod] << " method" << endl;
	    }
	    else
	    {
		cerr << (*i)->mResponseCode << " status code" << endl;
	    }
	    ++i;
	}
	*/
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
    else
    {
	DebugLog(<< "Warning: error parsing test specification.");
	TestSpecParseBuf->skipToOneOf("}");
	TestSpecParseBuf->skipChar();
	return;
    }

    TestSpecParseBuf->skipToOneOf("{");
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();

    start = TestSpecParseBuf->position();
    now = TestSpecParseBuf->skipToOneOf("}");
    *const_cast<char*>(now) = 0;
    DebugLog(<< "Injecting (isWireInject=" << isWireInject << "): " << start);
    TestSpecParseBuf->skipChar();
    if (isWireInject)
    {
	// sendToWire() is a helper function for TestTransport stuff.
	// sendToWire(start);
	SipMessage* message = Helper::makeMessage(start, true);
	assert(message);
	client->mStateMacFifo.add(message);
    }
    else
    {
	SipMessage* message = Helper::makeMessage(start, false);
	assert(message);
	client->send(*message);
    }
}

void
processExpect()
{
    const char* start = TestSpecParseBuf->position();
    const char* now;
    unsigned int expireTime = 1;
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
    else
    {
	DebugLog(<< "Warning: error parsing test specification"); 
	TestSpecParseBuf->skipToOneOf("}");
	TestSpecParseBuf->skipChar();
	delete thisWait;
	return;
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
	    expireTime = TestSpecParseBuf->integer();
	}
	else
	{
	    DebugLog(<< "Warning: error parsing test specification"); 
	    TestSpecParseBuf->skipToOneOf("}");
	    TestSpecParseBuf->skipChar();
	    delete thisWait;
	    return;
	}
	TestSpecParseBuf->skipWhitespace();
	start = TestSpecParseBuf->position();
    }

    assert(thisWait);
    WaitQueue.push_front(thisWait);
    gettimeofday(&thisWait->mExpiry, NULL);
    thisWait->mExpiry.tv_sec += expireTime;
    alarm(expireTime);


    TestSpecParseBuf->skipToOneOf("}");
    TestSpecParseBuf->skipChar();

    /*
    cerr << "-> Expecting " << endl;
    cerr << "   mIsTransport = " << (thisWait->mIsTransport == true) << endl;
    cerr << "   mIsRequest = " << (thisWait->mIsRequest == true) << endl;
    cerr << "   mResponseCode = " << (thisWait->mResponseCode) << endl;
    cerr << "   mExpiry = " << (thisWait->mExpiry.tv_sec) << endl;
    */
}

void
processDelays()
{
    TestSpecParseBuf->skipToOneOf("{");
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();

    TestSpecParseBuf->skipToOneOf("0123456789");
    int sleepLength = TestSpecParseBuf->integer();

    DebugLog( << "Pausing for " << sleepLength << " seconds");

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
}

bool
processClause()
{
    TestSpecParseBuf->skipWhitespace();

    // Look for 'i'/'e'/'d' for inject... or expect... or delay respectively.
    const char* now = TestSpecParseBuf->skipToOneOf("ied#");
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
    case '#':
	TestSpecParseBuf->skipToOneOf("\n");
	TestSpecParseBuf->skipChar();
	break;
    default:
	DebugLog(<< "Warning: error parsing test specification");
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
    client->addTransport(Transport::TestUnreliable, 1234);
    FdSet clientFdSet;

    signal(SIGALRM, processTimeouts);

    while (processClause())
    {
	client->process(clientFdSet);
    }

    // Catch any remaining events.
    processTimeouts(0);
    if (!WaitQueue.empty())
    {
	DebugLog( << "Warning: ending with expect clauses outstanding"); 
    }

    return 0;
}
