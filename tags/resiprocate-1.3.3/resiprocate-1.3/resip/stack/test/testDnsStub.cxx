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
#include "rutil/DnsUtil.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/dns/RRVip.hxx"
#include "rutil/dns/QueryTypes.hxx"
#include "rutil/dns/DnsStub.hxx"

using namespace std;


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

const char bf[] = "\033[01;34m";
const char gf[] = "\033[01;32m";
const char rf[] = "\033[01;31m";
const char ub[] = "\033[01;00m";

bool gComplete = false;

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#define sleep(x) Sleep(x)
#endif

namespace resip
{

class MyDnsSink : public DnsResultSink
{
   public:
      void onDnsResult(const DNSResult<DnsHostRecord>&);
#ifdef USE_IPV6
      void onDnsResult(const DNSResult<DnsAAAARecord>&);
#endif
      void onDnsResult(const DNSResult<DnsSrvRecord>&);
      void onDnsResult(const DNSResult<DnsNaptrRecord>&);
      void onDnsResult(const DNSResult<DnsCnameRecord>&);
};

void MyDnsSink::onDnsResult(const DNSResult<DnsHostRecord>& result)
{
   cout << "A records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsHostRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         cout << (*it).host() << endl;
      }
   }
   else
   {
      cout << "Dns look up failed:" << result.msg << endl;
   }
   gComplete = true;
}

void MyDnsSink::onDnsResult(const DNSResult<DnsNaptrRecord>& result)
{
   cout << "Naptr records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsNaptrRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         cout << (*it).name() << endl;
      }
   }
   else
   {
      cout << "Dns look up failed:" << result.msg << endl;
   }
   gComplete = true;
}

void MyDnsSink::onDnsResult(const DNSResult<DnsCnameRecord>& result)
{
   cout << "CNAME records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsCnameRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         cout << (*it).cname() << endl;
      }
   }
   else
   {
      cout << "Dns look up failed:" << result.msg << endl;
   }
   gComplete = true;
}

void MyDnsSink::onDnsResult(const DNSResult<DnsSrvRecord>& result)
{
   cout << "SRV records" << endl;
   cout << "Status: " << result.status << endl;
   cout << "Domain: " << result.domain << endl;
   if (result.status == 0)
   {
      for (vector<DnsSrvRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         cout << "Name: " << (*it).name() << endl;
         cout << "Priority: " << (*it).priority() << endl;
         cout << "Weight: " << (*it).weight() << endl;
         cout << "Port: " << (*it).port() << endl;
         cout << "Target: " << (*it).target() << endl;
      }
   }
   else
   {
      cout << "Dns look up failed:" << result.msg << endl;
   }
   gComplete = true;
}

#ifdef USE_IPV6
void MyDnsSink::onDnsResult(const DNSResult<DnsAAAARecord>& result)
{
   gComplete = true;
}
#endif

class TestDns : public ThreadIf, public DnsStub
{
   public:
      TestDns(const DnsStub::NameserverList& additional) : DnsStub(additional)
      {
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

int 
main(int argc, const char** argv)
{
   if (argc < 3) 
   {
      cout << "usage: " << argv[0] << " target" << " type" << endl;
      cout << "Valid type values: " << endl;
      cout << "A Record - 1" << endl;
      cout << "CNAME - 5" << endl;
      cout << "SRV - 33" << endl;
      cout << "NAPTR - 35" << endl;
      cout << "AAAA - 28" << endl;
      return 0;
   }

   DnsStub::NameserverList nameServerList = DnsStub::EmptyNameserverList;
   if(argc == 4)
   {
      Tuple tuple(Data(argv[3]), 0, V4);
      nameServerList.push_back(tuple.toGenericIPAddress());
   }

   char* logType = "cout";
   //char* logLevel = "STACK";
   char* logLevel = "DEBUG";

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
   TestDns dns(nameServerList);
   dns.run();   
   cerr << "Starting" << endl;   
#if defined(HAVE_POPT_H)
   const char** args = poptGetArgs(context);
#else
   const char** args = argv;
#endif

   MyDnsSink sink;

   switch (atoi(argv[2]))
   {
      case 1:
         dns.lookup<RR_A>(argv[1], Protocol::Sip, &sink);
         break;
      case 5:
         dns.lookup<RR_CNAME>(argv[1], Protocol::Sip, &sink);
         break;
#ifdef USE_IPV6
      case 28:
         dns.lookup<RR_AAAA>(argv[1], Protocol::Sip, &sink);
         break;
#endif
      case 33:
         dns.lookup<RR_SRV>(argv[1], Protocol::Sip, &sink);
         break;
      case 35:
         dns.lookup<RR_NAPTR>(argv[1], Protocol::Sip, &sink);
         break;
      default:
         cout << "Invalid Dns type" << endl;
         return 0;
   }

   while (!gComplete)
   {
#ifdef WIN32
      sleep(100);
#else
      sleep(1);
#endif
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
