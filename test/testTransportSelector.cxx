#include <iostream>
#include <memory>

#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TransportSelector.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class TestTransportSelector
{
   public:
      static void testEmptyFail()
      {
         cerr << "testEmptyFail" << endl;

         Fifo<Message> fif;
         TransportSelector ts(false, fif);
      
         Tuple tuple;

         Transport* t = ts.findTransport(tuple);
         assert(t == 0);
      }

      static void testExactFail()
      {
         cerr << "testExactFail" << endl;
         
         Tuple tuple("127.0.0.2", 5060, true, UDP);
         
         Fifo<Message> fif;
         TransportSelector ts(false, fif);
         
         ts.addTransport(UDP,
                         5061,
                         V4,
                         "127.0.0.1");

         Transport* t = ts.findTransport(tuple);
         assert(!t);
      }

      static void testExact()
      {
         cerr << "testExact" << endl;
         
         Tuple tuple("127.0.0.1", 5060, true, UDP);
         
         Fifo<Message> fif;
         TransportSelector ts(false, fif);
         
         ts.addTransport(UDP,
                         5060,
                         V4,
                         "127.0.0.1");

         Transport* trans = ts.findTransport(tuple);
         assert(trans);
      }

      static void testAnyInterface()
      { 
         cerr << "testAnyInterface" << endl;
         
         Tuple tuple("127.0.0.1", 5060, true, UDP);
         
         Fifo<Message> fif;
         TransportSelector ts(false, fif);
         
         ts.addTransport(UDP,
                         5060,
                         V4,
                         "");

         Transport* trans = ts.findTransport(tuple);
         assert(trans);      
      }

      static void testAnyInterfaceAnyPort()
      { 
         cerr << "testAnyInterfaceAnyPort" << endl;

         Tuple tuple("127.0.0.1", 5061, true, UDP);

         Fifo<Message> fif;
         TransportSelector ts(false, fif);
         
         ts.addTransport(UDP,
                         5060,
                         V4,
                         "");

         Transport* trans = ts.findTransport(tuple);
         assert(trans);      
      }

      static void testAnyInterfaceAnyPortV6()
      { 
         ErrLog(<< "testAnyInterfaceAnyPortV6");

         ErrLog(<< "before tuple");
         Tuple tuple("::1", 5061, false, TCP);
         ErrLog(<< "after tuple");

         Fifo<Message> fif;
         ErrLog(<< "before TransportSelector");
         TransportSelector ts(false, fif);
         ErrLog(<< "after TransportSelector");

         ErrLog(<< "before addTransport");
         ts.addTransport(TCP,
                         5060,
                         V6,
                         "");
         ErrLog(<< "after addTransport");

         Transport* trans = ts.findTransport(tuple);
         assert(trans);      
      }

      static void testAnyInterfaceAnyPortV6Fail()
      { 
         cerr << "testAnyInterfaceAnyPortV6" << endl;

         Tuple tuple("::1", 5061, false, TCP);

         Fifo<Message> fif;
         TransportSelector ts(false, fif);
         
         ts.addTransport(TCP,
                         5060,
                         V4,
                         "");

         Transport* trans = ts.findTransport(tuple);
         assert(!trans);
      }

      static void testAnyInterfaceAnyPortFail()
      { 
         cerr << "testAnyInterfaceAnyPortFail" << endl;

         Tuple tuple("127.0.0.1", 5061, true, TCP);

         Fifo<Message> fif;
         TransportSelector ts(false, fif);
         
         ts.addTransport(UDP,
                         5060,
                         V4,
                         "");

         Transport* trans = ts.findTransport(tuple);
         assert(!trans);
      }
};

int
main(int argc, char** argv)
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

#if 1
   TestTransportSelector::testEmptyFail();
   TestTransportSelector::testExactFail();
   TestTransportSelector::testExact();
   TestTransportSelector::testAnyInterface();
   TestTransportSelector::testAnyInterfaceAnyPort();
   TestTransportSelector::testAnyInterfaceAnyPortFail();
   TestTransportSelector::testAnyInterfaceAnyPortV6();
   TestTransportSelector::testAnyInterfaceAnyPortV6Fail();
#else
   TestTransportSelector::testAnyInterfaceAnyPortV6();
#endif

   cerr << "\nTEST OK" << endl;
   return 0;
}

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
