#if defined(HAVE_CONFIG_H)
#include "config.h"
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

#include "resip/stack/test/tassert.h"
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
      TestDnsHandler() 
      : mComplete(false),
         mCheckExpectedResults(false),
         mPermutationNumber(0)
      {}
      
      TestDnsHandler(const std::vector<Tuple>& expectedResults, 
                     const resip::Uri& uri) 
      : mComplete(false),
         mExpectedResults(expectedResults),
         mCheckExpectedResults(true),
         mUri(uri),
         mPermutationNumber(0)
      {}
      
      TestDnsHandler(const std::vector<Tuple>& expectedResults, 
                     const std::set<Tuple>& resultsToBlacklist,
                     const std::set<Tuple>& resultsToGreylist,
                     const resip::Uri& uri) 
      : mComplete(false),
         mExpectedResults(expectedResults),
         mCheckExpectedResults(true),
         mUri(uri),
         mPermutationNumber(0),
         mResultsToBlacklist(resultsToBlacklist),
         mResultsToGreylist(resultsToGreylist)
      {}
      
      void handle(DnsResult* result)
      {
         
         std::cout << gf << "DnsHandler received " <<  result->target() << ub <<  std::endl;
         Lock lock(mutex);
         DnsResult::Type type;
         while ((type=result->available()) == DnsResult::Available)
         {
            Tuple tuple = result->next();
            results.push_back(tuple);
            resipCout << gf << result->target() << " -> " << tuple << ub <<  std::endl;
            if(mResultsToGreylist.count(tuple)!=0)
            {
               result->greylistLast(Timer::getTimeMs()+15000);
            }

            if(mResultsToBlacklist.count(tuple)!=0)
            {
               result->blacklistLast(Timer::getTimeMs()+15000);
            }
         }
         if (type != DnsResult::Pending)
         {
            mComplete = true;
            if(mCheckExpectedResults)
            {
               checkExpectedResults();
            }
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

      void checkExpectedResults()
      {
         std::cout << "Input Uri was " << mUri << endl;
         tassert(mExpectedResults.size() == results.size());
         tassert_reset();
         std::cout << "Expected " << mExpectedResults.size() << ", got " << results.size() << endl;
         std::vector<Tuple>::const_iterator e;
         std::vector<Tuple>::const_iterator o;
         
         for(e=mExpectedResults.begin();e!=mExpectedResults.end();++e)
         {
            int p=0;
            resipCout << "Looking for " << *e << endl;
            bool found=false;
            for(o=results.begin();(o!=results.end() && !found);++o)
            {
               ++p;
               if(*e==*o)
               {
                  found=true;
                  resipCout << *o << " matched!" << endl;
                  mPermutation.push_back(p);
               }
               else
               {
                  resipCout << *o << " didn't match." << endl;
               }
            }
            
            tassert(found);
            tassert_reset();
         }            
      }

      int getPermutationNumber()
      {
         if(mPermutationNumber!=0)
         {
            return mPermutationNumber;
         }
         
         int result=1;

         // .bwc. Please forgive me for my use of permutation-group-foo.
         for(int i=mPermutation.size();i>0;--i)
         {
            int foundAt=0;
            for(std::list<int>::iterator j=mPermutation.begin();j!=mPermutation.end();++j)
            {
               ++foundAt;
               if(*j==i)
               {
                  result*=((foundAt-i)%i+1);
                  mPermutation.erase(j);
                  j=mPermutation.end();
               }
            }
         }
         
         mPermutationNumber=result;
         return result;
      }

      std::vector<Tuple> results;

   private:
      bool mComplete;
      std::vector<Tuple> mExpectedResults;
      bool mCheckExpectedResults;
      Mutex mutex;
      Uri mUri;
      std::list<int> mPermutation;
      int mPermutationNumber;
      std::set<resip::Tuple> mResultsToBlacklist;
      std::set<resip::Tuple> mResultsToGreylist;
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

class TestMarkListener : public MarkListener
{
   public:
      TestMarkListener(const resip::Tuple& listenFor) 
         : mTuple(listenFor),
         mGotOkCallback(false),
         mGotGreylistCallback(false), 
         mGotBlacklistCallback(false)
      {}
      virtual ~TestMarkListener(){}
      
      virtual void onMark(const Tuple& tuple, UInt64& expiry, TupleMarkManager::MarkType& mark)
      {
         if(mTuple == tuple)
         {
            switch(mark)
            {
               case TupleMarkManager::OK:
                  mGotOkCallback=true;
                  break;
               case TupleMarkManager::GREY:
                  mGotGreylistCallback=true;
                  break;
               case TupleMarkManager::BLACK:
                  mGotBlacklistCallback=true;
                  break;
               default:
                  ;
            }
         }
      }
      
      bool gotOkCallback() const {return mGotOkCallback;}
      bool gotGreylistCallback() const {return mGotGreylistCallback;}
      bool gotBlacklistCallback() const {return mGotBlacklistCallback;}
      
      void resetAll()
      {
         mGotOkCallback=false;
         mGotGreylistCallback=false;
         mGotBlacklistCallback=false;
      }
   private:
      Tuple mTuple;
      bool mGotOkCallback;
      bool mGotGreylistCallback;
      bool mGotBlacklistCallback;
      
};
	
class TestDns : public DnsInterface, public ThreadIf
{
   public:
      TestDns(DnsStub& stub) : DnsInterface(stub), mStub(stub)
      {
         addTransportType(TCP, V4);
         addTransportType(UDP, V4);
         addTransportType(TLS, V4);
#ifdef USE_IPV6
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
            mStub.buildFdSet(fdset);
            fdset.selectMilliSeconds(mStub.getTimeTillNextProcessMS());
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
   const char* logType = "cout";
   const char* logLevel = "STACK";
   const char* enumSuffix = "e164.arpa";
   
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

   resip::Data uris[16]={
      "sip:127.0.0.1:5070;transport=udp",
      "sips:127.0.0.1:5071;transport=tls",
      "sip:127.0.0.1;transport=udp",
      "sips:127.0.0.1;transport=tls",
      "sip:user.test.resiprocate.org:5070;transport=udp",
      "sips:user.test.resiprocate.org:5071;transport=tls",
      "sip:user.test.resiprocate.org;transport=udp",
      "sips:user.test.resiprocate.org;transport=tls",
      "sip:127.0.0.1:5070",
      "sips:127.0.0.1:5071",
      "sip:127.0.0.1",
      "sips:127.0.0.1",
      "sip:user.test.resiprocate.org:5070",
      "sips:user.test.resiprocate.org:5071",
      "sip:user-tcp.test.resiprocate.org",
      "sips:user-tcp.test.resiprocate.org"
   };
   
   
   int expectedPorts[16][3]={
      {5070,0,0},
      {5071,0,0},
      {5060,0,0},
      {5061,0,0},
      {5070,0,0},
      {5071,0,0},
      {5060,5070,5080},
      {5061,5071,5081},
      {5070,0,0},
      {5071,0,0},
      {5060,0,0},
      {5061,0,0},
      {5070,0,0},
      {5071,0,0},
      {5060,5070,5080},
      {5061,5071,5081}
   };
   
   TransportType expectedTransports[16][3]={
      {UDP,UDP,UDP},
      {TLS,TLS,TLS},
      {UDP, UDP, UDP},
      {TLS,TLS,TLS},
      {UDP, UDP, UDP},
      {TLS,TLS,TLS},
      {UDP, UDP, UDP},
      {TLS,TLS,TLS},
      {UDP, UDP, UDP},
      {TLS,TLS,TLS},
      {UDP, UDP, UDP},
      {TLS,TLS,TLS},
      {UDP, UDP, UDP},
      {TLS,TLS,TLS},
      {TCP, TCP, TCP},
      {TLS,TLS,TLS}  
   };
   
   resip::Data subUris[8]={
      "sip:<hostname>:5080;transport=TCP",
      "sips:<hostname>:5081;transport=TLS",
      "sip:<hostname>;transport=TCP",
      "sips:<hostname>;transport=TLS",
      "sip:<hostname>:5080",
      "sips:<hostname>:5081",
      "sip:<hostname>",
      "sips:<hostname>"
   };

   resip::Data ipAddr("127.0.0.1");

   Uri uri;

   for(int i=0; i< 16;++i)
   {      
      Query query;
      std::vector<Tuple> expected;
      
      for(int j=0;j<3;++j)
      {
         if(expectedPorts[i][j]==0)
         {
            break;
         }
         
         expected.push_back(Tuple(ipAddr,expectedPorts[i][j],V4,expectedTransports[i][j]));
      }
      
      
      //query.listener = new VipListener;
      cerr << "Creating Uri " << uris[i] << endl;       
      uri = Uri(uris[i]);

      query.handler = new TestDnsHandler(expected,uri);

      query.uri = uri;
      cerr << "Creating DnsResult" << endl;      
      DnsResult* res = dns.createDnsResult(query.handler);
      query.result = res;      
      queries.push_back(query);
      cerr << rf << "Looking up" << ub << endl;

      dns.lookup(res, uri);
   }
   
   resip::Data NAPTRstrings[3]={
      "",
      "-brokenNAPTR",
      "-noNAPTR"
   };
   
   resip::Data SRVstrings[3]={
      "",
      "-brokenSRV",
      "-noSRV"
   };
   
   
   for(int i=0;i<8;++i)
   {
      
      
      for(int n=0;n<3;++n)
      {
         for(int s=0;s<3;++s)
         {
            if(n==0 && s==0)
            {
               // .bwc. This is just user.test.resiprocate.org, which we have already done.
               continue;
            }
            
            if(n==1 && s==2)
            {
               // .bwc. broken NAPTR and missing SRV is equivalent to OK NAPTR
               // and missing SRV (n==0 and s==2). The former is not provisioned
               // in the DNS zone, but the latter is, so we have already taken 
               // care of this case.
               continue;
            }
            
            
            resip::Data hostname(resip::Data("user")+NAPTRstrings[n]+SRVstrings[s]+resip::Data(".test.resiprocate.org"));
            resip::Data target=subUris[i];
            target.replace("<hostname>",hostname);
            
            //query.listener = new VipListener;
            cerr << "Creating Uri " << target << endl;       
            uri = Uri(target);
            unsigned int port=0;
            TransportType type=UNKNOWN_TRANSPORT;

            // .bwc. Choose expected destination.
            if(uri.exists(p_transport))
            {
               // .bwc. Transport is explicitly specified; no NAPTR query
               // will be made. State of NAPTR is irrelevant.
               
               if(uri.port()!=0)
               {
                  // .bwc. Port is explicitly specified. No SRV query will
                  // be made. This will be a bare A record lookup.
                  port=uri.port();
                  type=toTransportType(uri.param(p_transport));
                  if(uri.scheme()=="sips" && type!=TLS)
                  {
                     // What is the resolver supposed to do in this case?
                     assert(0);
                  }
               }
               else
               {
                  // .bwc. Port is not explicitly specified. SRV query will
                  // be attempted.
                  
                  if(s==0)
                  {
                     // SRV ok. Will land on 127.0.0.1:507[01] on specified
                     // transport.
                     type=toTransportType(uri.param(p_transport));
                     if(type==TLS)
                     {
                        port=5071;
                     }
                     else
                     {
                        port=5070;
                     }
                  }
                  else if(s==1)
                  {
                     // SRV broken. (Exists, so will be followed into space)
                     // Leave port as 0, since no results will come of this.
                  }
                  else
                  {
                     // SRV missing. DNS fill fail over to A record lookup.
                     type=toTransportType(uri.param(p_transport));
                     if(type==TLS)
                     {
                        port=5061;
                     }
                     else
                     {
                        port=5060;
                     }
                  }
               }
            }
            else
            {
               // transport is not specified
               
               if(uri.port()!=0)
               {
                  // Port is specified, so we need to make an A query. We choose
                  // UDP if scheme is sip, and TLS if scheme is sips.
                  port=uri.port();
                  
                  if(uri.scheme()=="sip")
                  {
                     type=UDP;
                  }
                  else if(uri.scheme()=="sips")
                  {
                     type=TLS;
                  }
                  else
                  {
                     assert(0);
                  }
               }
               else
               {
                  // Port is not specified, and neither is transport. Full
                  // NAPTR lookup.
                  
                  if(n==0)
                  {
                     // NAPTR ok.
                     if(s==0)
                     {
                        // SRV ok. We know what we're getting at this point.

                        if(uri.scheme()=="sips")
                        {
                           type=TLS;
                        }
                        else if(uri.scheme()=="sip")
                        {
                           type=TCP;
                        }
                        else
                        {
                           assert(0);
                        }
                     
                        if(type==TLS)
                        {
                           port=5071;
                        }
                        else
                        {
                           port=5070;
                        }
                     }
                     else if(s==1)
                     {
                        // SRV broken. We fail.
                     }
                     else
                     {
                        // SRV missing. Do A lookup, default the port.
                        // (We have already chosen transport)

                        if(uri.scheme()=="sips")
                        {
                           type=TLS;
                        }
                        else if(uri.scheme()=="sip")
                        {
                           type=UDP;
                        }
                        else
                        {
                           assert(0);
                        }

                        if(type==TLS)
                        {
                           port=5061;
                        }
                        else
                        {
                           port=5060;
                        }
                     }
                  }
                  else if(n==1)
                  {
                     // NAPTR is broken. This is the same situation as
                     // missing SRVs.
                     if(uri.scheme()=="sips")
                     {
                        type=TLS;
                     }
                     else if(uri.scheme()=="sip")
                     {
                        type=UDP;
                     }
                     else
                     {
                        assert(0);
                     }

                     if(type==TLS)
                     {
                        port=5061;
                     }
                     else
                     {
                        port=5060;
                     }
                  }
                  else
                  {
                     // NAPTR is missing. Next we try SRV.
                     
                     if(uri.scheme()=="sips")
                     {
                        type=TLS;
                     }
                     
                     if(s==0)
                     {
                        // SRV ok.
                        if(type==TLS)
                        {
                           port=5071;
                        }
                        else
                        {
                           port=5070;
                        }
                     }
                     else if(s==1)
                     {
                        // SRV broken. We are hosed.
                     }
                     else
                     {
                        // SRVs missing. Fail over to A records.
                        if(uri.scheme()=="sips")
                        {
                           type=TLS;
                        }
                        else if(uri.scheme()=="sip")
                        {
                           type=UDP;
                        }
                        else
                        {
                           assert(0);
                        }

                        if(type==TLS)
                        {
                           port=5061;
                        }
                        else
                        {
                           port=5060;
                        }
                        
                     }
                  }
               }
            }


            Query query;
            std::vector<Tuple> expected;
            
            if(port)
            {
               if(type!=UNKNOWN_TRANSPORT)
               {
                  expected.push_back(Tuple(ipAddr,port,V4,type));
               }
               else
               {
                  // .bwc. If we get UNKNOWN_TRANSPORT from the block of
                  // code above, it means we will try all three. (Yes, this
                  // is hackish. At least I documented it.)
                  assert(port%2==0);
                  expected.push_back(Tuple(ipAddr,port,V4,UDP));
                  expected.push_back(Tuple(ipAddr,port,V4,TCP));
                  expected.push_back(Tuple(ipAddr,port+1,V4,TLS));
               }
            }
            
            query.handler = new TestDnsHandler(expected,uri);
            query.uri = uri;
            cerr << "Creating DnsResult" << endl;      
            DnsResult* res = dns.createDnsResult(query.handler);
            query.result = res;      
            queries.push_back(query);
            cerr << rf << "Looking up" << ub << endl;

            dns.lookup(res, uri);
            
         }
      }
   }

   // .bwc. Resolves uris from command line, if they are present.
   while (argc > 1 && args && *args != 0)
   {
      Uri uri;
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
      catch (ParseException& e)
      {
         cerr << "Couldn't parse arg " << *(args-1) << ": " << e.getMessage() << endl;
      }
      
      argc--;
   }

   // .bwc. Wait for outstanding queries to finish.
   int count = queries.size();
   while (count>0)
   {
      for (std::list<Query>::iterator it = queries.begin(); it != queries.end(); )
      {
         if ((*it).handler->complete())
         {
            cerr << rf << "DNS results for " << (*it).uri << ub << endl;
            for (std::vector<Tuple>::iterator i = (*it).handler->results.begin(); i != (*it).handler->results.end(); ++i)
            {
               resipCerr << rf << (*i) << ub << endl;
            }
            
            --count;
            (*it).result->destroy();
            delete (*it).handler;

            std::list<Query>::iterator temp = it;
            ++it;
            queries.erase(temp);
         }
         else
         {
            ++it;
         }
      }
      sleep(1);
   }

   assert(queries.empty());

   std::map<resip::Tuple,unsigned int> ipAddrToNum;
   ipAddrToNum[Tuple("127.0.0.1",5060,V4,TCP)]=0;
   ipAddrToNum[Tuple("127.0.0.2",5060,V4,TCP)]=1;
   ipAddrToNum[Tuple("127.0.0.3",5060,V4,TCP)]=2;
   ipAddrToNum[Tuple("127.0.0.4",5060,V4,TCP)]=3;

   // .bwc. Test load-leveling.
   for(unsigned int numSRV=2;numSRV<5;++numSRV)
   {
      resip::Data hostname("loadlevel");
      hostname+=Data::from(numSRV)+".test.resiprocate.org";
      
      Uri uri;
      uri.host()=hostname;
      uri.scheme()="sip";
      
      for(int i=0; i<1000;++i)
      {
         Query query;                        
         query.handler = new TestDnsHandler();
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
         if(i%20==0)
         {
            // .bwc. Let things have some time to cache, so we don't hammer the
            // DNS to death. (Odds are good that we have hit every NAPTR at
            // least once by now)
            sleep(2);
            stub->logDnsCache();
         }
      }
      
      // .bwc. first index is the order (1st=0, 2nd=1, etc), and second index
      // is the last tuple in the IP address (127.0.0.1 is 0, 127.0.0.2 is 1)
      // The value stored is the number of times this combination was encountered.
#ifdef __GNUC__
      int table[numSRV][numSRV];
#else
      int table[5][5];
#endif
      
      for(unsigned int i=0;i<numSRV;++i)
      {
         for(unsigned int j=0;j<numSRV;++j)
         {
            table[i][j]=0;
         }
      }
      
      int count = queries.size();
      while (count>0)
      {
         for (std::list<Query>::iterator it = queries.begin(); it != queries.end(); )
         {
            if ((*it).handler->complete())
            {
               cerr << rf << "DNS results for " << (*it).uri << ub << endl;
               assert(it->handler->results.size()==numSRV);
               
                for(unsigned int i=0;i<numSRV;++i)
               {
                  assert(ipAddrToNum[it->handler->results[i]] >=0);
                  assert(ipAddrToNum[it->handler->results[i]] <numSRV);
                  ++table[i][ipAddrToNum[it->handler->results[i]]];
                  resipCerr << rf << it->handler->results[i] << ub << endl;
               }
                              
               --count;
               (*it).result->destroy();
               delete (*it).handler;

               std::list<Query>::iterator temp = it;
               ++it;
               queries.erase(temp);
            }
            else
            {
               ++it;
            }
         }
         sleep(1);
      }
      
      assert(queries.empty());
      
      std::cout << "Tabulated results:" << std::endl;
      for(unsigned int i=0;i<numSRV;++i)
      {
         for(unsigned int j=0;j<numSRV;++j)
         {
            std::cout << table[i][j] << std::setw(6);
         }
         std::cout << std::endl;
      }
      
   }


   // .bwc. Test blacklisting
   std::cout << "Testing blacklisting." << std::endl;
   {
      Tuple toBlacklist("127.0.0.1",5060,V4,TCP);
      Tuple ok2("127.0.0.2",5060,V4,TCP);
      Tuple ok3("127.0.0.3",5060,V4,TCP);
      Tuple ok4("127.0.0.4",5060,V4,TCP);
      
      std::vector<Tuple> expected;
      expected.push_back(ok2);
      expected.push_back(ok3);
      expected.push_back(ok4);
      expected.push_back(toBlacklist);
      
      std::set<Tuple> blacklist;
      blacklist.insert(toBlacklist);
      std::set<Tuple> greylist;
      
      Uri uri;
      uri.scheme()="sip";
      uri.host()="loadlevel4.test.resiprocate.org";
      
      TestMarkListener* listener = new TestMarkListener(toBlacklist);
      dns.getMarkManager().registerMarkListener(listener);
      
      Query query;                        
      query.handler = new TestDnsHandler(expected,blacklist,greylist,uri);
      query.uri = uri;
      cerr << "Creating DnsResult" << endl;      
      DnsResult* res = dns.createDnsResult(query.handler);
      query.result = res;      
      queries.push_back(query);
      cerr << rf << "Looking up" << ub << endl;
      dns.lookup(res, uri);
      
      // .bwc. Give this query plenty of time.
      sleep(2);
      
      assert(listener->gotBlacklistCallback());
      listener->resetAll();
      
      // This removes the Tuple toBlacklist
      expected.pop_back();
      
      for(int i=0;i<20;++i)
      {
         Query query;                        
         query.handler = new TestDnsHandler(expected,uri);
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
      }
      
      // .bwc. Wait for blacklist to expire.
      sleep(16);
      
      // Put the blacklisted Tuple back.
      expected.push_back(toBlacklist);
      
      for(int i=0;i<20;++i)
      {
         Query query;                        
         query.handler = new TestDnsHandler(expected,uri);
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
      }
      
      int count = queries.size();
      while (count>0)
      {
         for (std::list<Query>::iterator it = queries.begin(); it != queries.end(); )
         {
            if ((*it).handler->complete())
            {
               cerr << rf << "DNS results for " << (*it).uri << ub << endl;
               for (std::vector<Tuple>::iterator i = (*it).handler->results.begin(); i != (*it).handler->results.end(); ++i)
               {
                  resipCerr << rf << (*i) << ub << endl;
               }
               
               --count;
               (*it).result->destroy();
               delete (*it).handler;
   
               std::list<Query>::iterator temp = it;
               ++it;
               queries.erase(temp);
            }
            else
            {
               ++it;
            }
         }
         sleep(1);
      }
   
      assert(queries.empty());
      assert(listener->gotOkCallback());
      listener->resetAll();
      dns.getMarkManager().unregisterMarkListener(listener);
      delete listener;
   }

   // .bwc. Test greylisting
   std::cout << "Testing greylisting." << std::endl;
   {
      Tuple toGreylist("127.0.0.1",5060,V4,TCP);
      Tuple ok2("127.0.0.2",5060,V4,TCP);
      Tuple ok3("127.0.0.3",5060,V4,TCP);
      Tuple ok4("127.0.0.4",5060,V4,TCP);
      
      std::vector<Tuple> expected;
      expected.push_back(ok2);
      expected.push_back(ok3);
      expected.push_back(ok4);
      expected.push_back(toGreylist);
      
      std::set<Tuple> blacklist;
      std::set<Tuple> greylist;
      greylist.insert(toGreylist);
      
      Uri uri;
      uri.scheme()="sip";
      uri.host()="loadlevel4.test.resiprocate.org";
      
      TestMarkListener* listener = new TestMarkListener(toGreylist);
      dns.getMarkManager().registerMarkListener(listener);
      
      {
         Query query;                        
         query.handler = new TestDnsHandler(expected,blacklist,greylist,uri);
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
      }

      // .bwc. Give this query plenty of time.
      sleep(2);
      
      assert(listener->gotGreylistCallback());
      listener->resetAll();
      
      resipCout << toGreylist << " was greylisted." << std::endl;
      
      for(int i=0;i<20;++i)
      {
         Query query;                        
         query.handler = new TestDnsHandler(expected,uri);
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
      }
      
      // .bwc. Wait for greylist to expire.
      sleep(16);

      {
         Query query;                        
         query.handler = new TestDnsHandler(expected,uri);
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
      }

      // .bwc. Give this query plenty of time.
      sleep(2);
      
      assert(listener->gotOkCallback());
      listener->resetAll();
      resipCout << "greylist on " << toGreylist << " has expired." << std::endl;
      
      for(int i=0;i<20;++i)
      {
         Query query;                        
         query.handler = new TestDnsHandler(expected,uri);
         query.uri = uri;
         cerr << "Creating DnsResult" << endl;      
         DnsResult* res = dns.createDnsResult(query.handler);
         query.result = res;      
         queries.push_back(query);
         cerr << rf << "Looking up" << ub << endl;
         dns.lookup(res, uri);
      }
      
      int count = queries.size();
      while (count>0)
      {
         for (std::list<Query>::iterator it = queries.begin(); it != queries.end(); )
         {
            if ((*it).handler->complete())
            {
               cerr << rf << "DNS results for " << (*it).uri << ub << endl;
               for (std::vector<Tuple>::iterator i = (*it).handler->results.begin(); i != (*it).handler->results.end(); ++i)
               {
                  resipCerr << rf << (*i) << ub << endl;
               }
               
               --count;
               (*it).result->destroy();
               delete (*it).handler;
   
               std::list<Query>::iterator temp = it;
               ++it;
               queries.erase(temp);
            }
            else
            {
               ++it;
            }
         }
         sleep(1);
      }
   
      assert(queries.empty());
      dns.getMarkManager().unregisterMarkListener(listener);
      delete listener;
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
