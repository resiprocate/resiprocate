#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include <iostream>
#include <list>

#include "rutil/Lock.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/DnsUtil.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/dns/RRVip.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/DnsHandler.hxx"

using namespace std;


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

const char bf[] = "\033[01;34m";
const char gf[] = "\033[01;32m";
const char rf[] = "\033[01;31m";
const char ub[] = "\033[01;00m";

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#define sleep(x) Sleep(x*1000)
#endif

namespace resip
{

// called on the DNS processing thread, therefore state must be locked
class TestDnsHandler : public DnsHandler
{
   public:
      TestDnsHandler() : mComplete(false) {}
      void handle(DnsResult* result)
      {
         
         std::cout << gf << "DnsHandler received " <<  result->target() << ub <<  std::endl;
         Lock lock(mutex);
         DnsResult::Type type;
         while ((type=result->available()) == DnsResult::Available)
         {
            Tuple tuple = result->next();
            results.push_back(tuple);
            std::cout << gf << result->target() << " -> " << tuple << ub <<  std::endl;
         }
         if (type != DnsResult::Pending)
         {
            mComplete = true;
         }         
      }
      void rewriteRequest(const Uri& rewrite)
      {
         std::cout << "Rewriting uri (enum) to " << rewrite << std::endl;
      }
      

      bool complete()
      {
         Lock lock(mutex);
         return mComplete;
      }

      std::vector<Tuple> results;

   private:
      bool mComplete;
      Mutex mutex;
};

/*
class VipListener : public RRVip::Listener
{
   void onVipInvalidated(int rrType, const Data& vip) const
   {
      cout << rf << "VIP " << " -> " << vip << " type" << " -> " << rrType << " has been invalidated." << ub << endl;
   }
};
*/
	
class TestDns : public DnsInterface, public ThreadIf
{
   public:
      TestDns(DnsStub& stub) : DnsInterface(stub), mStub(stub)
      {
         addTransportType(TCP, V4);
         addTransportType(UDP, V4);
         addTransportType(TLS, V4);
#ifdef IPV6
         addTransportType(TCP, V6);
         addTransportType(UDP, V6);
         addTransportType(TLS, V6);
#endif
      }

      void thread()
      {
         while (!waitForShutdown(100))
         {
            FdSet fdset;
            buildFdSet(fdset);
            mStub.buildFdSet(fdset);
            fdset.selectMilliSeconds(1);
            process(fdset);
            mStub.process(fdset);
         }
      }

      DnsStub& mStub;
};
 
}

using namespace resip;

typedef struct 
{
      DnsResult* result;
      Uri uri;
      TestDnsHandler* handler;
      //VipListener* listener;
} Query;

int 
main(int argc, const char** argv)
{
   char* logType = "cout";
   char* logLevel = "STACK";
   char* enumSuffix = "e164.arpa";
   
#if defined(HAVE_POPT_H)
  struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"enum-suffix",   'e', POPT_ARG_STRING, &enumSuffix,  0, "specify what enum domain to search in", "e164.arpa"},      
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif

   Log::initialize(logType, logLevel, argv[0]);
   initNetwork();
   DnsStub* stub = new DnsStub;
   TestDns dns(*stub);
   dns.run();   
   Uri uri;
   cerr << "Starting" << endl;   
   std::list<Query> queries;
#if defined(HAVE_POPT_H)
   const char** args = poptGetArgs(context);
#else
   const char** args = argv;
#endif

   std::vector<Data> enumSuffixes;
   enumSuffixes.push_back(enumSuffix);
   stub->setEnumSuffixes(enumSuffixes);

   // default query: sip:yahoo.com
   if (argc == 1)
   {
      Query query;
      query.handler = new TestDnsHandler;
      
      //query.listener = new VipListener;
      cerr << "Creating Uri" << endl;       
      uri = Uri("sip:yahoo.com");
      query.uri = uri;
      cerr << "Creating DnsResult" << endl;      
      DnsResult* res = dns.createDnsResult(query.handler);
      query.result = res;      
      queries.push_back(query);
      cerr << rf << "Looking up" << ub << endl;

      dns.lookup(res, uri);
   }

   while (argc > 1 && args && *args != 0)
   {
      Query query;
      query.handler = new TestDnsHandler;
      //query.listener = new VipListener;
      cerr << "Creating Uri: " << *args << endl;       
      try
      {
         Data input(*args++);
         uri = Uri(input);
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
      }
      catch (ParseBuffer::Exception& e)
      {
         cerr << "Couldn't parse arg " << *(args-1) << ": " << e.getMessage() << endl;
      }
      
      argc--;
   }

   int count = queries.size();
   while (count>0)
   {
      for (std::list<Query>::iterator it = queries.begin(); it != queries.end(); ++it)
      {
         if ((*it).handler->complete())
         {
            --count;
         }
      }
      sleep(1);
   }

   for (std::list<Query>::iterator it = queries.begin(); it != queries.end(); ++it)
   {
      cerr << rf << "DNS results for " << (*it).uri << ub << endl;
      for (std::vector<Tuple>::iterator i = (*it).handler->results.begin(); i != (*it).handler->results.end(); ++i)
      {
         cerr << rf << (*i) << ub << endl;
      }
   }

   for (std::list<Query>::iterator it = queries.begin(); it != queries.end(); ++it)
   {
      (*it).result->destroy();
      delete (*it).handler;
      //delete (*it).listener;
   }

   dns.shutdown();
   dns.join();

   delete stub;

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
