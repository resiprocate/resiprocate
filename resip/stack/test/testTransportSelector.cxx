#include <memory>

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/UdpTransport.hxx"
#include "rutil/dns/DnsStub.hxx"

#define private public
#include "resip/stack/TransportSelector.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

class TestUdpTransport : public UdpTransport
{
   public:
      TestUdpTransport(Fifo<TransactionMessage>& rxFifo,
                       const uint transportKey,
                       int portNum,
                       IpVersion version,
                       const Data& interfaceObj) :
         UdpTransport(rxFifo, portNum, version, StunDisabled, version == V6 ? "::1" : "127.0.0.1")
      {
         // The transport is bound to localhost to avoid socket conflicts.
         // However, the real host is used in comparisons.
         mInterface = interfaceObj;

         setKey(transportKey);
      }

      ~TestUdpTransport() override = default;
};

class TestTcpTransport : public TcpTransport
{
   public:
      TestTcpTransport(Fifo<TransactionMessage>& rxFifo,
                       const uint transportKey,
                       int portNum,
                       IpVersion version,
                       const Data& interfaceObj) :
         TcpTransport(rxFifo, portNum, version, version == V6 ? "::1" : "127.0.0.1")
      {
         // The transport is bound to localhost to avoid socket conflicts.
         // However, the real host is used in comparisons.
         mInterface = interfaceObj;

         setKey(transportKey);
      }

      ~TestTcpTransport() override = default;
};

class TestTransportSelector : public TransportSelector
{
   public:
      TestTransportSelector() :
         TransportSelector(mFifo, nullptr, mDnsStub, mCompression, false)
      { }

      ~TestTransportSelector() override = default;

      void addTransport(const Data &interfaceObj, int portNum, IpVersion version, TransportType ttype)
      {
         std::unique_ptr<Transport> transport;

         if (ttype == UDP)
         {
            transport.reset(new TestUdpTransport { mFifo, mNextTransportKey++, portNum, version, interfaceObj });
         }
         else if (ttype == TCP)
         {
            transport.reset(new TestTcpTransport { mFifo, mNextTransportKey++, portNum, version, interfaceObj });
         }
         else
         {
            resip_assert(0);  // Tests do not support the provided transport type.
         }

         TransportSelector::addTransport(std::move(transport), false);
      }

   private:
      static Fifo<TransactionMessage> mFifo;
      static DnsStub mDnsStub;
      static Compression mCompression;
      static uint mNextTransportKey;
};

Fifo<TransactionMessage> TestTransportSelector::mFifo;
DnsStub TestTransportSelector::mDnsStub;
Compression TestTransportSelector::mCompression = Compression::NONE;
uint TestTransportSelector::mNextTransportKey = 1;

void
testFindTransportBySource()
{
   SipMessage msg;     // Enough to test UPD and TCP transports.

   {
      // Transport selection for unknown source.
      resipCout << "test transport selection by source interface lookup - empty fail" << std::endl;

      TestTransportSelector ts;

      Tuple tuple;
      Transport *t = ts.findTransportBySource(tuple, &msg);
      resip_assert(t == nullptr);
   }

   {
      // mExactTransports - search for matching port on a specific interface for IPv4.
      resipCout << "test transport selection by source V4 interface lookup - exact matching" << std::endl;

      TestTransportSelector ts;
      ts.addTransport("192.168.1.1", 5060, V4, UDP);

      Tuple tuple1 { "192.168.1.1", 5060, V4, UDP };
      Transport *t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple tuple2 { "192.168.1.1", 5100, V4, UDP };
      t = ts.findTransportBySource(tuple2, &msg);
      resip_assert(t == nullptr);

      Tuple tuple3 { "192.168.1.2", 5060, V4, UDP };
      t = ts.findTransportBySource(tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple tuple4 { "192.168.1.1", 5060, V4, TCP };
      t = ts.findTransportBySource(tuple4, &msg);
      resip_assert(t == nullptr);

#ifdef USE_IPV6
      Tuple v6tuple { "fe80::a00:27ff:fea3:e60e", 5060, V6, UDP };
      t = ts.findTransportBySource(v6tuple, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // mExactTransports - search for matching port on a specific interface for IPv6.
#ifdef USE_IPV6
      resipCout << "test transport selection by source V6 interface lookup - exact matching" << std::endl;

      TestTransportSelector ts;
      ts.addTransport("fe80::a00:27ff:fea3:e60e", 5060, V6, UDP);

      Tuple v6tuple1 { "fe80::a00:27ff:fea3:e60e", 5060, V6, UDP };
      Transport *t = ts.findTransportBySource(v6tuple1, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple v6tuple2 { "fe80::a00:27ff:fea3:e60e", 5100, V6, UDP };
      t = ts.findTransportBySource(v6tuple2, &msg);
      resip_assert(t == nullptr);

      Tuple v6tuple3 { "fe80::a00:27ff:fea3:1111", 5100, V6, UDP };
      t = ts.findTransportBySource(v6tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple v6tuple4 { "fe80::a00:27ff:fea3:e60e", 5060, V6, TCP };
      t = ts.findTransportBySource(v6tuple4, &msg);
      resip_assert(t == nullptr);

      Tuple tuple { "192.168.1.1", 5060, V4, UDP };
      t = ts.findTransportBySource(tuple, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // mExactTransports - search for matching port on a specific interface.
      resipCout << "test transport selection by source exact lookup - multiple transports" << std::endl;

      TestTransportSelector ts;
      ts.addTransport("192.168.1.1", 5060, V4, UDP);
      ts.addTransport("192.168.1.1", 5100, V4, UDP);

      Tuple tuple1 { "192.168.1.1", 5060, V4, UDP };
      Transport *t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple tuple2 { "192.168.1.1", 5100, V4, UDP };
      t = ts.findTransportBySource(tuple2, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5100);

      Tuple tuple3 { "192.168.1.2", 5100, V4, UDP };
      t = ts.findTransportBySource(tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple tuple4 { "192.168.1.2", 5200, V4, UDP };
      t = ts.findTransportBySource(tuple4, &msg);
      resip_assert(t == nullptr);
   }

   {
      // FindLoopbackTransportBySource - search for matching port on any loopback interface for IPv4.
      resipCout << "test transport selection by source loopback V4 interface lookup - exact port" << std::endl;

      TestTransportSelector ts;
      ts.addTransport("127.0.0.1", 5060, V4, UDP);

      Tuple tuple1 { "127.0.0.1", 5060, V4, UDP };
      Transport *t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple tuple2 { "127.0.0.50", 5060, V4, UDP };
      t = ts.findTransportBySource(tuple2, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple tuple3 { "127.0.0.1", 5060, V4, TCP };
      t = ts.findTransportBySource(tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple tuple4 { "192.168.1.2", 5060, V4, UDP };
      t = ts.findTransportBySource(tuple4, &msg);
      resip_assert(t == nullptr);

#ifdef USE_IPV6
      Tuple v6tuple { "::1", 5060, V6, UDP };
      t = ts.findTransportBySource(v6tuple, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // FindLoopbackTransportBySource - search for matching port on any loopback interface for IPv6.
      // TODO: not yet implemented.
   }

   {
      // mAnyInterfaceTransports - search for specific port on ANY interface for IPv4.
      resipCout << "test transport selection by source any V4 interface lookup - any host" << std::endl;

      TestTransportSelector ts;
      ts.addTransport(Data::Empty, 5060, V4, UDP);

      Tuple tuple1 { "192.168.1.1", 5060, V4, UDP };
      Transport *t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t != nullptr);

      Tuple tuple2 { "192.168.1.2", 5060, V4, UDP };
      t = ts.findTransportBySource(tuple2, &msg);
      resip_assert(t != nullptr);

      Tuple tuple3 { "192.168.1.1", 5061, V4, UDP };
      t = ts.findTransportBySource(tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple tuple4 { "192.168.1.1", 5060, V4, TCP };
      t = ts.findTransportBySource(tuple4, &msg);
      resip_assert(t == nullptr);

#ifdef USE_IPV6
      Tuple v6tuple { "fe80::a00:27ff:fea3:e60e", 5060, V6, UDP };
      t = ts.findTransportBySource(v6tuple, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // mAnyInterfaceTransports - search for specific port on ANY interface for IPv6.
#ifdef USE_IPV6
      resipCout << "test transport selection by source any V6 interface lookup - any host" << std::endl;

      TestTransportSelector ts;
      ts.addTransport(Data::Empty, 5060, V6, UDP);

      Tuple v6tuple1 { "fe80::a00:27ff:fea3:e60e", 5060, V6, UDP };
      Transport *t = ts.findTransportBySource(v6tuple1, &msg);
      resip_assert(t != nullptr);

      Tuple v6tuple2 { "fe80::a00:27ff:fea3:1111", 5060, V6, UDP };
      t = ts.findTransportBySource(v6tuple2, &msg);
      resip_assert(t != nullptr);

      Tuple v6tuple3 { "fe80::a00:27ff:fea3:e60e", 5061, V6, UDP };
      t = ts.findTransportBySource(v6tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple v6tuple4 { "fe80::a00:27ff:fea3:e60e", 5060, V6, TCP };
      t = ts.findTransportBySource(v6tuple4, &msg);
      resip_assert(t == nullptr);

      Tuple tuple { "192.168.1.1", 5060, V4, UDP };
      t = ts.findTransportBySource(tuple, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // mAnyPortTransports - search for ANY port on specific interface for IPv4.
      resipCout << "test transport selection by source specific V4 interface lookup - any port" << std::endl;

      TestTransportSelector ts;
      ts.addTransport("192.168.1.1", 5060, V4, UDP);

      Tuple tuple1 { "192.168.1.1", 0, V4, UDP };
      Transport *t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple tuple2 { "192.168.1.2", 0, V4, UDP };
      t = ts.findTransportBySource(tuple2, &msg);
      resip_assert(t == nullptr);

      Tuple tuple3 { "192.168.1.1", 0, V4, TCP };
      t = ts.findTransportBySource(tuple3, &msg);
      resip_assert(t == nullptr);

#ifdef USE_IPV6
      Tuple v6tuple1 { "fe80::a00:27ff:fea3:e60e", 0, V6, UDP };
      t = ts.findTransportBySource(v6tuple1, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // mAnyPortTransports - search for ANY port on specific interface for IPv6.
#ifdef USE_IPV6
      resipCout << "test transport selection by source specific V6 interface lookup - any port" << std::endl;

      TestTransportSelector ts;
      ts.addTransport("fe80::a00:27ff:fea3:e60e", 5060, V6, UDP);

      Tuple v6tuple1 { "fe80::a00:27ff:fea3:e60e", 0, V6, UDP };
      Transport *t = ts.findTransportBySource(v6tuple1, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple v6tuple2 { "fe80::a00:27ff:fea3:1111", 0, V6, UDP };
      t = ts.findTransportBySource(v6tuple2, &msg);
      resip_assert(t == nullptr);

      Tuple v6tuple3 { "fe80::a00:27ff:fea3:e60e", 0, V6, TCP };
      t = ts.findTransportBySource(v6tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple tuple { "192.168.1.1", 0, V4, UDP };
      t = ts.findTransportBySource(tuple, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // FindLoopbackTransportBySource (ignorePort) - search for ANY port on any loopback interface for IPv4.
      resipCout << "test transport selection by source loopback V4 interface lookup - any port" << std::endl;

      TestTransportSelector ts;
      ts.addTransport("127.0.0.1", 5060, V4, UDP);

      Tuple tuple1 { "127.0.0.1", 0, V4, UDP };
      Transport *t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple tuple2 { "127.0.0.50", 0, V4, UDP };
      t = ts.findTransportBySource(tuple2, &msg);
      resip_assert(t != nullptr);
      resip_assert(t->port() == 5060);

      Tuple tuple3 { "127.0.0.1", 0, V4, TCP };
      t = ts.findTransportBySource(tuple3, &msg);
      resip_assert(t == nullptr);

      Tuple tuple4 { "192.168.1.2", 0, V4, UDP };
      t = ts.findTransportBySource(tuple4, &msg);
      resip_assert(t == nullptr);

#ifdef USE_IPV6
      Tuple v6tuple1 { "::1", 0, V6, UDP };
      t = ts.findTransportBySource(v6tuple1, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // FindLoopbackTransportBySource (ignorePort) - search for ANY port on any loopback interface for IPv6.
      // TODO: not yet implemented.
   }

   {
      // mAnyPortAnyInterfaceTransports - search for ANY port on ANY interface for IPv4.
      resipCout << "test transport selection by source any V4 interface lookup - any host and any port" << std::endl;

      TestTransportSelector ts;
      ts.addTransport(Data::Empty, 5060, V4, UDP);

      Tuple tuple1 { "192.168.1.1", 0, V4, UDP };
      Transport *t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t != nullptr);

      Tuple tuple2 { "192.168.1.1", 0, V4, TCP };
      t = ts.findTransportBySource(tuple2, &msg);
      resip_assert(t == nullptr);

#ifdef USE_IPV6
      Tuple v6tuple1 { "fe80::a00:27ff:fea3:e60e", 0, V6, TCP };
      t = ts.findTransportBySource(v6tuple1, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }

   {
      // mAnyPortAnyInterfaceTransports - search for ANY port on ANY interface for IPv6.
#ifdef USE_IPV6
      resipCout << "test transport selection by source any V6 interface lookup - any host and any port" << std::endl;

      TestTransportSelector ts;
      ts.addTransport(Data::Empty, 5060, V6, TCP);

      Tuple v6tuple1 { "fe80::a00:27ff:fea3:e60e", 0, V6, TCP };
      Transport *t = ts.findTransportBySource(v6tuple1, &msg);
      resip_assert(t != nullptr);

      Tuple v6tuple2 { "fe80::a00:27ff:fea3:e60e", 0, V6, UDP };
      t = ts.findTransportBySource(v6tuple2, &msg);
      resip_assert(t == nullptr);

      Tuple tuple1 { "192.168.1.1", 0, V4, TCP };
      t = ts.findTransportBySource(tuple1, &msg);
      resip_assert(t == nullptr);
#endif // USE_IPV6
   }
}

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   testFindTransportBySource();

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
