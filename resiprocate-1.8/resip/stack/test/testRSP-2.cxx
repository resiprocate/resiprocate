#include "resip/stack/SipStack.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/stack/Helper.hxx"

#ifndef WIN32
#include <pthread.h>
#endif

class TestSS : public resip::Subsystem
{
public:
  static const TestSS RSP2;
private:
  TestSS(const resip::Data& rhs) : resip::Subsystem(resip::Data("TestSS")+rhs){};
  TestSS& operator=(const resip::Data& rhs);
}

const TestSS::RSP2("RSP2");

using namespace std;
using namespace resip;

bool g_stop_poller = false;

void *poller(void *arg)
{
  int i = 0;
  SipStack *stack1 = (SipStack*)arg;
  while (!g_stop_poller)
    {
      FdSet fdset;
      stack1->buildFdSet(fdset);
      fdset.selectMilliSeconds(100);
      stack1->process(fdset);
      SipMessage *msg = stack1->receive();
      if (msg)
        {
	  cerr << "Got a message" << endl;
	  SipMessage *resp = Helper::makeResponse(*msg,604);
	  stack1->send(*resp);
	  delete resp;
	  delete msg;
        }
      else
	{
	  ++i;
	  if (!(i%10))
	    cerr << "Nothing there " << i/10 << '\r';
	}
    }
  return NULL;
}

#define RESIPROCATE_SUBSYSTEM TestSS::RSP2

int main(int argc, char* argv[])
{
  resip::Log::initialize(Log::Cout, Log::Stack, *argv);
  SipStack stack1;  
  //pthread_t tid;
  stack1.addTransport(UDP, 5060, V4, StunDisabled, Data::Empty, Data::Empty, Data::Empty, SecurityTypes::TLSv1);
  stack1.addTransport(TCP, 5060, V4, StunDisabled, Data::Empty, Data::Empty, Data::Empty, SecurityTypes::TLSv1);
  //pthread_create(&tid, NULL, poller, &stack1);
  //pthread_join(tid, &retval);
  poller((void*)&stack1);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
