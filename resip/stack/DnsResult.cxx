#include "precompile.h"

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <algorithm>
#include <stack>

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#else
#include <Winsock2.h>
#include <svcguid.h>
#ifdef USE_IPV6
#include <ws2tcpip.h>
#endif
#endif

#if defined(USE_ARES)
#include "ares.h"
#include "ares_dns.h"
#endif

#include "rutil/DnsUtil.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Random.hxx"
#include "rutil/compat.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/dns/DnsHandler.hxx"
#include "rutil/dns/QueryTypes.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/WinLeakCheck.hxx"


#if !defined(USE_ARES)
#warning "ARES is required"
#endif

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

DnsResult::DnsResult(DnsInterface& interfaceObj, DnsStub& dns, RRVip& vip, DnsHandler* handler) 
   : mInterface(interfaceObj),
     mDns(dns),
     mVip(vip),
     mHandler(handler),
     mSRVCount(0),
     mDoingEnum(false),
     mSips(false),
     mTransport(UNKNOWN_TRANSPORT),
     mPort(-1),
     mType(Pending),
     mBlacklistLastReturnedResult(false)
{
}

DnsResult::~DnsResult()
{
   //DebugLog (<< "DnsResult::~DnsResult() " << *this);
   assert(mType != Pending);
}

void 
DnsResult::transition(Type t)
{
   if ((t == Finished || t == Destroyed || t == Available) &&
       (mType == Pending))
   {
      mInterface.mActiveQueryCount--;
      assert(mInterface.mActiveQueryCount >= 0);
   }
   mType = t;
}

void
DnsResult::destroy()
{
   assert(this);
   //DebugLog (<< "DnsResult::destroy() " << *this);
   
   if (mType == Pending)
   {
      transition(Destroyed);
   }
   else
   {
      delete this;
   }
}

DnsResult::Type
DnsResult::available()
{
   assert(mType != Destroyed);
   if (mType == Available)
   {
      if (!mResults.empty())
      {
         return Available;
      }
      else
      {
         if (mBlacklistLastReturnedResult)
         {
            blacklistLastReturnedResult();
            mBlacklistLastReturnedResult = false;
         }
         primeResults();
         return available(); // recurse
      }
   }
   else
   {
      return mType;
   }
}

Tuple
DnsResult::next() 
{
   assert(available() == Available);
   Tuple next = mResults.front();
   mResults.pop_front();
   StackLog (<< "Returning next dns entry: " << next);
  
   if (mBlacklistLastReturnedResult)
   {
      blacklistLastReturnedResult();
   }
   else if (!mCurrResultPath.empty())
   {
      mBlacklistLastReturnedResult = true;
   }
   mLastReturnedResult = next;

   assert(mCurrSuccessPath.size()<=3);
   Item top;
   if (!mCurrSuccessPath.empty())
   {
      top = mCurrSuccessPath.top();
      if (top.rrType == T_A || top.rrType == T_AAAA)
      {
         mCurrSuccessPath.pop();
      }
   }
   top.domain = next.getTargetDomain();
   top.rrType = next.isV4()? T_A : T_AAAA;
   top.value = Tuple::inet_ntop(next);
   mCurrSuccessPath.push(top);
   return next;
}

void DnsResult::success()
{
   while (!mCurrSuccessPath.empty())
   {
      Item top = mCurrSuccessPath.top();
      DebugLog( << "Whitelisting " << top.domain << "(" << top.rrType << "): " << top.value);
      mVip.vip(top.domain, top.rrType, top.value);
      mCurrSuccessPath.pop();
   }
}

void
DnsResult::lookup(const Uri& uri, const std::vector<Data> &enumSuffixes)
{
   DebugLog (<< "DnsResult::lookup " << uri);
   //int type = this->mType;
   if (!enumSuffixes.empty() && uri.isEnumSearchable())
   {
      mInputUri = uri;
      mDoingEnum = true;
      std::vector<Data> enums = uri.getEnumLookups(enumSuffixes);
      assert(enums.size() <= 1);
      if (!enums.empty())
      {
         InfoLog (<< "Doing ENUM lookup on " << *enums.begin());
         mDns.lookup<RR_NAPTR>(*enums.begin(), Protocol::Enum, this); 
         return;
      }
   }

   mDoingEnum = false;
   lookupInternal(uri);
}

void
DnsResult::lookupInternal(const Uri& uri)
{
   //assert(uri.scheme() == Symbols::Sips || uri.scheme() == Symbols::Sip);  
   mSips = (uri.scheme() == Symbols::Sips);
   mTarget = (!mSips && uri.exists(p_maddr)) ? uri.param(p_maddr) : uri.host();
   mSrvKey = Symbols::UNDERSCORE + uri.scheme().substr(0, uri.scheme().size()) + Symbols::DOT;
   bool isNumeric = DnsUtil::isIpAddress(mTarget);

   if (uri.exists(p_transport))
   {
      mTransport = Tuple::toTransport(uri.param(p_transport));

      if (isNumeric) // IP address specified
      {
         mPort = getDefaultPort(mTransport, uri.port());
         Tuple tuple(mTarget, mPort, mTransport, mTarget);
         DebugLog (<< "Found immediate result: " << tuple);
         mResults.push_back(tuple);
         transition(Available);
         if (mHandler) mHandler->handle(this);         
      }
      else if (uri.port() != 0)
      {
         mPort = uri.port();
         lookupHost(mTarget); // for current target and port
      }
      else 
      { 
         if (mSips)
         {
            if (mTransport == UDP)
            {
               mTransport = DTLS;
               if (!mInterface.isSupportedProtocol(mTransport))
               {
                  transition(Finished);
                  if (mHandler) mHandler->handle(this);
                  return;
               }
               mSRVCount++;
               mDns.lookup<RR_SRV>("_sips._udp." + mTarget, Protocol::Sip, this);
               StackLog (<< "Doing SRV lookup of _sips._udp." << mTarget);
            }
            else
            {
               mTransport = TLS;
               if (!mInterface.isSupportedProtocol(mTransport))
               {
                  transition(Finished);
                  if (mHandler) mHandler->handle(this);
                  return;
               }
               mSRVCount++;
               mDns.lookup<RR_SRV>("_sips._tcp." + mTarget, Protocol::Sip,  this);
               StackLog (<< "Doing SRV lookup of _sips._tcp." << mTarget);
            }
         }
         else
         {
            if (!mInterface.isSupportedProtocol(mTransport))
            {
               transition(Finished);
               if (mHandler) mHandler->handle(this);
               return;
            }

            switch(mTransport)
            {
               case TLS: //deprecated, mean TLS over TCP
                  mSRVCount++;
                  mDns.lookup<RR_SRV>("_sips._tcp." + mTarget, Protocol::Sip, this);
                  StackLog (<< "Doing SRV lookup of _sips._tcp." << mTarget);
                  break;
               case DTLS: //deprecated, mean TLS over TCP
                  mSRVCount++;
                  mDns.lookup<RR_SRV>("_sip._dtls." + mTarget, Protocol::Sip, this);
                  StackLog (<< "Doing SRV lookup of _sip._dtls." << mTarget);
                  break;
               case TCP:
                  mSRVCount++;
                  mDns.lookup<RR_SRV>("_sip._tcp." + mTarget, Protocol::Sip, this);
                  StackLog (<< "Doing SRV lookup of _sip._tcp." << mTarget);
                  break;
               case SCTP:
               case DCCP:
               case UDP:
               default: //fall through to UDP for unimplemented & unknown
                  mSRVCount++;
                  mDns.lookup<RR_SRV>("_sip._udp." + mTarget, Protocol::Sip, this);
                  StackLog (<< "Doing SRV lookup of _sip._udp." << mTarget);
            }
         }
      }
   }
   else 
   {
      if (isNumeric || uri.port() != 0)
      {
         if (mSips || mTransport == TLS)
         {
            mTransport = TLS;
         }
         else 
         {
            mTransport = UDP;
         }

         if (isNumeric) // IP address specified
         {
            mPort = getDefaultPort(mTransport, uri.port());
            Tuple tuple(mTarget, mPort, mTransport, mTarget);
            mResults.push_back(tuple);
            transition(Available);
            DebugLog (<< "Numeric result so return immediately: " << tuple);
            if (mHandler) mHandler->handle(this);
         }
         else // port specified so we know the transport
         {
            mPort = uri.port();
            if (mInterface.isSupported(mTransport, V6) || mInterface.isSupported(mTransport, V4))
            {
               lookupHost(mTarget);
            }
            else
            {
               assert(0);
               if (mHandler) mHandler->handle(this);
            }
         }
      }
      else // do NAPTR
      {
         mDns.lookup<RR_NAPTR>(mTarget, Protocol::Sip, this); // for current target
      }
   }
}

void DnsResult::lookupHost(const Data& target)
{
   if (mInterface.isSupported(mTransport, V6))
   {
#ifdef USE_IPV6
      DebugLog(<< "Doing host (AAAA) lookup: " << target);
      mPassHostFromAAAAtoA = target;
      mDns.lookup<RR_AAAA>(target, Protocol::Sip, this);
#else
      assert(0);
      mDns.lookup<RR_A>(target, Protocol::Sip, this);
#endif
   }
   else if (mInterface.isSupported(mTransport, V4))
   {
      mDns.lookup<RR_A>(target, Protocol::Sip, this);
   }
   else
   {
      assert(0);
   }
}

int
DnsResult::getDefaultPort(TransportType transport, int port)
{
   if (port == 0)
   {
      switch (transport)
      {
         case UDP:
            return Symbols::DefaultSipPort;
         case TCP:
            return mSips ? Symbols::DefaultSipsPort : Symbols::DefaultSipPort;
         case TLS:
         case DTLS:
            return Symbols::DefaultSipsPort;
         default:
            ErrLog( << "Should not get this - unknown transport" );
            return Symbols::DefaultSipPort; // !cj! todo - remove 
            assert(0);
      }
   }
   else
   {
      return port;
   }

	assert(0);
	return 0;
}

void
DnsResult::primeResults()
{
   StackLog(<< "Priming " << Inserter(mSRVResults));
   //assert(mType != Pending);
   //assert(mType != Finished);
   assert(mResults.empty());

   if (!mSRVResults.empty())
   {
      SRV next = retrieveSRV();
      StackLog (<< "Primed with SRV=" << next);
      transition(Pending);
      mPort = next.port;
      mTransport = next.transport;
      StackLog (<< "No A or AAAA record for " << next.target << " in additional records");
      if (mInterface.isSupported(mTransport, V6) || mInterface.isSupported(mTransport, V4))
      {
         assert(mCurrResultPath.size()<=2);
         Item top;
         if (!mCurrResultPath.empty())
         {
            top = mCurrResultPath.top();
            if (top.rrType == T_SRV)
            {
               vector<Data> records;
               records.push_back(top.value);
               mDns.blacklist(top.domain, top.rrType, Protocol::Sip, records);
               mCurrResultPath.pop();
            }
            else
            {
               assert(top.rrType==T_NAPTR);
            }
         }

         while (!mCurrSuccessPath.empty())
         {
            top = mCurrSuccessPath.top();
            if (top.rrType != T_NAPTR)
            {
               mCurrSuccessPath.pop();
            }
            else
            {
               break;
            }
         }
         top.domain = next.key;
         top.rrType = T_SRV;
         top.value = next.target + ":" + Data(next.port);
         mCurrResultPath.push(top);
         mCurrSuccessPath.push(top);
         lookupHost(next.target);
      }
      else
      {
         assert(0);
         if (mHandler) mHandler->handle(this);
      }
      // don't call primeResults since we need to wait for the response to
      // AAAA/A query first
   }
   else
   {
      bool changed = (mType == Pending);
      transition(Finished);
      if (!mCurrResultPath.empty())
      {
         assert(mCurrResultPath.size()<=2);
         while (!mCurrResultPath.empty())
         {
            Item top = mCurrResultPath.top();
            vector<Data> records;
            records.push_back(top.value);
            mDns.blacklist(top.domain, top.rrType, Protocol::Sip, records);
            mCurrResultPath.pop();
         }
      }
      if (changed && mHandler) mHandler->handle(this);
   }

   // Either we are finished or there are results primed
   //assert(mType == Finished ||        // !slg! handle() might end up destroying the DnsResult - so we can't safely do this assert
   //       mType == Pending || 
   //       (mType == Available && !mResults.empty())
   //   );
}

// implement the selection algorithm from rfc2782 (SRV records)
DnsResult::SRV 
DnsResult::retrieveSRV()
{
    // !ah! if mTransport is known -- should we ignore those that don't match?!
   assert(!mSRVResults.empty());

   const SRV& srv = *mSRVResults.begin();
   if (srv.cumulativeWeight == 0)
   {
      int priority = srv.priority;
   
     mCumulativeWeight=0;
      //!dcm! -- this should be fixed properly; TCP req. lines were being sent
      //out on UDP
      TransportType transport = mSRVResults.begin()->transport;      
      for (std::vector<SRV>::iterator i=mSRVResults.begin(); 
           i!=mSRVResults.end() 
              && i->priority == priority 
              && i->transport == transport; i++)
      {
         mCumulativeWeight += i->weight;
         i->cumulativeWeight = mCumulativeWeight;
      }
   }
   
   int selected = Random::getRandom() % (mCumulativeWeight+1);

   StackLog (<< "cumulative weight = " << mCumulativeWeight << " selected=" << selected);

   std::vector<SRV>::iterator i;
   for (i=mSRVResults.begin(); i!=mSRVResults.end(); i++)
   {
      if (i->cumulativeWeight >= selected)
      {
         break;
      }
   }
   
   if (i == mSRVResults.end())
   {
      InfoLog (<< "SRV Results problem selected=" << selected << " cum=" << mCumulativeWeight);
   }
   assert(i != mSRVResults.end());
   SRV next = *i;
   mCumulativeWeight -= next.cumulativeWeight;
   mSRVResults.erase(i);
   
   StackLog (<< "SRV: " << Inserter(mSRVResults));

   return next;
}

// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::skipDNSQuestion(const unsigned char *aptr,
                           const unsigned char *abuf,
                           int alen)
{
   char *name=0;
   int status=0;
   int len=0;
   
   // Parse the question name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      StackLog (<< "Failed parse of RR");
      return NULL;
   }
   aptr += len;

   // Make sure there's enough data after the name for the fixed part
   // of the question.
   if (aptr + QFIXEDSZ > abuf + alen)
   {
      free(name);
      StackLog (<< "Failed parse of RR");
      return NULL;
   }

   // Parse the question type and class. 
   //int type = DNS_QUESTION_TYPE(aptr);
   //int dnsclass = DNS_QUESTION_CLASS(aptr);
   aptr += QFIXEDSZ;
   
   free(name);
   return aptr;
}

DnsResult::NAPTR::NAPTR() : order(0), pref(0)
{
}

bool 
DnsResult::NAPTR::operator<(const DnsResult::NAPTR& rhs) const
{
   if (key.empty()) // default value
   {
      return false;
   }
   else if (rhs.key.empty()) // default value
   {
      return true;
   }
   else if (order < rhs.order)
   {
      return true;
   }
   else if (order == rhs.order)
   {
      if (pref < rhs.pref)
      {
         return true;
      }
      else if (pref == rhs.pref)
      {
         return replacement < rhs.replacement;
      }
   }
   return false;
}

DnsResult::SRV::SRV() : priority(0), weight(0), cumulativeWeight(0), port(0)
{
}

bool 
DnsResult::SRV::operator<(const DnsResult::SRV& rhs) const
{
   if (transport < rhs.transport)
   {
      return true;
   }
   else if (transport == rhs.transport)
   {
      if (priority < rhs.priority)
      {
         return true;
      }
      else if (priority == rhs.priority)
      {
         if (weight < rhs.weight)
         {
            return true;
         }
         else if (weight == rhs.weight)
         {
            if (target < rhs.target)
            {
               return true;
            }
         }
      }
   }
   return false;
}

void DnsResult::onDnsResult(const DNSResult<DnsHostRecord>& result)
{
   if (!mInterface.isSupported(mTransport, V4) && !mInterface.isSupported(mTransport, V6))
   {
      return;
   }
   StackLog (<< "Received dns result for: " << mTarget);
   StackLog (<< "DnsResult::onDnsResult() " << result.status);
   
   // This function assumes that the A query that caused this callback
   // is the _only_ outstanding DNS query that might result in a
   // callback into this function
   if ( mType == Destroyed )
   {
      destroy();
      return;
   }

   if (result.status == 0)
   {
      for (vector<DnsHostRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         in_addr addr;
         addr.s_addr = (*it).addr().s_addr;
         Tuple tuple(addr, mPort, mTransport, mTarget);
         StackLog (<< "Adding " << tuple << " to result set");
         mResults.push_back(tuple);
      }
   }
   else
   {
      StackLog (<< "Failed async A query: " << result.msg);
   }

   if (mSRVCount == 0)
   {
      bool changed = (mType == Pending);
      if (mResults.empty())
      {
#ifdef WIN32_SYNCRONOUS_RESOLUTION_ON_ARES_FAILURE
         // Try Windows Name Resolution (not asyncronous)
         WSAQUERYSET QuerySet = { 0 };
	     GUID guidServiceTypeUDP = SVCID_UDP(mPort);
	     GUID guidServiceTypeTCP = SVCID_TCP(mPort);
         HANDLE hQuery;
         QuerySet.dwSize = sizeof(WSAQUERYSET);
         QuerySet.lpServiceClassId = mTransport == UDP ? &guidServiceTypeUDP : &guidServiceTypeTCP;
         QuerySet.dwNameSpace = NS_ALL;
         QuerySet.lpszServiceInstanceName = (char *)mTarget.c_str();
         if(WSALookupServiceBegin(&QuerySet, LUP_RETURN_ADDR, &hQuery) == 0)
         {
             DWORD dwQuerySize = 256;   // Starting size
             int iRet = 0;
             bool fDone = false;
             LPWSAQUERYSET pQueryResult = (LPWSAQUERYSET) new char[dwQuerySize];
             while(iRet == 0 && pQueryResult)
             {
                iRet = WSALookupServiceNext(hQuery, 0, &dwQuerySize, pQueryResult);
                if(pQueryResult && iRet == -1 && GetLastError() == WSAEFAULT)
                {
                   delete [] pQueryResult;
                   pQueryResult = (LPWSAQUERYSET) new char[dwQuerySize]; // Re-allocate new size
                   iRet = WSALookupServiceNext(hQuery, 0, &dwQuerySize, pQueryResult);
                }
                if(pQueryResult && iRet == 0)
                {
                   for(DWORD i = 0; i < pQueryResult->dwNumberOfCsAddrs; i++)
                   {
     	              SOCKADDR_IN *pSockAddrIn = (SOCKADDR_IN *)pQueryResult->lpcsaBuffer[i].RemoteAddr.lpSockaddr;
                      Tuple tuple(pSockAddrIn->sin_addr, mPort, mTransport, mTarget);
                      StackLog (<< "Adding (WIN) " << tuple << " to result set");
                      mResults.push_back(tuple);
                      mType = Available;
                   }
                }
             }
             delete [] pQueryResult;
             WSALookupServiceEnd(hQuery);
         }
         if(mResults.empty())
         {
            mType = Finished; 
         }
#else
         mType = Finished; 
#endif
         clearCurrPath();
      }
      else 
      {
         if (mSRVResults.empty())
         {
            transition(Available);
         }
         else
         {
            mType = Available;
         }
         addToPath(mResults);
      }
      if (changed && mHandler) mHandler->handle(this);
   }
}

#ifdef USE_IPV6
void DnsResult::onDnsResult(const DNSResult<DnsAAAARecord>& result)
{
   StackLog (<< "Received AAAA result for: " << mTarget);
   if (!mInterface.isSupported(mTransport, V6))
   {
      return;
   }
   StackLog (<< "DnsResult::onDnsResult() " << result.status);
   assert(mInterface.isSupported(mTransport, V6));

   // This function assumes that the AAAA query that caused this callback
   // is the _only_ outstanding DNS query that might result in a
   // callback into this function
   if ( mType == Destroyed )
   {
      destroy();
      return;
   }

   if (result.status == 0)
   {
      for (vector<DnsAAAARecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         Tuple tuple((*it).v6Address(), mPort, mTransport, mTarget);
         StackLog (<< "Adding " << tuple << " to result set");
         mResults.push_back(tuple);
      }
   }
   else
   {
      StackLog (<< "Failed async AAAA query: " << result.msg);
   }
   // funnel through to host processing
   mDns.lookup<RR_A>(mPassHostFromAAAAtoA, Protocol::Sip, this);

}
#endif

void DnsResult::onDnsResult(const DNSResult<DnsSrvRecord>& result)
{
   StackLog (<< "Received SRV result for: " << mTarget);
   assert(mSRVCount>=0);
   mSRVCount--;
   StackLog (<< "DnsResult::onDnsResult() " << mSRVCount << " status=" << result.status);

   // There could be multiple SRV queries outstanding, but there will be no
   // other DNS queries outstanding that might cause a callback into this
   // object.
   if (mType == Destroyed && mSRVCount == 0)
   {
      destroy();
      return;
   }

   if (result.status == 0)
   {
      for (vector<DnsSrvRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         SRV srv;
         srv.key = (*it).name();
         srv.priority = (*it).priority();
         srv.weight = (*it).weight();
         srv.port = (*it).port();
         srv.target = (*it).target();
         if (srv.key.find("_sips._udp") != Data::npos)
         {
            srv.transport = DTLS;
         }
         else if (srv.key.find("_sips._tcp") != Data::npos)
         {
            srv.transport = TLS;
         }
         else if (srv.key.find("_udp") != Data::npos)
         {
            srv.transport = UDP;
         }
         else if (srv.key.find("_dtls") != Data::npos)
         {
            srv.transport = DTLS;
         }
         else if (srv.key.find("_tls") != Data::npos)
         {
            srv.transport = TLS;
         }
         else if (srv.key.find("_tcp") != Data::npos)
         {
            srv.transport = TCP;
         }
         else
         {
            StackLog (<< "Skipping SRV " << srv.key);
            continue;
         }
         mSRVResults.push_back(srv);
      }
   }
   else
   {
      StackLog (<< "SRV lookup failed: " << result.domain << " " << result.status);
   }

   // no outstanding queries 
   if (mSRVCount == 0) 
   {
      if (mSRVResults.empty())
      {
         if (mTransport == UNKNOWN_TRANSPORT)
         {
            if (mSips)
            {
               mTransport = TLS;
               mPort = Symbols::DefaultSipsPort;
            }
            else
            {
               mTransport = UDP;
               mPort = Symbols::DefaultSipPort;
            }
         }
         else
         {
            mPort = getDefaultPort(mTransport, 0);
         }
         
         StackLog (<< "No SRV records for " << mTarget << ". Trying A records");
         if (mInterface.isSupported(mTransport, V6) || mInterface.isSupported(mTransport, V4))
         {
            lookupHost(mTarget);
         }
         else
         {
            primeResults();
         }
      }
      else
      {
         std::sort(mSRVResults.begin(),mSRVResults.end()); // !jf! uggh
         primeResults();
      }
   }
}

void
DnsResult::onEnumResult(const DNSResult<DnsNaptrRecord>& result)
{
   mDoingEnum = false;
   
   if (result.status == 0)
   {
      static Data enumService1("e2u+sip");
      static Data enumService2("sip+e2u");

      DnsNaptrRecord best;
      best.order() = -1;

      for (vector<DnsNaptrRecord>::const_iterator i = result.records.begin(); i != result.records.end(); ++i)
      {
         InfoLog (<< "service=" << i->service()
                  << " order=" << i->order()
                  << " flags="  << i->flags() 
                  << " regexp substitution=" << i->regexp().replacement()
                  << " replacement=" << i->replacement());

         if ( (isEqualNoCase(i->service(), enumService1) ||
               isEqualNoCase(i->service(), enumService2) )  && // only E2U records
              //i->flags().find("u") != Data::npos && // must be terminal record
              i->replacement().empty() )
               
         {
            if (best.order() == -1)
            {
               best = *i;
            }
            else if (i->order() < best.order())
            {
               best = *i;
            }
            else if (i->order() == best.order() && 
                     i->preference() < best.preference())
            {
               best = *i;
            }
         }
      }
      
      if (best.order() != -1)
      {
         InfoLog (<< "Found an enum result: " << best.regexp().replacement());
         try
         {
            Uri rewrite(best.regexp().apply(Data::from(mInputUri)));
            InfoLog (<< "Rewrote uri " << mInputUri << " -> " << rewrite);
            mHandler->rewriteRequest(rewrite);
            lookupInternal(rewrite);
         }
         catch (ParseBuffer::Exception& /* e */)
         {
            lookupInternal(mInputUri);
         }
      }
      else
      {
         lookupInternal(mInputUri);
      }
   }
   else
   {
      lookupInternal(mInputUri);
   }
}

void
DnsResult::onNaptrResult(const DNSResult<DnsNaptrRecord>& result)
{
   bool bFail = false;
   if (result.status == 0)
   {
      for (vector<DnsNaptrRecord>::const_iterator it = result.records.begin(); it != result.records.end(); ++it)
      {
         NAPTR naptr;
         naptr.key = (*it).name();
         naptr.flags = (*it).flags();
         naptr.order = (*it).order();
         naptr.pref = (*it).preference();
         naptr.regex = (*it).regexp();
         naptr.replacement = (*it).replacement();
         naptr.service = (*it).service();
         
         StackLog (<< "Adding NAPTR record: " << naptr);
         
         if ( mSips && naptr.service.find("SIPS") == 0)
         {
            if (mInterface.isSupported(naptr.service) && naptr < mPreferredNAPTR)
            {
               mPreferredNAPTR = naptr;
               StackLog (<< "Picked preferred: " << mPreferredNAPTR);
            }
         }
         else if (mInterface.isSupported(naptr.service) && naptr < mPreferredNAPTR)
         {
            mPreferredNAPTR = naptr;
            StackLog (<< "Picked preferred: " << mPreferredNAPTR);
         }
      }

      // This means that dns / NAPTR is misconfigured for this client 
      if (mPreferredNAPTR.key.empty())
      {
         StackLog (<< "There are no NAPTR records supported by this client so do an SRV lookup instead");
         bFail = true;
      }
      else
      {
         transition(Pending);
         Item item;
         item.domain = mPreferredNAPTR.key;
         item.rrType = T_NAPTR;
         item.value = mPreferredNAPTR.replacement;
         clearCurrPath();
         mCurrResultPath.push(item);
         mCurrSuccessPath.push(item);
         mSRVCount++;
         InfoLog (<< "Doing SRV lookup of " << mPreferredNAPTR.replacement);
         mDns.lookup<RR_SRV>(mPreferredNAPTR.replacement, Protocol::Sip, this);
      }
   }
   else
   {
      if (result.status > 6)
      {
         DebugLog (<< "NAPTR lookup failed: " << result.domain << " " << result.msg);
      }
      else
      {
         StackLog (<< "NAPTR lookup failed: " << result.domain << " " << result.msg);
      }
      bFail = true;
   }

   if (bFail)
   {

      if (mSips)
      {
         if (!mInterface.isSupportedProtocol(TLS))
         {
            transition(Finished);
            if (mHandler) mHandler->handle(this);
            return;
         }

         mSRVCount++;
         mDns.lookup<RR_SRV>("_sips._tcp." + mTarget, Protocol::Sip, this);
         StackLog (<< "Doing SRV lookup of _sips._tcp." << mTarget);
      }
      else
      {
         if (mInterface.isSupportedProtocol(TLS))
         {
            mDns.lookup<RR_SRV>("_sips._tcp." + mTarget, Protocol::Sip, this);
            ++mSRVCount;
            StackLog (<< "Doing SRV lookup of _sips._tcp." << mTarget);
         }
         if (mInterface.isSupportedProtocol(DTLS))
         {
            mDns.lookup<RR_SRV>("_sips._udp." + mTarget, Protocol::Sip, this);
            ++mSRVCount;
            StackLog (<< "Doing SRV lookup of _sips._udp." << mTarget);
         }
         if (mInterface.isSupportedProtocol(TCP))
         {
            mDns.lookup<RR_SRV>("_sip._tcp." + mTarget, Protocol::Sip, this);
            ++mSRVCount;
            StackLog (<< "Doing SRV lookup of _sip._tcp." << mTarget);
         }
         if (mInterface.isSupportedProtocol(UDP))
         {
            mDns.lookup<RR_SRV>("_sip._udp." + mTarget, Protocol::Sip, this);
            ++mSRVCount;
            StackLog (<< "Doing SRV lookup of _sip._udp." << mTarget);
         }
      }
   }
}

void 
DnsResult::onDnsResult(const DNSResult<DnsNaptrRecord>& result)
{
   StackLog (<< "Received NAPTR result for: " << mInputUri << " target=" << mTarget);
   StackLog (<< "DnsResult::onDnsResult() " << result.status);

   // This function assumes that the NAPTR query that caused this
   // callback is the ONLY outstanding query that might cause
   // a callback into this object
   if (mType == Destroyed)
   {
      destroy();
      return;
   }

   if (mDoingEnum)
   {
      onEnumResult(result);
   }
   else
   {
      onNaptrResult(result);
   }
  
}

void DnsResult::onDnsResult(const DNSResult<DnsCnameRecord>& result)
{
}

void DnsResult::clearCurrPath()
{
   while (!mCurrResultPath.empty())
   {
      mCurrResultPath.pop();
   }

   while (!mCurrSuccessPath.empty())
   {
      mCurrSuccessPath.pop();
   }
}

void DnsResult::blacklistLastReturnedResult()
{
   assert(!mCurrResultPath.empty());
   Item top = mCurrResultPath.top();
   assert(top.rrType==T_A || top.rrType==T_AAAA);
   assert(top.domain==mLastReturnedResult.getTargetDomain());
   assert(top.value==Tuple::inet_ntop(mLastReturnedResult));
   vector<Data> records;
   records.push_back(top.value);
   DebugLog( << "Blacklisting " << top.domain << "(" << top.rrType << "): " << top.value);
   mDns.blacklist(top.domain, top.rrType, Protocol::Sip, records);
   DebugLog( << "Remove vip " << top.domain << "(" << top.rrType << ")");
   mVip.removeVip(top.domain, top.rrType);
   mCurrResultPath.pop();
}

void DnsResult::addToPath(const std::deque<Tuple>& results)
{
   assert(mCurrResultPath.size()<=2);
   for (std::deque<Tuple>::const_reverse_iterator it = results.rbegin(); it != results.rend(); ++it)
   {
      Item item;
      item.domain = (*it).getTargetDomain();
      item.rrType = (*it).isV4()? T_A : T_AAAA;
      item.value = Tuple::inet_ntop((*it));
      mCurrResultPath.push(item);
   }
}

EncodeStream& 
resip::operator<<(EncodeStream& strm, const resip::DnsResult& result)
{
   strm << result.mTarget << " --> " << Inserter(result.mResults);
   return strm;
}


EncodeStream& 
resip::operator<<(EncodeStream& strm, const resip::DnsResult::NAPTR& naptr)
{
   strm << "key=" << naptr.key
        << " order=" << naptr.order
        << " pref=" << naptr.pref
        << " flags=" << naptr.flags
        << " service=" << naptr.service
        << " regex=" << naptr.regex.regexp() << " -> " << naptr.regex.replacement()
        << " replacement=" << naptr.replacement;
   return strm;
}

EncodeStream& 
resip::operator<<(EncodeStream& strm, const resip::DnsResult::SRV& srv)
{
   strm << "key=" << srv.key
        << " t=" << Tuple::toData(srv.transport) 
        << " p=" << srv.priority
        << " w=" << srv.weight
        << " c=" << srv.cumulativeWeight
        << " port=" << srv.port
        << " target=" << srv.target;
   return strm;
}


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
