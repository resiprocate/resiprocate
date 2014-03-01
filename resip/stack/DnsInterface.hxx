#if !defined(RESIP_DNSINTERFACE_HXX)
#define RESIP_DNSINTERFACE_HXX 

#include <set>
#include <vector>

#include "rutil/TransportType.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/RRVip.hxx"
#include "resip/stack/TupleMarkManager.hxx"

namespace resip
{
class DnsHandler;
class DnsResultSink;
class DnsResult;
class Uri;
class Via;
class DnsRawSink;

class DnsInterface
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line) : BaseException(msg,file,line){}
            const char* name() const { return "DnsInterface::Exception"; }
      };

      // Used to create an asynchronous Dns Interface. Any lookup requests will
      // be queued for later processing. It is critical that the consumer of the
      // DnsResult be in the same thread that is processing the async results
      // since there is no locking on the DnsResult
      // Will throw DnsInterface::Exception if the Dns provider fails to initialize
      DnsInterface(DnsStub& dnsStub);

      virtual ~DnsInterface();

      //Data errorMessage(int status);

      // set the supported set of types that a UAC wishes to use
      void addTransportType(TransportType type, IpVersion version);
      void removeTransportType(TransportType type, IpVersion version);

      // return if the client supports the specified service (e.g. SIP+D2T)
      bool isSupported(const Data& service);
      bool isSupported(TransportType t, IpVersion version);

      // this is used if NAPTR doesn't return anything to decide which SRV
      // records to query
      bool isSupportedProtocol(TransportType t);
      int supportedProtocols();

      // For each of the following calls, immediately return a DnsResult to the
      // caller. If synchronous, the DnsResult is complete and may block for an
      // arbitrary amount of time. In the synchronous case, the transactionId is
      // not useful. If asynchronous, the DnsResult will be returned immediately
      // and is owned by the caller. If queries are outstanding, it is not valid
      // for the caller to delete the DnsResult.
      // 
      // First determine a transport.  Second, determine a set of ports and ip
      // addresses. These can be returned to the client by asking the DnsResult
      // for the next result in the form of a Transport:Tuple. The client can
      // continue to ask the DnsResult to return more tuples. If the tuples for
      // the current transport are exhausted, move on to the next preferred
      // transport (if there is one)

      DnsResult* createDnsResult(DnsHandler* handler=0);
      void lookup(DnsResult* res, const Uri& uri);

      TupleMarkManager& getMarkManager(){return mMarkManager;}

      bool setUdpOnlyOnNumeric(bool value)
      {
         mUdpOnlyOnNumeric = value;
         return mUdpOnlyOnNumeric;
      }

      bool getUdpOnlyOnNumeric() const
      {
         return mUdpOnlyOnNumeric;
      }

   protected: 
      const Data* getSupportedNaptrType(TransportType type);
      void logSupportedTransports();

      // When complete or partial results are ready, call DnsHandler::process()
      // For synchronous DnsInterface, set to 0
      friend class DnsResult;
      Mutex mSupportedMutex; // Protects mSupportedNaptrs and mSupportTransports
      typedef std::map<Data, unsigned int> SupportedNaptrMap; // second is for RefCount
      SupportedNaptrMap mSupportedNaptrs;  
      typedef std::map<std::pair<TransportType, IpVersion>, unsigned int> TransportMap; // second is for RefCount
      TransportMap mSupportedTransports;
      // Whether transport failover should be disabled on URIs with only a numeric
      // IP address (only UDP will ever be attempted).
      bool mUdpOnlyOnNumeric;

      DnsStub& mDnsStub;  
      RRVip mVip;                      // Ensure all access is from DnsThread/DnsStub fifo for thread safety
      TupleMarkManager mMarkManager;   // Ensure all access is from DnsThread/DnsStub fifo for thread safety
};

}

#endif
//  Copyright (c) 2003, Jason Fischl
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

