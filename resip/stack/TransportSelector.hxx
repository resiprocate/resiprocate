#if !defined(RESIP_TRANSPORTSELECTOR_HXX)
#define RESIP_TRANSPORTSELECTOR_HXX

#ifndef WIN32
#include <sys/select.h>
#endif

#include <map>
#include <vector>
#include <list>

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "rutil/SelectInterruptor.hxx"


#include "resip/stack/SecurityTypes.hxx"
class TestTransportSelector;

namespace osc
{
  class Stack;
}

namespace resip
{

class DnsHandler;
class Message;
class TransactionMessage;
class SipMessage;
class TransactionController;
class Security;
class Compression;
class FdPollGrp;

/**
   @internal

  TransportSelector has two distinct roles.  The first is transmit on the best
outgoing Transport for a given SipMessage based on the target's TransportType
and the hints present in the topmost Via in the SipMessage.  The second role
is to hold all Transports added to the SipStack, and if a given Transport returns
true for shareStackProcessAndSelect(), TransportSelector will include the
Transport in the FdSet processing loop.  If the Transport returns false for
shareStackProcessAndSelect(), TransportSelector will call startOwnProcessing
on Transport add.

Thread Safey Notes - the majority of the access to this class is expected to be from
the TransactionController Thread.  The TransportSelectorThread itself is used 
to provide cycles to the actual transports for sending data in their Fifo's and
receiving data from the wire.  The mSharedProcessTransports list is one member that
is expected to be accessed from TransportSelector processing loop only , all other 
members are accessed from the TransactionController processing loop.
*/
class TransportSelector
{
   public:
      TransportSelector(Fifo<TransactionMessage>& fifo, Security* security, DnsStub& dnsStub, Compression &compression);
      virtual ~TransportSelector();

      /**
        @retval true	Some transport in the transport list has data to send
        @retval false	No transport in the transport list has data to send
        Should only be called from TransportSelector processing loop.
      */
      bool hasDataToSend() const;

      /// Shuts down all transports.
      void shutdown();

      /// Returns true if all Transports have their buffers cleared, false otherwise.
      bool isFinished() const;

      /// Configure a PollGrp to use (instead of buildFdSet/process)
      /// Must be called before adding any transports
      void setPollGrp(FdPollGrp *pollGrp);

      /// Called when the TransportSelector will be running in a different 
      // thread than the TransactionController; this will allow the thread to be 
      // interrupted when poke() is called.
      void createSelectInterruptor();

      /// Calls process on all suitable transports
      /// NOTE that TransportSelector no longer handles DNSInterface
      /// NOTE not used with pollGrp
      void process(FdSet& fdset);
      void process();
      /// Builds an FdSet comprised of all FDs from all suitable Transports
      void buildFdSet(FdSet& fdset);

      /// Causes transport process loops to be interrupted if there is stuff in
      /// their transmit fifos.
      void poke();

      /// Add/Remove a transport
      void addTransport(std::auto_ptr<Transport> transport, bool isStackRunning);
      void removeTransport(unsigned int transportKey);

      /// DNS Resolution
      DnsResult* createDnsResult(DnsHandler* handler);
      void dnsResolve(DnsResult* result, SipMessage* msg);

      /**
       transmit results in msg->resolve() being called to either
       kick off dns resolution or to pick the next tuple and will cause the
       message to be encoded and via updated
      */
      typedef enum
      {
         Unsent,
         Sent
      } TransmitState;
      TransmitState transmit( SipMessage* msg, Tuple& target, SendData* sendData=0 );

      /// Resend to the same transport as last time
      void retransmit(const SendData& msg);

      void closeConnection(const Tuple& peer);

      unsigned int sumTransportFifoSizes() const;

      unsigned int getTimeTillNextProcessMS();
      Fifo<TransactionMessage>& stateMacFifo() { return mStateMacFifo; }

      void registerMarkListener(MarkListener* listener);
      void unregisterMarkListener(MarkListener* listener);
      void setEnumSuffixes(const std::vector<Data>& suffixes);

      static Tuple getFirstInterface(bool is_v4, TransportType type);
      void terminateFlow(const resip::Tuple& flow);
      void enableFlowTimer(const resip::Tuple& flow);

      bool setUdpOnlyOnNumeric(bool value)
      {
         return mDns.setUdpOnlyOnNumeric(value);
      }

      bool getUdpOnlyOnNumeric() const
      {
         return mDns.getUdpOnlyOnNumeric();
      }

      void setCongestionManager(CongestionManager* manager)
      {
         for(TransportKeyMap::iterator i=mTransports.begin();
               i!=mTransports.end();++i)
         {
            i->second->setCongestionManager(manager);
         }
      }

      /**
         @internal - public only for stream operator access
      */
      class TlsTransportKey
      {
         public:
            TlsTransportKey(const resip::Tuple& tuple) : mTuple(tuple) {}
            TlsTransportKey(const resip::Data& domainName, resip::TransportType type, resip::IpVersion version) :
               mTuple(Data::Empty, 0, version, type, domainName) {}
            TlsTransportKey(const TlsTransportKey& orig) { mTuple = orig.mTuple; }
            ~TlsTransportKey(){}

            bool operator<(const TlsTransportKey& rhs) const
            {
               if(mTuple.getTargetDomain() < rhs.mTuple.getTargetDomain())
               {
                  return true;
               }
               else if(mTuple.getTargetDomain() == rhs.mTuple.getTargetDomain())
               {
                  if(mTuple.getType() < rhs.mTuple.getType())
                  {
                     return true;
                  }
                  else if(mTuple.getType() == rhs.mTuple.getType())
                  {
                     return mTuple.ipVersion() < rhs.mTuple.ipVersion();
                  }
               }
               return false;
            }

            resip::Tuple mTuple;

         private:
            TlsTransportKey();
      };

   private:
      void checkTransportAddRemoveQueue();
      Connection* findConnection(const Tuple& dest) const;
      Transport* findTransportBySource(Tuple& src, const SipMessage* msg) const;
      Transport* findLoopbackTransportBySource(bool ignorePort, Tuple& src) const;
      Transport* findTransportByDest(const Tuple& dest);
      Transport* findTransportByVia(SipMessage* msg, const Tuple& dest, Tuple& src) const;
      Transport* findTlsTransport(const Data& domain,TransportType type,IpVersion ipv) const;
      Tuple determineSourceInterface(SipMessage* msg, const Tuple& dest) const;

      DnsInterface mDns;
      Fifo<TransactionMessage>& mStateMacFifo;
      Security* mSecurity;// for computing identity header

      // specific port and interface
      typedef std::map<Tuple, Transport*> ExactTupleMap;
      ExactTupleMap mExactTransports;

      // specific port, ANY interface
      typedef std::map<Tuple, Transport*, Tuple::AnyInterfaceCompare> AnyInterfaceTupleMap;
      AnyInterfaceTupleMap mAnyInterfaceTransports;

      // ANY port, specific interface
      typedef std::map<Tuple, Transport*, Tuple::AnyPortCompare> AnyPortTupleMap;
      AnyPortTupleMap mAnyPortTransports;

      // ANY port, ANY interface
      typedef std::map<Tuple, Transport*, Tuple::AnyPortAnyInterfaceCompare> AnyPortAnyInterfaceTupleMap;
      AnyPortAnyInterfaceTupleMap mAnyPortAnyInterfaceTransports;

      typedef std::map<unsigned int, Transport*> TransportKeyMap;
      TransportKeyMap mTransports; // owns all Transports

      typedef std::map<TlsTransportKey, Transport*> TlsTransportMap ;
      TlsTransportMap mTlsTransports;

      typedef std::list<Transport*> TransportList;
      TransportList mSharedProcessTransports;  // Warning - only access this from the TransportSelector process loop / thread
      TransportList mHasOwnProcessTransports;

      typedef std::multimap<Tuple, Transport*, Tuple::AnyPortAnyInterfaceCompare> TypeToTransportMap;
      TypeToTransportMap mTypeToTransportMap;

      // fake socket(s) one for each netns, for connect() and route table lookups
      mutable HashMap<Data, Socket> mSockets;
      mutable HashMap<Data, Socket> mSocket6s;

      // An AF_UNSPEC addr_in for rapid unconnect
      GenericIPAddress mUnspecified;
      GenericIPAddress mUnspecified6;

      /// SigComp configuration object
      Compression &mCompression;
      osc::Stack  *mSigcompStack;

      // epoll support, for sharedprocess transports
      FdPollGrp* mPollGrp;

      int mAvgBufferSize;
      Fifo<Transport> mTransportsToAddRemove;
      std::auto_ptr<SelectInterruptor> mSelectInterruptor;
      FdPollItemHandle mInterruptorHandle;

      friend class TestTransportSelector;
      friend class SipStack; // for debug only
};

EncodeStream&
operator<<(EncodeStream& ostrm, const TransportSelector::TlsTransportKey& tlsTransportKey);

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

// vim: softtabstop=3:shiftwidth=3:expandtab
