#if !defined(RESIP_DNSRESOLVER_HXX)
#define RESIP_DNSRESOLVER_HXX 

#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if defined(__linux__) && !defined(USE_ARES)
#define USE_ARES
#endif

#include <list>
#include <set>

#if defined(USE_ARES)
extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}
#endif

#include "resiprocate/os/HashMap.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/Message.hxx"

namespace resip
{
class TransactionController;
class Data;
class Uri;
class Via;

class DnsResolver
{
   public:
      typedef std::list<Transport::Tuple> TupleList;
      typedef std::list<Transport::Tuple>::const_iterator TupleIterator;

      struct Srv
      {
            int priority;
            int weight;
            int port;
            Data host;
            Transport::Type transport;
            
            bool operator<(const Srv& rhs) const
            {
               return priority < rhs.priority;
            }
      };
      typedef std::set<DnsResolver::Srv> SrvSet;
      typedef std::set<DnsResolver::Srv>::const_iterator SrvIterator;

      struct Naptr
      {
            int order;
            int pref;
            Data flags;
            Data service;
            Data regex;
            Data replacement;

            bool operator<(const Naptr& rhs) const
            {
               if (order != rhs.order)
               {
                  return pref < rhs.pref;
               }
               else
               {
                  return order < rhs.order;
               }
            }
      };
      typedef std::set<DnsResolver::Naptr> NaptrSet;
      typedef std::set<DnsResolver::Naptr>::const_iterator NaptrIterator;

      struct Request
      {
            Request(TransactionController& pstack, const Data& ptid, const Data& phost, int pport, Transport::Type ptransport, Data pscheme)
               : controller(pstack),tid(ptid),host(phost),port(pport),transport(ptransport),scheme(pscheme),isFinal(false)
            {
            }
            
            TransactionController& controller;
            Data tid;
            Data host;
            int port;
            Transport::Type transport;
            Data scheme;
            std::list<Transport::Type> otherTransports;
            bool isFinal;
      };
     
      class DnsMessage : public Message
      {
         public:
            DnsMessage(const Data& tid) : mTransactionId(tid), isFinal(false) {}
            virtual const Data& getTransactionId() const { return mTransactionId; }
            virtual Data brief() const;
            virtual std::ostream& encode(std::ostream& strm) const;
            virtual bool isClientTransaction() const;
            
            Data mTransactionId;
            TupleList mTuples;
            SrvSet mSrvs;
            bool isFinal;
      };
      
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line) : BaseException(msg,file,line){}
            const char* name() const { return "DnsResolver::Exception"; }
      };

 
      DnsResolver(TransactionController& controller);
      ~DnsResolver();

      void process(FdSet& fdset);
      void buildFdSet(FdSet& fdset);

      void lookup(const Data& transactionId, const Uri& url);
      void lookup(const Data& transactionId, const Via& via);

      void lookupARecords(const Data& transactionId, 
                          const Data& host, 
                          int port, 
                          Transport::Type transport);
      
      static bool isIpAddress(const Data& data);
      
      // probably not going to be supported by ares so remove
      //void stop(const Data& tid);

   private:
#if defined(USE_ARES)
      ares_channel mChannel;
      static void aresCallbackHost(void *arg, int status, struct hostent* host);
      static void aresCallbackSrvTcp(void *arg, int status, unsigned char *abuf, int alen);
      static void aresCallbackSrvUdp(void *arg, int status, unsigned char *abuf, int alen);
      static void aresCallbackNaptr(void *arg, int status, unsigned char *abuf, int alen);
#endif

      TransactionController& mController;
};
      
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
