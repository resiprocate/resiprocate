#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
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

#include "resiprocate/os/Lock.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/DnsInterface.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/dns/RRVip.hxx"

using namespace std;


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

const char bf[] = "\033[01;34m";
const char gf[] = "\033[01;32m";
const char rf[] = "\033[01;31m";
const char ub[] = "\033[01;00m";

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#define sleep(x) Sleep(x)
#endif

namespace resip
{

class TestDnsHandler : public DnsHandler
{
   public:
      TestDnsHandler() : mComplete(false) {}
      void handle(DnsResult* result)
      {
         Lock lock(mutex);
         (void)lock;
         DnsResult::Type type;
         int count = 0;
         Tuple vip;
         bool whitelisted = false;
         while ((type=result->available()) == DnsResult::Available)
         {
            Tuple tuple = result->next();
            if (count==1)
            {
               result->success();
               vip = tuple;
               whitelisted = true;
            }
            results.push_back(tuple);
            std::cout << gf << result->target() << " -> " << tuple << ub <<  std::endl;
            ++count;
         }
         if (whitelisted)
         {
            cout << "Whitelist " << result->target() << " -> " << vip << std::endl;
            whitelisted = false;
         }
         if (type != DnsResult::Pending)
         {
            mComplete = true;
         }         
      }
      bool complete()
      {
         Lock lock(mutex);
         (void)lock;
         return mComplete;
      }

      std::vector<Tuple> results;

   private:
      bool mComplete;
      Mutex mutex;
};

class VipListener : public RRVip::Listener
{
   void onVipInvalidated(int rrType, const Data& vip) const
   {
      cout << "VIP " << " -> " << vip << " type" << " -> " << rrType << " has been invalidated." << endl;
   }
};
	
class TestDns : public DnsInterface, public ThreadIf
{
   public:
      TestDns()
      {
         addTransportType(TCP, V4);
         addTransportType(UDP, V4);
         addTransportType(TLS, V4);
      }

      void thread()
      {
         while (!waitForShutdown(100))
         {
            FdSet fdset;
            buildFdSet(fdset);
            fdset.selectMilliSeconds(1);
            process(fdset);
         }
      }
};
 
}

using namespace resip;

typedef struct 
{
   DnsResult* res;
   Uri uri;
   TestDnsHandler* handler;
   VipListener* listener;
} Results;

int 
main(int argc, const char** argv)
{
   char* logType = 0;
   char* logLevel = "INFO";

#if defined(HAVE_POPT_H)
  struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif

   Log::initialize(logType, logLevel, argv[0]);
   initNetwork();
   TestDns dns;
   dns.run();   
   Uri uri;
   cerr << "Starting" << endl;   
   std::list<Results> results;
#if defined(HAVE_POPT_H)
   const char** args = poptGetArgs(context);
#else
   const char** args = argv;
#endif

   // default query: sip:www.yahoo.com
   if (argc == 1)
   {
      Results result;
      result.handler = new TestDnsHandler;
      result.listener = new VipListener;
      cerr << "Creating Uri" << endl;       
      uri = Uri("sip:www.yahoo.com");
      result.uri = uri;
      cerr << "Creating DnsResult" << endl;      
      DnsResult* res = dns.createDnsResult(result.handler);
      result.res = res;      
      results.push_back(result);
      cerr << "Looking up" << endl;
      dns.lookup(res, uri);
   }

   while (argc > 1 && args && *args != 0)
   {
      Results result;
      result.handler = new TestDnsHandler;
      result.listener = new VipListener;
      cerr << "Creating Uri" << endl;       
      uri = Uri(*++args);
      result.uri = uri;
      cerr << "Creating DnsResult" << endl;      
      DnsResult* res = dns.createDnsResult(result.handler);
      result.res = res;      
      results.push_back(result);
      cerr << "Looking up" << endl;
      dns.lookup(res, uri);
      argc--;
   }

   for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
   {
      dns.registerVipListener((*it).listener);
   }

   int count = results.size();
   while (count>0)
   {
      for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
      {
         if ((*it).handler->complete())
         {
            --count;
         }
         else
         {
            std::cout << rf << "Waiting for " << *((*it).res) << ub << std::endl;
         }
      }
#ifdef WIN32
      sleep(100);
#else
      sleep(1);
#endif
   }

   for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
   {
      dns.unregisterVipListener((*it).listener);
   }

   for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
   {
      (*it).res->destroy();
      delete (*it).handler;
      delete (*it).listener;
   }

   for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
   {
      (*it).handler = new TestDnsHandler;
      (*it).res = dns.createDnsResult((*it).handler);
      (*it).listener = new VipListener;
      dns.lookup((*it).res, (*it).uri);
   }

   for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
   {
      dns.registerVipListener((*it).listener);
   }

   count = results.size();
   while (count>0)
   {
      for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
      {
         if ((*it).handler->complete())
         {
            --count;
         }
         else
         {
            std::cout << rf << "Waiting for " << *((*it).res) << ub << std::endl;
         }
      }
#ifdef WIN32
      sleep(100);
#else
      sleep(1);
#endif
   }

   for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
   {
      dns.unregisterVipListener((*it).listener);
   }
   
   for (std::list<Results>::iterator it = results.begin(); it != results.end(); ++it)
   {
      (*it).res->destroy();
      delete (*it).handler;
      delete (*it).listener;
   }
   dns.shutdown();
   dns.join();

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
