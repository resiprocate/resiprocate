#include <iostream>
#include <memory>

#include "rutil/DataStream.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/TransportSelector.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class TestTransportSelector
{
   public:
      static void testEmptyFail()
      {
         InfoLog (<< "testEmptyFail" );

         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
      
         Tuple tuple;

         Transport* t = ts.findTransport(tuple);
         assert(t == 0);
      }

      static void testExactFail()
      {
         InfoLog (<< "testExactFail" );
         
         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(UDP, 5061, V4, "127.0.0.1",  Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("127.0.0.2", 5060, true, UDP);
         Transport* t = ts.findTransport(tuple);
         assert(!t);
      }

      static void testExact()
      {
         InfoLog (<< "testExact" );
         
         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(UDP, 5060, V4, "127.0.0.1", Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("127.0.0.1", 5060, true, UDP);
         Transport* trans = ts.findTransport(tuple);
         assert(trans);
         assert(trans->port() == 5060);
      }

      static void testExact2()
      {
         InfoLog (<< "testExact" );
         
         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(UDP, 5060, V4, "127.0.0.1", Data::Empty, Data::Empty, SecurityTypes::TLSv1);
         ts.addTransport(UDP, 5100, V4, "127.0.0.1", Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("127.0.0.1", 5060, true, UDP);
         Transport* trans = ts.findTransport(tuple);
         assert(trans);
         assert(trans->port() == 5060);

         Tuple tuple2("127.0.0.1", 5100, true, UDP);
         trans = ts.findTransport(tuple2);
         assert(trans);
         assert(trans->port() == 5100);

         Tuple tuple3("127.0.0.2", 5100, true, UDP);
         trans = ts.findTransport(tuple3);
         assert(!trans);

         Tuple tuple4("127.0.0.2", 5200, true, UDP);
         trans = ts.findTransport(tuple4);
         assert(!trans);
      }


      static void testExactAnyPort()
      {
         InfoLog (<< "testExactAnyPort" );
         
         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(UDP, 5060, V4, "127.0.0.1", Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("127.0.0.1", 0, true, UDP);
         Transport* trans = ts.findTransport(tuple);
         assert(trans);
         assert(trans->port() == 5060);
      }


      static void testAnyInterface()
      { 
         InfoLog (<< "testAnyInterface" );
         
         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(UDP,5060,V4, Data::Empty,Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("127.0.0.1", 5060, true, UDP);
         Transport* trans = ts.findTransport(tuple);
         assert(trans);      
      }

      static void testAnyInterfaceAnyPort()
      { 
         InfoLog (<< "testAnyInterfaceAnyPort" );

         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(UDP, 5060, V4, Data::Empty,Data::Empty, Data::Empty, SecurityTypes::TLSv1);
         
         Tuple tuple("127.0.0.1", 0, true, UDP);
         Transport* trans = ts.findTransport(tuple);
         assert(trans);      
      }

      static void testAnyInterfaceAnyPortV6()
      { 
         InfoLog (<< "testAnyInterfaceAnyPortV6" );

         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(TCP,  5060,   V6, Data::Empty,Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("::1", 0, false, TCP);
         Transport* trans = ts.findTransport(tuple);
         assert(trans);      
      }

      static void testAnyInterfaceAnyPortV6Fail()
      { 
         InfoLog (<< "testAnyInterfaceAnyPortV6" );

         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         
         ts.addTransport(TCP,  5060,  V4, Data::Empty,Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("::1", 0, false, TCP);
         Transport* trans = ts.findTransport(tuple);
         assert(!trans);
      }

      static void testAnyInterfaceAnyPortFail()
      { 
         InfoLog (<< "testAnyInterfaceAnyPortFail" );

         Fifo<TransactionMessage> fif;
         TransportSelector ts(fif, 0);
         ts.addTransport(UDP,5060,V4, Data::Empty, Data::Empty, Data::Empty, SecurityTypes::TLSv1);

         Tuple tuple("127.0.0.1", 0, true, TCP);
         Transport* trans = ts.findTransport(tuple);
         assert(!trans);
      }
};

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

#if 1
   TestTransportSelector::testEmptyFail();
   TestTransportSelector::testExactFail();
   TestTransportSelector::testExact();
   TestTransportSelector::testExact2();
   TestTransportSelector::testExactAnyPort();
   TestTransportSelector::testAnyInterface();
   TestTransportSelector::testAnyInterfaceAnyPort();
   TestTransportSelector::testAnyInterfaceAnyPortFail();
   TestTransportSelector::testAnyInterfaceAnyPortV6();
   TestTransportSelector::testAnyInterfaceAnyPortV6Fail();
#else
   TestTransportSelector::testAnyInterfaceAnyPortV6();
#endif

   InfoLog (<< "TEST OK" );
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
