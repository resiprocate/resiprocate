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

#include "rutil/Timer.hxx"

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
     mHaveChosenTransport(false),
     mType(Pending),
     mCumulativeWeight(0),
     mHaveReturnedResults(false)
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
   if ((t == Finished || t == Destroyed) &&
       (mType == Pending || mType == Available))
   {
      mInterface.mActiveQueryCount--;
      assert(mInterface.mActiveQueryCount >= 0);
   }
   
   if((t == Pending || t== Available) && 
         (mType== Finished || mType == Destroyed) )
   {
      assert(0);
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
      transition(Finished);
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
   assert(available()==Available);
   assert(mCurrentPath.size()<=3);
   
   mLastResult=mResults.front();
   mResults.pop_front();
   
   if(!mCurrentPath.empty() && 
      (mCurrentPath.back().rrType==T_A || mCurrentPath.back().rrType==T_AAAA))
   {
      mCurrentPath.pop_back();
   }
   
   Item AorAAAA;
   AorAAAA.domain = mLastResult.getTargetDomain();
   AorAAAA.rrType = mLastResult.isV4() ? T_A : T_AAAA;
   AorAAAA.value = Tuple::inet_ntop(mLastResult);
   mCurrentPath.push_back(AorAAAA);
   
   StackLog (<< "Returning next dns entry: " << mLastResult);
   mLastReturnedPath=mCurrentPath;
   mHaveReturnedResults=true;
   return mLastResult;
}

void DnsResult::success()
{
   std::vector<Item>::iterator i;
   for (i=mLastReturnedPath.begin(); i!=mLastReturnedPath.end(); ++i)
   {
      DebugLog( << "Whitelisting " << i->domain << "(" << i->rrType << "): " << i->value);
      mVip.vip(i->domain, i->rrType, i->value);
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
      mHaveChosenTransport=true;
      
      if (isNumeric) // IP address specified
      {
         mPort = getDefaultPort(mTransport, uri.port());
         Tuple tuple(mTarget, mPort, mTransport, mTarget);

         if(!DnsResult::blacklisted(tuple))
         {
            DebugLog (<< "Found immediate result: " << tuple);
            mResults.push_back(tuple);
            transition(Available);
            if (mHandler) mHandler->handle(this);
         }
         else
         {
            transition(Available);
            if (mHandler) mHandler->handle(this);
         }

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
               mHaveChosenTransport=true;
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
               mHaveChosenTransport=true;
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
         bool foundTuple=false;
         Tuple tuple;
         
         if(isNumeric)
         {
            if(mSips)
            {
               mTransport=TLS;
               mPort = getDefaultPort(mTransport,uri.port());
               tuple=Tuple(mTarget,mPort,mTransport,mTarget);
               foundTuple=!DnsResult::blacklisted(tuple);
            }
            else
            {
               if(!foundTuple)
               {
                  mTransport=UDP;
                  mPort = getDefaultPort(mTransport,uri.port());
                  tuple=Tuple(mTarget,mPort,mTransport,mTarget);
                  foundTuple=!DnsResult::blacklisted(tuple);
               }
               
               if(!foundTuple)
               {
                  mTransport=TCP;
                  mPort = getDefaultPort(mTransport,uri.port());
                  tuple=Tuple(mTarget,mPort,mTransport,mTarget);
                  foundTuple=!DnsResult::blacklisted(tuple);
               }
               
               if(!foundTuple)
               {
                  mTransport=TLS;
                  mPort = getDefaultPort(mTransport,uri.port());
                  tuple=Tuple(mTarget,mPort,mTransport,mTarget);
                  foundTuple=!DnsResult::blacklisted(tuple);
               }
            }
            
            if(foundTuple)
            {
               mHaveChosenTransport=true;
               mResults.push_back(tuple);
               transition(Available);
               DebugLog (<< "Numeric result so return immediately: " << tuple);
            }
            else
            {
               // !bwc! Numeric result is blacklisted. Oh well.
               assert(mResults.empty());
               transition(Available);
               DebugLog(<< "Numeric result, but this result is currently blacklisted: " << tuple);
            }
            
            if (mHandler) mHandler->handle(this);

         }
        else // port specified so we know the transport
         {
            if(mSips)
            {
               mTransport=TLS;
               mPort = uri.port();
            }
            else
            {
               mTransport=UDP;
               mPort = uri.port();
            }
            
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
         Item top;
         while (!mCurrentPath.empty())
         {
            top = mCurrentPath.back();
            if (top.rrType != T_NAPTR)
            {
               mCurrentPath.pop_back();
            }
            else
            {
               break;
            }
         }
         top.domain = next.key;
         top.rrType = T_SRV;
         top.value = next.target + ":" + Data(next.port);
         mCurrentPath.push_back(top);
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
   assert(mSRVCount==0);

   const SRV& srv = *mSRVResults.begin();
   int priority = srv.priority;
   TransportType transport=UNKNOWN_TRANSPORT;
   
   if(!mHaveChosenTransport)
   {
      // !bwc! We have not chosen a transport yet; this happens when we fail
      // to find a NAPTR record, and the transport is not specified in the uri.
      // In this contingency, we manufacture best-guess SRV queries for each
      // transport we support, and try one transport at a time. This
      // means we might try more than one transport for the uri in question.
      transport = srv.transport;
   }
   else
   {
      // !bwc! We chose our transport before we started looking up SRVs.
      // All SRVs must match. 
      
      transport=mTransport;
      assert(mSRVResults.begin()->transport==transport);
   }
   
   if (mCumulativeWeight == 0)
   {
      for (std::vector<SRV>::iterator i=mSRVResults.begin(); 
           i!=mSRVResults.end() 
              && i->priority == priority 
              && i->transport == transport; i++)
      {
         assert(i->weight>=0);
         mCumulativeWeight += i->weight;
      }
   }
   
   int selected =0;
   if(mCumulativeWeight!=0)
   {
      selected = Random::getRandom() % (mCumulativeWeight);
   }
   else
   {
      // !bwc! All of the remaining SRVs (at this priority/type) have a weight
      // of 0. The best we can do here is pick arbitrarily. In this case, we 
      // will end up picking the first.
      // (selected will be less than the weight of the first SRV, causing the
      // loop below to break on the first iteration)
      selected=-1;
   }
   
   StackLog (<< "cumulative weight = " << mCumulativeWeight << " selected=" << selected);

   std::vector<SRV>::iterator i;
   int cumulativeWeight=0;
   for (i=mSRVResults.begin(); i!=mSRVResults.end(); ++i)
   {
      cumulativeWeight+=i->weight;
      if (cumulativeWeight > selected)
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
   mCumulativeWeight -= next.weight;
   mSRVResults.erase(i);
   
   if(!mSRVResults.empty())
   {
      int nextPriority=mSRVResults.begin()->priority;
      TransportType nextTransport=mSRVResults.begin()->transport;
      
      // !bwc! If we have finished traversing a priority value/transport type,
      // we reset the cumulative weight to 0, to prompt its recalculation.
      if(priority!=nextPriority || transport!=nextTransport)
      {
         mCumulativeWeight=0;
      }
   }
   
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

DnsResult::SRV::SRV() : priority(0), weight(0), port(0)
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
         
         if(!DnsResult::blacklisted(tuple))
         {
            StackLog (<< "Adding " << tuple << " to result set");
            mResults.push_back(tuple);
         }
      
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
                      
                      if(!DnsResult::blacklisted(tuple))
                      {
                        StackLog (<< "Adding (WIN) " << tuple << " to result set");
                        mResults.push_back(tuple);
                        transition(Available);
                      }
                   
                   }
                }
             }
             delete [] pQueryResult;
             WSALookupServiceEnd(hQuery);
         }
         
         if(mResults.empty())
         {
            if(mSRVResults.empty())
            {
               transition(Finished);
               clearCurrPath();
            }
            else
            {
               transition(Available);
            }
         }
#else
         // !bwc! If this A query failed, don't give up if there are more SRVs!
         if(mSRVResults.empty())
         {
            transition(Finished);
            clearCurrPath();
         }
         else
         {
            transition(Available);
         }
#endif
      }
      else 
      {
         transition(Available);
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
         
         if(!DnsResult::blacklisted(tuple))
         {
            StackLog (<< "Adding " << tuple << " to result set");
            mResults.push_back(tuple);
         }
      
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
         
         if(!mHaveChosenTransport || srv.transport==mTransport)
         {
            // !bwc! If we have not committed to a given transport, or we have 
            // committed to a given transport which this SRV matches, we will
            // add this SRV. We do not add SRVs that do not match a transport
            // we have committed to.
            mSRVResults.push_back(srv);
         }
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
               mHaveChosenTransport=true;
               mPort = Symbols::DefaultSipsPort;
            }
            else
            {
               if (mInterface.isSupported(UDP, V4))
               {
                  mTransport = UDP;
                  mHaveChosenTransport=true;
               }
               else if (mInterface.isSupported(TCP, V4))
               {
                  mTransport = TCP;
                  mHaveChosenTransport=true;
               }
               /* Yes, there is the possibility that at this point mTransport
                  is still UNKNOWN_TRANSPORT, but this is likely to fail just as
                  well as defaulting to UDP when there isn't an interface that
                  supports UDP.
                  It doesn't support failover to TCP when there is a UDP failure,
                  but neither does the original code.
                  This fixes the case where there is no UDP transport, but
                  there was no explicit ;transport=tcp on the uri.
                  (mjf)
                */
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
         
         if ( !mSips || naptr.service.find("SIPS") == 0)
         {
            if (mInterface.isSupported(naptr.service) && naptr < mPreferredNAPTR)
            {
               mPreferredNAPTR = naptr;
               StackLog (<< "Picked preferred: " << mPreferredNAPTR);
            }
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
         mCurrentPath.push_back(item);
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
   while (!mCurrentPath.empty())
   {
      mCurrentPath.pop_back();
   }
}

void DnsResult::blacklistLastReturnedResult(time_t expiry)
{
   assert(!mLastReturnedPath.empty());
   assert(mLastReturnedPath.size()<=3);
   Item top = mLastReturnedPath.back();

   DnsResult::blacklist(mLastResult,expiry);

   DebugLog( << "Remove vip " << top.domain << "(" << top.rrType << ")");
   mVip.removeVip(top.domain, top.rrType);
}

DnsResult::Blacklist DnsResult::theBlacklist;
resip::Mutex DnsResult::theBlacklistMutex;

DnsResult::BlacklistEntry::BlacklistEntry()
{}

DnsResult::BlacklistEntry::BlacklistEntry(const Tuple& tuple, time_t expiry)
{
   mTuple=tuple;
   mExpiry=expiry;
}

DnsResult::BlacklistEntry::BlacklistEntry(const DnsResult::BlacklistEntry& orig)
{
   mTuple=orig.mTuple;
   mExpiry=orig.mExpiry;
}

DnsResult::BlacklistEntry::~BlacklistEntry()
{}

bool 
DnsResult::BlacklistEntry::operator<(const DnsResult::BlacklistEntry& rhs) const
{
   if(mTuple < rhs.mTuple)
   {
      return true;
   }
   else if(rhs.mTuple < mTuple)
   {
      return false;
   }
   
   return mTuple.getTargetDomain() < rhs.mTuple.getTargetDomain();
}

bool 
DnsResult::BlacklistEntry::operator>(const DnsResult::BlacklistEntry& rhs) const
{
   if(rhs.mTuple < mTuple)
   {
      return true;
   }
   else if(mTuple < rhs.mTuple)
   {
      return false;
   }
   
   return mTuple.getTargetDomain() > rhs.mTuple.getTargetDomain();
}

bool 
DnsResult::BlacklistEntry::operator==(const DnsResult::BlacklistEntry& rhs) const
{
   return (mTuple==rhs.mTuple && mTuple.getTargetDomain()==rhs.mTuple.getTargetDomain());
}

bool DnsResult::blacklisted(const Tuple& tuple)
{
   BlacklistEntry entry(tuple,0);
   resip::Lock g(DnsResult::theBlacklistMutex);
   Blacklist::iterator i=DnsResult::theBlacklist.find(entry);
   
   if(i!=DnsResult::theBlacklist.end())
   {
      time_t now=Timer::getTimeMs();
      if(i->mExpiry > now)
      {
         return true;
      }
      else
      {
         DnsResult::theBlacklist.erase(i);
      }
   }
   
   return false;
}

void DnsResult::blacklist(const Tuple& tuple,time_t expiry)
{
   BlacklistEntry entry(tuple,expiry);
   resip::Lock g(DnsResult::theBlacklistMutex);
   DnsResult::theBlacklist.insert(entry);
}

std::ostream& 
resip::operator<<(std::ostream& strm, const resip::DnsResult& result)
{
   strm << result.mTarget << " --> " << Inserter(result.mResults);
   return strm;
}


std::ostream& 
resip::operator<<(std::ostream& strm, const resip::DnsResult::NAPTR& naptr)
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

std::ostream& 
resip::operator<<(std::ostream& strm, const resip::DnsResult::SRV& srv)
{
   strm << "key=" << srv.key
        << " t=" << Tuple::toData(srv.transport) 
        << " p=" << srv.priority
        << " w=" << srv.weight
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
