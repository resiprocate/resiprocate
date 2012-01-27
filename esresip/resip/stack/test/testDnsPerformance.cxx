/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

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
#include <iomanip>
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
#include "resip/stack/TupleMarkManager.hxx"
#include "resip/stack/MarkListener.hxx"
#include "rutil/dns/RRVip.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/DnsHandler.hxx"

using namespace std;


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

namespace resip
{
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
         while (!isShutdown())
         {
            FdSet fdset;
            mStub.buildFdSet(fdset);
            fdset.selectMilliSeconds(1);
            mStub.process(fdset);
         }
      }

      DnsStub& mStub;
};

class LoadTestDnsHandler : public DnsHandler
{
   public:
      LoadTestDnsHandler(TestDns& dns, int numRuns, int maxOutstanding) :
         mDns(dns),
         mNumRuns(numRuns),
         mCompleted(0),
         mMaxOutstanding(maxOutstanding),
         mStart(resip::Timer::getTimeMs())
      {
         fillOutstandingWindow();
      }

      void handle(DnsResult* result)
      {
         DnsResult::Type type;
         while ((type=result->available()) == DnsResult::Available)
         {
            // eat results
            result->next();
         }
         if (type != DnsResult::Pending)
         {
            delete result;
            ++mMaxOutstanding; // Can schedule more now
            ++mCompleted;
            if(mCompleted%100==0)
            {
               if(mCompleted==mNumRuns)
               {
                  UInt64 finish=resip::Timer::getTimeMs();
                  std::cout << "Done with " << mNumRuns <<" runs!" << std::endl;
                  int runsPerSecond = 1000*mNumRuns/(finish-mStart);
                  std::cout << "Rate was " << runsPerSecond << " runs/sec." << std::endl;
                  exit(0);
               }
               std::cout << mCompleted << " runs completed..." << std::endl;
            }
            fillOutstandingWindow();
         }
      }

      void rewriteRequest(const Uri& rewrite)
      {
      }

      void fillOutstandingWindow()
      {
         while(mMaxOutstanding && mCompleted < mNumRuns)
         {
            DnsResult* res = mDns.createDnsResult(this);
            static const Data prefix("+1972");
            resip::Uri uri;
            uri.scheme()="tel";
            uri.user()=prefix;
            resip::Data digits(resip::Random::getRandom());
            if(digits.size()>7)
            {
               digits.truncate(7);
            }
            while(digits.size()<7)
            {
               digits+='0';
            }
            uri.user()+=digits;
            mDns.lookup(res, uri);
            --mMaxOutstanding;
         }
      }
      
   private:
      TestDns& mDns;
      unsigned int mNumRuns;
      unsigned int mCompleted;
      unsigned int mMaxOutstanding;
      UInt64 mStart;
};
}

int main(int argc, const char** argv)
{
   int numRuns=10000;
   int windowSize=500;
   const char* logType = "cout";
   const char* logLevel = "WARNING";
   resip::Log::initialize(logType, logLevel, argv[0]);
   resip::initNetwork();
   resip::DnsStub* stub = new resip::DnsStub;
   std::vector<resip::Data> enumSuffixes;
   enumSuffixes.push_back("test.estacado.net");
   stub->setEnumSuffixes(enumSuffixes);
   resip::TestDns dns(*stub);
   dns.run();
   sleep(1); // Give some time for the enum suffixes to stick.
   cerr << "Starting" << endl;
   resip::LoadTestDnsHandler loadTest(dns,numRuns,windowSize);
   sleep(3000);
   dns.shutdown();
   dns.join();
   delete stub;

   return -1;
}
