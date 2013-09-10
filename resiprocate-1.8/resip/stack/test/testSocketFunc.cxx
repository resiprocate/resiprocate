#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <sys/types.h>
#include <iostream>
#include <memory>

#include "rutil/GeneralCongestionManager.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "rutil/SelectInterruptor.hxx"
#include "resip/stack/TransportThread.hxx"
#include "resip/stack/InterruptableStackThread.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/Uri.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

/* prototype is in rutil/Socket.hxx:
  typedef void(*AfterSocketCreationFuncPtr)(Socket s, int transportType, const char* file, int line); */

void afterSocketCreationFunction(Socket sock, int transport, const char *file, int line)
{
}

/* For TLS and WSS, it is necessary to create a key and cert with OpenSSL

   openssl req -new -x509 -days 365 -nodes \
       -out ~/.sipCerts/domain_cert_127.0.0.1.pem \
       -keyout ~/.sipCerts/domain_key_127.0.0.1.pem \
       -subj '/CN=127.0.0.1'

  and then uncomment TLS and WSS below
 */

int main()
{
  Security *sec = new Security();
  SipStack sipStack(sec, DnsStub::EmptyNameserverList, 0, false, &afterSocketCreationFunction);
  sipStack.addTransport(UDP, 5060, V4, StunDisabled, Data::Empty, "127.0.0.1");
  sipStack.addTransport(TCP, 5062, V4, StunDisabled, Data::Empty, "127.0.0.1");
  //sipStack.addTransport(TLS, 5061, V4, StunDisabled, Data::Empty, "127.0.0.1");
  //sipStack.addTransport(WSS, 8443, V4, StunDisabled, Data::Empty, "127.0.0.1");
  return 0;
}


/* ====================================================================
 *
 * Copyright (c) 2013 Daniel Pocock  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

