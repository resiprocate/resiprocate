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
// ...put a SIP message here that has a CRLF after each line...
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
// This is used as a "barrier" at which you want all the above timeouts
// to happen.  You'll want one at the end of your test, for example.

#ifndef __MINGW32__

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"

#include "resip/stack/test/TestSupport.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/Transport.hxx"
#include "rutil/ParseBuffer.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST
#define PORT 5060


// --------------------------------------------------

namespace resip 
{
class TestFSM  // this class is a friend of the SipStack and can directly access private stuff
{
   public:
      static void addMessage(SipStack* stack,SipMessage* message)
      {
         stack->mTransactionController->mStateMacFifo.add(message);
      }
};
}

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
FdSet clientFdSet;
struct sockaddr_in clientSa;
int clientFd;
Fifo<SipMessage> fakeTxFifo;
int errorCount = 0;

// --------------------------------------------------

// This is pure evil.  We interpose our own version of sendto
// so that this gets called by the UDP transport instead of the libc
// version.
int
sendto(int s, const void *msg, size_t len, int flags,
       const struct sockaddr *to, int tolen)
{
    fakeTxFifo.add(TestSupport::makeMessage(Data((const char *)msg, (int)len), true));
    return len;
}

// --------------------------------------------------


void
exitusage()
{
	cerr << "Usage: " << ProgramName << " ";
	cerr << "<testfileofsipmessagesandstuff>" << endl;
	exit(1);
}

extern "C" { void processTimeouts(int arg); }

void
processTimeouts(int arg)
{
    client->process(5);

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
    while (fakeTxFifo.messageAvailable())
    {
	message = fakeTxFifo.getNext();
	for (list<WaitNode*>::iterator i = WaitQueue.begin();
	     i != WaitQueue.end();
	     /* don't increment */)
	{
	    if ((*i)->mIsRequest && message->isRequest())
	    {
		if ((*i)->mMethod == message->header(h_RequestLine).getMethod())
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
	    DebugLog( << "Warning: unexpected message seen at the transport: " 
		      << message);
	}
	else
	{
	    DebugLog( << "Success: expected message seen at the transport");
	}
	delete message;
    }

    // Now go through the data at the TU.
    while (0 != (message = client->receive()))
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
	    DebugLog( << "Warning: unexpected message seen at the TU: "
		      << *message);
	    delete message;
	}
	else
	{
	    DebugLog( << "Success: expected message seen at TU");
	}
    }

    // Print the list of expected events that have failed to happen withing
    // the specified timeout.
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
		         << getMethodName((*i)->mMethod) << " method");
		++errorCount;
	    }
	    else
	    {
		DebugLog(<< "Error: timeout waiting for "
		         << (*i)->mResponseCode << " status code");
		++errorCount;
	    }
	    delete *i;
	    WaitQueue.erase(i++);
	}
	else
	{
	    /*
	    cerr << "Still waiting for: ";
	    if ((*i)->mIsRequest)
	    {
		cerr << MethodNames[(*i)->mMethod] << " method" << endl;
	    }
	    else
	    {
		cerr << (*i)->mResponseCode << " status code" << endl;
	    }
	    */
	    ++i;
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
    else
    {
	DebugLog(<< "Warning: error parsing test specification.");
	TestSpecParseBuf->skipToChar('}');
	TestSpecParseBuf->skipChar();
	return;
    }

    TestSpecParseBuf->skipToChar('{');
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();

    start = TestSpecParseBuf->position();
    now = TestSpecParseBuf->skipToChar('}');
    *const_cast<char*>(now) = 0;
    DebugLog(<< "Injecting (isWireInject=" << isWireInject << "): " << start);
    TestSpecParseBuf->skipChar();
    if (isWireInject)
    {
	// sendToWire() is a helper function for TestTransport stuff.
	// sendToWire(start);
	SipMessage* message = TestSupport::makeMessage(start, true);
	assert(message);

	TestFSM::addMessage(client,message); //  does a client->mStateMacFifo.add(message);
	
    }
    else
    {
	SipMessage* message = TestSupport::makeMessage(start, false);
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
	TestSpecParseBuf->skipToChar('}');
	TestSpecParseBuf->skipChar();
	delete thisWait;
	return;
    }

    TestSpecParseBuf->skipToChar('{');
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();
    start = TestSpecParseBuf->position();

    // We will want to get two of these in an expect_ clause.
    for (int i = 0; i < 2; i++)
    {
	TestSpecParseBuf->skipToChar('=');
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
	    TestSpecParseBuf->skipToChar('}');
	    TestSpecParseBuf->skipChar();
	    delete thisWait;
	    return;
	}
	TestSpecParseBuf->skipWhitespace();
	start = TestSpecParseBuf->position();
    }

    assert(thisWait);

    gettimeofday(&thisWait->mExpiry, NULL);
    thisWait->mExpiry.tv_sec += expireTime / 1000;
    thisWait->mExpiry.tv_usec += (expireTime % 1000) * 1000;
    WaitQueue.push_front(thisWait);

    TestSpecParseBuf->skipToChar('}');
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
    TestSpecParseBuf->skipToChar('{');
    TestSpecParseBuf->skipChar();
    TestSpecParseBuf->skipWhitespace();

    TestSpecParseBuf->skipToOneOf("0123456789");
    int sleepLength = TestSpecParseBuf->integer();

    DebugLog( << "Pausing for " << sleepLength << " ms");

    // We sleep this way to avoid conflict with SIGALRM from alarm().
    struct timespec ts, remainder;
    ts.tv_sec = sleepLength / 1000;
    ts.tv_nsec = (sleepLength % 1000) * 1000000;
    while (nanosleep(&ts, &remainder) < 0)
    {
	ts = remainder;
    }

    TestSpecParseBuf->skipToChar('}');
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
	TestSpecParseBuf->skipToChar('\n');
	TestSpecParseBuf->skipChar();
	break;
    default:
	DebugLog(<< "Warning: error parsing test specification");
	TestSpecParseBuf->skipToChar('}');
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

    int clientFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    clientSa.sin_family = PF_INET;
    clientSa.sin_addr.s_addr = inet_addr("127.0.0.1");
    clientSa.sin_port = htons(PORT);
    int clientFdFlags = fcntl(clientFd, F_GETFL, 0);
    fcntl(clientFd, F_SETFL, clientFdFlags | O_NONBLOCK);

    client = new SipStack();
    assert(client);
    client->addTransport(UDP, PORT);

    signal(SIGALRM, processTimeouts);

    // Cause a signal to be generated with setitimer for its resolution
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100000; // 100 ms resolution
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 100000; // 100 ms resolution
    setitimer(ITIMER_REAL, &timer, NULL);

    while (processClause())
    {
    }

    // Catch any remaining events.
    processTimeouts(0);
    if (!WaitQueue.empty())
    {
	DebugLog( << "Warning: ending with expect clauses outstanding"); 
	++errorCount;
    }

    if (errorCount > 0)
    {
	cerr << "FAIL" << endl;
    }
    else
    {
	cerr << "PASS" << endl;
    }

    return errorCount;
}
#else
#include <iostream>
int
main(int argc, char *argv[])
{
  std::cout << argv[0] << ": Sorry. This test driver hasn't been ported "
                          "to Windows yet." << std::endl;
  return -1;
}
#endif
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
