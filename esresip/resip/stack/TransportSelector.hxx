/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_TRANSPORTSELECTOR_HXX)
#define RESIP_TRANSPORTSELECTOR_HXX

#ifndef WIN32
#include <sys/select.h>
#endif

#include <map>
#include <vector>

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "resip/stack/SelectInterruptor.hxx"


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

*/
class TransportSelector 
{
   public:
      TransportSelector(Fifo<TransactionMessage>& fifo, Security* security, DnsStub& dnsStub, Compression &compression);
      virtual ~TransportSelector();
      /**
	    @retval true	Some transport in the transport list has data to send
	    @retval false	No transport in the transport list has data to send
	  */
      bool hasDataToSend() const;
      
      /**
		Shuts down all transports.
	  */
      void shutdown();
      
      /// Returns true if all Transports have their buffers cleared, false otherwise.
      bool isFinished() const;
      
      /// Calls process on all suitable transports and the DNSInterface
      void process(FdSet& fdset);
      /// Builds an FdSet comprised of all FDs from all suitable Transports and the DNSInterface
      void buildFdSet(FdSet& fdset);

      /// Causes transport process loops to be interrupted if there is stuff in
      /// their transmit fifos.
      void poke();
      void addTransport( std::auto_ptr<Transport> transport, bool immediate);

      DnsResult* createDnsResult(DnsHandler* handler);

      void dnsResolve(DnsResult* result, SipMessage* msg);

      /**
       Results in msg->resolve() being called to either
       kick off dns resolution or to pick the next tuple and will cause the
       message to be encoded and via updated
	  */
      std::auto_ptr<SendData> transmit( SipMessage* msg, Tuple& target );

      /// Resend to the same transport as last time
      void retransmit(const SendData* msg);

      void closeConnection(const Tuple& peer);

      unsigned int sumTransportFifoSizes() const;

      unsigned int getTimeTillNextProcessMS();
      Fifo<TransactionMessage>& stateMacFifo() { return mStateMacFifo; }

      void registerMarkListener(MarkListener* listener);
      void unregisterMarkListener(MarkListener* listener);
      void setEnumSuffixes(const std::vector<Data>& suffixes);

      static Tuple getFirstInterface(bool is_v4, TransportType type);

      inline void setFilloutContacts(bool filloutContacts)
      {
         mFilloutContacts=filloutContacts;
      }
      
      inline bool getFilloutContacts() const
      {
         return mFilloutContacts;
      }

      void setCongestionManager(CongestionManager* manager)
      {
         for(TransportList::iterator i=mSharedProcessTransports.begin();
               i!=mSharedProcessTransports.end();++i)
         {
            (*i)->setCongestionManager(manager);
         }

         for(TransportList::iterator i=mHasOwnProcessTransports.begin();
               i!=mHasOwnProcessTransports.end();++i)
         {
            (*i)->setCongestionManager(manager);
         }
      }

      inline const resip::Data& getFQDN() const { return mFQDN;} 
      inline void setFQDN(const resip::Data& pFQDN) { mFQDN = pFQDN;}

   private:
      void addTransportInternal( std::auto_ptr<Transport> transport);
      Transport* findTransportBySource(Tuple& src, const SipMessage* msg);
      Transport* findTransportByDest(const Tuple& dest);
      Transport* findTlsTransport(const Data& domain,TransportType type,IpVersion ipv);
      Transport* findLoopbackTransport(Tuple& src, bool ignorePort);
      Transport* findTransportByHerculeanEffort(Tuple& search, bool ignorePort);
      void determineSourceInterface(SipMessage* msg, 
                                    const Tuple& dest,
                                    Tuple& source) const;

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

      std::vector<Transport*> mTransports;

      /**
         @internal
      */
      class TlsTransportKey
      {
         public:
            TlsTransportKey(const resip::Data& domain, resip::TransportType type, resip::IpVersion version)
               :mDomain(domain),
               mType(type),
               mVersion(version)
            {}
            
            TlsTransportKey(const TlsTransportKey& orig)
            {
               mDomain=orig.mDomain;
               mType=orig.mType;
               mVersion=orig.mVersion;
            }
            
            ~TlsTransportKey(){}
            bool operator<(const TlsTransportKey& rhs) const
            {
               if(mDomain < rhs.mDomain)
               {
                  return true;
               }
               else if(mDomain == rhs.mDomain)
               {
                  if(mType < rhs.mType)
                  {
                     return true;
                  }
                  else if(mType == rhs.mType)
                  {
                     return mVersion < rhs.mVersion;
                  }
               }
               return false;
            }
            
            resip::Data mDomain;
            resip::TransportType mType;
            resip::IpVersion mVersion;
         
         private:
            TlsTransportKey();
      };
      
      typedef std::map<TlsTransportKey, Transport*> TlsTransportMap ;
      
      TlsTransportMap mTlsTransports;

      typedef std::vector<Transport*> TransportList;
      TransportList mSharedProcessTransports;
      TransportList mHasOwnProcessTransports;

      typedef std::multimap<Tuple, Transport*, Tuple::AnyPortAnyInterfaceCompare> TypeToTransportMap;
      TypeToTransportMap mTypeToTransportMap;

      // fake socket for connect() and route table lookups
      mutable Socket mSocket;
      mutable Socket mSocket6;
      
      // An AF_UNSPEC addr_in for rapid unconnect
      GenericIPAddress mUnspecified;
      GenericIPAddress mUnspecified6;

      /// SigComp configuration object
      Compression &mCompression;
      osc::Stack  *mSigcompStack;

      // ?bwc? Should we allow this to be set on a per-TU basis, or even a per
      // message basis?
      /// Should stack attempt to fill out incomplete Contact headers?
      bool mFilloutContacts;
      resip::Data mFQDN;
      
      int mAvgBufferSize;
      Data mEncodeBuffer;
      oDataStream mEncodeStream;
      Fifo<Transport> mTransportsToAdd;
      SelectInterruptor mSelectInterruptor;


      friend class TestTransportSelector;
      friend class SipStack; // for debug only
};

}

#endif

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
