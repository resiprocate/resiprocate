#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/compat.hxx"

#include "resiprocate/DnsHandler.hxx"
#include "resiprocate/DnsInterface.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/Uri.hxx"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#endif


// This is here so we can use the same macros to parse a dns result using the
// standard synchronous unix dns_query call
#if !defined(USE_ARES)
// borrowed from ares - will also work in win32

#define ARES_SUCCESS 0

#define DNS__16BIT(p)			(((p)[0] << 8) | (p)[1])
/* Macros for parsing a DNS header */
#define DNS_HEADER_QID(h)		DNS__16BIT(h)
#define DNS_HEADER_QR(h)		(((h)[2] >> 7) & 0x1)
#define DNS_HEADER_OPCODE(h)		(((h)[2] >> 3) & 0xf)
#define DNS_HEADER_AA(h)		(((h)[2] >> 2) & 0x1)
#define DNS_HEADER_TC(h)		(((h)[2] >> 1) & 0x1)
#define DNS_HEADER_RD(h)		((h)[2] & 0x1)
#define DNS_HEADER_RA(h)		(((h)[3] >> 7) & 0x1)
#define DNS_HEADER_Z(h)			(((h)[3] >> 4) & 0x7)
#define DNS_HEADER_RCODE(h)		((h)[3] & 0xf)
#define DNS_HEADER_QDCOUNT(h)		DNS__16BIT((h) + 4)
#define DNS_HEADER_ANCOUNT(h)		DNS__16BIT((h) + 6)
#define DNS_HEADER_NSCOUNT(h)		DNS__16BIT((h) + 8)
#define DNS_HEADER_ARCOUNT(h)		DNS__16BIT((h) + 10)

/* Macros for parsing the fixed part of a DNS resource record */
#define DNS_RR_TYPE(r)			DNS__16BIT(r)
#define DNS_RR_CLASS(r)			DNS__16BIT((r) + 2)
#define DNS_RR_TTL(r)			DNS__32BIT((r) + 4)
#define DNS_RR_LEN(r)			DNS__16BIT((r) + 8)

#endif


using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

DnsResult::DnsResult(DnsInterface& interfaceObj, DnsHandler* handler) 
   : mInterface(interfaceObj),
     mHandler(handler),
     mSRVCount(0),
     mSips(false),
     mTransport(UNKNOWN_TRANSPORT),
     mPort(-1),
     mType(Pending)
{
}

DnsResult::~DnsResult()
{
   assert(mType != Destroyed);
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
   assert(available() == Available);
   Tuple next = mResults.front();
   mResults.pop_front();
   DebugLog (<< "Returning next dns entry: " << next);
   return next;
}

void
DnsResult::destroy()
{
   assert(this);
   DebugLog (<< "DnsResult::destroy() " << *this);
   
   if (mType == Pending)
   {
      mType = Destroyed;
   }
   else
   {
      delete this;
   }
}

void
DnsResult::lookup(const Uri& uri)
{
   DebugLog (<< "DnsResult::lookup " << uri);
   
   assert(uri.scheme() == Symbols::Sips || uri.scheme() == Symbols::Sip);  
   mTarget = uri.exists(p_maddr) ? uri.param(p_maddr) : uri.host();
   mSips = (uri.scheme() == Symbols::Sips);
   bool isNumeric = DnsUtil::isIpAddress(mTarget);

   if (uri.exists(p_transport))
   {
      mTransport = Tuple::toTransport(uri.param(p_transport));

      if (isNumeric) // IP address specified
      {
         Tuple tuple;
         tuple.transportType = mTransport;
         tuple.transport = 0;
		 DnsUtil::inet_pton( mTarget, tuple.ipv4); // !jf! - check return 
         mPort = getDefaultPort(mTransport, uri.port());
         tuple.port = mPort;

         DebugLog (<< "Found immediate result: " << tuple);
         mResults.push_back(tuple);
         mType = Available;
         //mHandler->handle(this); // !jf! should I call this? 
      }
      else if (uri.port() != 0)
      {
         mPort = uri.port();
         lookupARecords(mTarget); // for current target and port         
      }
   }
   else 
   {
      if (isNumeric || uri.port() != 0)
      {
         if (mSips)
         {
            mTransport = TLS;
         }
         else 
         {
            mTransport = UDP;
         }

         if (isNumeric) // IP address specified
         {
            Tuple tuple;
            tuple.transportType = mTransport;
            tuple.transport = 0;
			DnsUtil::inet_pton( mTarget, tuple.ipv4 ); // !jf! check return 

            mPort = getDefaultPort(mTransport, uri.port());
            tuple.port = mPort;

            mResults.push_back(tuple);
            mType = Available;
            //mHandler->handle(this); // !jf! should I call this? 
         }
         else // port specified so we know the transport
         {
            mPort = uri.port();
            lookupARecords(mTarget); // for current target and port
         }
      }
      else // do NAPTR
      {
         lookupNAPTR(); // for current target
      }
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
            return 5060;
         case TCP:
            return mSips ? 5061 : 5060;
         default:
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
DnsResult::lookupAAAARecords(const Data& target)
{
   DebugLog(<< "Doing host (AAAA) lookup: " << target);
   mPassHostFromAAAAtoA = target; // hackage
#if defined(USE_ARES)
   ares_query(mInterface.mChannel, target.c_str(), C_IN, T_AAAA, DnsResult::aresAAAACallback, this); 
#else
   /*TODO - deal with looking for AAAAs directly */
   /* For now, shortcut straight to looking for A records */
   lookupARecords(target);
#endif
}

void
DnsResult::lookupARecords(const Data& target)
{
   DebugLog (<< "Doing Host (A) lookup: " << target);

#if defined(USE_ARES)
   ares_gethostbyname(mInterface.mChannel, target.c_str(), AF_INET, DnsResult::aresHostCallback, this);
#else   
   struct hostent* result=0;
   int ret=0;
   int herrno=0;

#if defined(__linux__)
   struct hostent hostbuf; 
   char buffer[8192];
   ret = gethostbyname_r( target.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
   assert (ret != ERANGE);
#elif defined(WIN32) 
   result = gethostbyname( target.c_str() );
   herrno = WSAGetLastError();
#elif defined( __MACH__ ) || defined (__FreeBSD__)
   result = gethostbyname( target.c_str() );
   herrno = h_errno;
#elif defined(__QNX__) || defined(__sun)
   struct hostent hostbuf; 
   char buffer[8192];
   result = gethostbyname_r( target.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno );
#else
#   error "need to define some version of gethostbyname for your arch"
#endif

   if ( (ret!=0) || (result==0) )
   {
      switch (herrno)
      {
         case HOST_NOT_FOUND:
            InfoLog ( << "host not found: " << target);
            break;
         case NO_DATA:
            InfoLog ( << "no data found for: " << target);
            break;
         case NO_RECOVERY:
            InfoLog ( << "no recovery lookup up: " << target);
            break;
         case TRY_AGAIN:
            InfoLog ( << "try again: " << target);
            break;
		 default:
            ErrLog( << "DNS Resolver got error" << herrno << " looking up " << target );
            assert(0);
            break;
      }
   }
   else
   {
      processHost(ARES_SUCCESS, result);
   }
#endif
}

void
DnsResult::lookupNAPTR()
{
   DebugLog (<< "Doing NAPTR lookup: " << mTarget);

#if defined(USE_ARES)
   ares_query(mInterface.mChannel, mTarget.c_str(), C_IN, T_NAPTR, DnsResult::aresNAPTRCallback, this); 
#else
   unsigned char result[4096];
   int len = res_query(mTarget.c_str(), C_IN, T_NAPTR, result, sizeof(result));
   processNAPTR(ARES_SUCCESS, result, len);
#endif
}

void
DnsResult::lookupSRV(const Data& target)
{
   DebugLog (<< "Doing SRV lookup: " << target);
   
#if defined(USE_ARES)
   ares_query(mInterface.mChannel, target.c_str(), C_IN, T_SRV, DnsResult::aresSRVCallback, this); 
#else
   unsigned char result[4096];
   int len = res_query(mTarget.c_str(), C_IN, T_SRV, result, sizeof(result));
   processSRV(ARES_SUCCESS, result, len);
#endif
}

#if defined (USE_ARES)
void
DnsResult::aresHostCallback(void *arg, int status, struct hostent* result)
{
   DnsResult *thisp = reinterpret_cast<DnsResult*>(arg);
   DebugLog (<< "Received A result for: " << thisp->mHandler << " for " << thisp->mTarget);
   thisp->processHost(status, result);
}

void
DnsResult::aresNAPTRCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   DnsResult *thisp = reinterpret_cast<DnsResult*>(arg);
   DebugLog (<< "Received NAPTR result for: " << thisp->mHandler << " for " << thisp->mTarget);
   thisp->processNAPTR(status, abuf, alen);
}


void
DnsResult::aresSRVCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   DnsResult *thisp = reinterpret_cast<DnsResult*>(arg);
   DebugLog (<< "Received SRV result for: " << thisp->mHandler << " for " << thisp->mTarget);
   thisp->processSRV(status, abuf, alen);
}

void
DnsResult::aresAAAACallback(void *arg, int status, unsigned char *abuf, int alen)
{
   DnsResult *thisp = reinterpret_cast<DnsResult*>(arg);
   DebugLog (<< "Received AAAA result for: " << thisp->mHandler << " for " << thisp->mTarget);
   thisp->processAAAA(status, abuf, alen);
}
#endif


void
DnsResult::processNAPTR(int status, unsigned char* abuf, int alen)
{
   DebugLog (<< "DnsResult::processNAPTR() " << status);

   // This function assumes that the NAPTR query that caused this
   // callback is the ONLY outstanding query that might cause
   // a callback into this object
   if (mType == Destroyed)
   {
      destroy();
      return;
   }

   if (status == ARES_SUCCESS)
   {
      const unsigned char *aptr = abuf + HFIXEDSZ;
      int qdcount = DNS_HEADER_QDCOUNT(abuf);    /* question count */
      for (int i = 0; i < qdcount && aptr; i++)
      {
         aptr = skipDNSQuestion(aptr, abuf, alen);
      }
      
      int ancount = DNS_HEADER_ANCOUNT(abuf);    /* answer record count */
      for (int i = 0; i < ancount && aptr; i++)
      {
         NAPTR naptr;
         aptr = parseNAPTR(aptr, abuf, alen, naptr);
         
         if (aptr)
         {
            DebugLog (<< "Adding NAPTR record: " << naptr);
            if (mSips && naptr.service.find("SIPS") == 0)
            {
               if (mInterface.isSupported(naptr.service) && naptr < mPreferredNAPTR)
               {
                  mPreferredNAPTR = naptr;
                  DebugLog (<< "Picked preferred: " << mPreferredNAPTR);
               }
            }
            else if (mInterface.isSupported(naptr.service) && naptr < mPreferredNAPTR)
            {
               mPreferredNAPTR = naptr;
               DebugLog (<< "Picked preferred: " << mPreferredNAPTR);
            }
         }
      }

      // This means that dns / NAPTR is misconfigured for this client 
      if (mPreferredNAPTR.key.empty())
      {
         DebugLog (<< "No NAPTR records that are supported by this client");
         mType = Finished;
         mHandler->handle(this);
         return;
      }

      int nscount = DNS_HEADER_NSCOUNT(abuf);    /* name server record count */
      DebugLog (<< "Found " << nscount << " nameserver records");
      for (int i = 0; i < nscount && aptr; i++)
      {
         // this will ignore NS records
         aptr = parseAdditional(aptr, abuf, alen);
      }

      int arcount = DNS_HEADER_ARCOUNT(abuf);    /* additional record count */
      DebugLog (<< "Found " << arcount << " additional records");
      for (int i = 0; i < arcount && aptr; i++)
      {
         // this will store any related SRV and A records
         // don't store SRV records other than the one in the selected NAPTR
         aptr = parseAdditional(aptr, abuf, alen);
      }

      if (ancount == 0) // didn't find any NAPTR records
      {
         DebugLog (<< "There are no NAPTR records so do an SRV lookup instead");
         goto NAPTRFail; // same as if no NAPTR records
      }
      else if (mSRVResults.empty())
      {
         // didn't find any SRV records in Additional, so do another query

         assert(mSRVCount == 0);
         assert(!mPreferredNAPTR.replacement.empty());

         DebugLog (<< "No SRV record for " << mPreferredNAPTR.replacement << " in additional section");
         mType = Pending;
         mSRVCount++;
         lookupSRV(mPreferredNAPTR.replacement);
      }
      else
      {
         // This will fill in mResults based on the DNS result
         primeResults();
      }
   }
   else
   {
      {
#ifdef USE_ARES
         char* errmem=0;
         DebugLog (<< "NAPTR lookup failed: " << ares_strerror(status, &errmem));
         ares_free_errmem(errmem);
#endif
      }
      
      // This will result in no NAPTR results. In this case, send out SRV
      // queries for each supported protocol
     NAPTRFail:
      // For now, don't add _sips._tcp in this case. 
      lookupSRV("_sip._tcp." + mTarget);
      mSRVCount++;
      lookupSRV("_sip._udp." + mTarget);
      mSRVCount++;
      DebugLog (<< "Doing SRV query " << mSRVCount << " for " << mTarget);
   }
}

void
DnsResult::processSRV(int status, unsigned char* abuf, int alen)
{
   assert(mSRVCount>=0);
   mSRVCount--;
   DebugLog (<< "DnsResult::processSRV() " << mSRVCount << " status=" << status);

   // There could be multiple SRV queries outstanding, but there will be no
   // other DNS queries outstanding that might cause a callback into this
   // object.
   if (mType == Destroyed && mSRVCount == 0)
   {
      destroy();
      return;
   }

   if (status == ARES_SUCCESS)
   {
      const unsigned char *aptr = abuf + HFIXEDSZ;
      int qdcount = DNS_HEADER_QDCOUNT(abuf);    /* question count */
      for (int i = 0; i < qdcount && aptr; i++)
      {
         aptr = skipDNSQuestion(aptr, abuf, alen);
      }
      
      int ancount = DNS_HEADER_ANCOUNT(abuf);    /* answer record count */
      for (int i = 0; i < ancount && aptr; i++)
      {
         SRV srv;
         aptr = parseSRV(aptr, abuf, alen, srv);
         
         if (aptr)
         {
            if (srv.key.find("_sip._udp") == 0)
            {
               srv.transport = UDP;
            }
            else if (srv.key.find("_sip._tcp") == 0)
            {
               srv.transport = TCP;
            }
            else if (srv.key.find("_sips._tcp") == 0)
            {
               srv.transport = TLS;
            }
            else
            {
               DebugLog (<< "Skipping SRV " << srv.key);
               continue;
            }

            DebugLog (<< "Adding SRV record (no NAPTR): " << srv);
            mSRVResults.insert(srv);
         }
      }

      int nscount = DNS_HEADER_NSCOUNT(abuf);    /* name server record count */
      DebugLog (<< "Found " << nscount << " nameserver records");
      for (int i = 0; i < nscount && aptr; i++)
      {
         // this will ignore NS records
         aptr = parseAdditional(aptr, abuf, alen);
      }
      
      int arcount = DNS_HEADER_ARCOUNT(abuf);    /* additional record count */
      DebugLog (<< "Found " << arcount << " additional records");
      for (int i = 0; i < arcount && aptr; i++)
      {
         // this will store any related A records
         aptr = parseAdditional(aptr, abuf, alen);
      }
   }
   else
   {
#ifdef USE_ARES
      char* errmem=0;
      DebugLog (<< "SRV lookup failed: " << ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
#endif
   }

   // no outstanding queries 
   if (mSRVCount == 0) 
   {
      if (mSRVResults.empty())
      {
         if (mSips)
         {
            mTransport = TCP;
            mPort = 5060;
         }
         else
         {
            mTransport = UDP;
            mPort = 5060;
         }
         
         DebugLog (<< "No SRV records for " << mTarget << ". Trying A records");
         lookupARecords(mTarget);
      }
      else
      {
#ifndef WIN32
         DebugLog(<< "Got all SRV responses. Priming " << Inserter(mSRVResults));
#endif
         primeResults();
      }
   }
}

void
DnsResult::processAAAA(int status, unsigned char* abuf, int alen)
{
   DebugLog (<< "DnsResult::processAAAA() " << status);
   // This function assumes that the AAAA query that caused this callback
   // is the _only_ outstanding DNS query that might result in a
   // callback into this function
   if ( mType == Destroyed )
   {
      destroy();
      return;
   }
   if (status == ARES_SUCCESS)
   {
     const unsigned char *aptr = abuf + HFIXEDSZ;

     int qdcount = DNS_HEADER_QDCOUNT(abuf); /*question count*/
     for (int i=0; i < qdcount && aptr; i++)
     {
       aptr = skipDNSQuestion(aptr, abuf, alen);
     }

     int ancount = DNS_HEADER_ANCOUNT(abuf); /*answer count*/
     for (int i=0; i < ancount && aptr ; i++)
     {
       struct in6_addr aaaa;
       aptr = parseAAAA(aptr,abuf,alen,&aaaa);
       if (aptr)
       {
         Tuple tuple(aaaa,mPort,mTransport);
         mResults.push_back(tuple);
       }
     }
   }
   else
   {
#ifdef USE_ARES
      char* errmem=0;
      DebugLog (<< "Failed async dns query: " << ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
#endif
   }
   lookupARecords(mPassHostFromAAAAtoA);
}

void
DnsResult::processHost(int status, struct hostent* result)
{
   DebugLog (<< "DnsResult::processHost() " << status);
   
   // This function assumes that the A query that caused this callback
   // is the _only_ outstanding DNS query that might result in a
   // callback into this function
   if ( mType == Destroyed )
   {
      destroy();
      return;
   }

   if (status == ARES_SUCCESS)
   {
      DebugLog (<< "DNS A lookup canonical name: " << result->h_name);
      Tuple tuple;
      tuple.port = mPort;
      tuple.transportType = mTransport;
      tuple.transport = 0;
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         tuple.ipv4.s_addr = *((u_int32_t*)(*pptr));
         mResults.push_back(tuple);
      }
   }
   else
   {
#ifdef USE_ARES
      char* errmem=0;
      DebugLog (<< "Failed async dns query: " << ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
#endif
   }

   if (mSRVCount == 0)
   {
      if (mResults.empty())
      {
         mType = Finished;
      }
      else 
      {
         mType = Available;
      }
      mHandler->handle(this);
   }
}

void
DnsResult::primeResults()
{
   DebugLog (<< "primeResults() " << mType);
#ifndef WIN32
   DebugLog (<< "SRV: " << Inserter(mSRVResults));
#endif

   //assert(mType != Pending);
   //assert(mType != Finished);
   assert(mResults.empty());

   if (!mSRVResults.empty())
   {
      SRV next = retrieveSRV();
      DebugLog (<< "Primed with SRV=" << next);
      
      if ( mARecords.count(next.target) + mAAAARecords.count(next.target) )
      {
         std::list<struct in6_addr>& aaaarecs = mAAAARecords[next.target];
         for (std::list<struct in6_addr>::const_iterator i=aaaarecs.begin();
	         i!=aaaarecs.end(); i++)
         {
            Tuple tuple(*i,next.port,next.transport);
            mResults.push_back(tuple);
         }
         std::list<struct in_addr>& arecs = mARecords[next.target];
         for (std::list<struct in_addr>::const_iterator i=arecs.begin(); i!=arecs.end(); i++)
         {
            Tuple tuple;
            tuple.port = next.port;
            tuple.transportType = next.transport;
            tuple.transport = 0;
            tuple.ipv4 = *i;
            mResults.push_back(tuple);
         }
#ifndef WIN32
         DebugLog (<< "Try: " << Inserter(mResults));
#endif

         mType = Available;
         mHandler->handle(this);
      }
      else
      {
         // !jf! not going to test this for now
         // this occurs when the A records were not provided in the Additional
         // Records of the DNS result
         // we will need to store the SRV record that is being looked up so we
         // can populate the resulting Tuples 
         mType = Pending;
         mPort = next.port;
         mTransport = next.transport;
         
         lookupAAAARecords(next.target);
      }

      // recurse if there are no results. This could happen if the DNS SRV
      // target record didn't have any A/AAAA records. This will terminate if we
      // run out of SRV results. 
      if (mResults.empty())
      {
         primeResults();
      }
   }
   else
   {
      mType = Finished;
   }

   // Either we are finished or there are results primed
   assert(mType == Finished || (mType == Available && !mResults.empty()));
}

DnsResult::SRV 
DnsResult::retrieveSRV()
{
   assert(!mSRVResults.empty());

   const SRV& srv = *mSRVResults.begin();
   if (srv.cumulativeWeight == 0)
   {
      int priority = srv.priority;
   
      mCumulativeWeight=0;
      for (std::set<SRV>::iterator i=mSRVResults.begin(); 
           i!=mSRVResults.end() && i->priority == priority; i++)
      {
         mCumulativeWeight += i->weight;
         SRV copy(srv);
         copy.cumulativeWeight = mCumulativeWeight;
         mSRVResults.erase(mSRVResults.begin());
         mSRVResults.insert(copy);
      }
   }
   
   int selected = Random::getRandom() % (mCumulativeWeight+1);

   DebugLog (<< "cumulative weight = " << mCumulativeWeight << " selected=" << selected);
#ifndef WIN32
   DebugLog (<< "SRV: " << Inserter(mSRVResults));
#endif

   std::set<SRV>::iterator i;
   for (i=mSRVResults.begin(); i!=mSRVResults.end(); i++)
   {
      if (i->cumulativeWeight >= selected)
      {
         break;
      }
   }
   
   assert(i != mSRVResults.end());
   SRV next = *i;
   mSRVResults.erase(i);

   return next;
}


// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::parseAdditional(const unsigned char *aptr,
                           const unsigned char *abuf, 
                           int alen)
{
   char *name=0;
   int len=0;
   int status=0;

#if !defined (USE_ARES)
#error foo
#endif

   // Parse the RR name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
   aptr += len;

   /* Make sure there is enough data after the RR name for the fixed
    * part of the RR.
    */
   if (aptr + RRFIXEDSZ > abuf + alen)
   {
      DebugLog (<< "Failed parse of RR");
      free(name);
      return NULL;
   }
  
   // Parse the fixed part of the RR, and advance to the RR data
   // field. 
   int type = DNS_RR_TYPE(aptr);
   int dlen = DNS_RR_LEN(aptr);
   aptr += RRFIXEDSZ;
   if (aptr + dlen > abuf + alen)
   {
      DebugLog (<< "Failed parse of RR");
      free(name);
      return NULL;
   }

   // Display the RR data.  Don't touch aptr. 
   Data key = name;
   free(name);

   if (type == T_SRV)
   {
      SRV srv;
      
      // The RR data is three two-byte numbers representing the
      // priority, weight, and port, followed by a domain name.
      srv.key = key;
      srv.priority = DNS__16BIT(aptr);
      srv.weight = DNS__16BIT(aptr + 2);
      srv.port = DNS__16BIT(aptr + 4);
      status = ares_expand_name(aptr + 6, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         return NULL;
      }
      srv.target = name;
      free(name);
      
      // Only add SRV results for the preferred NAPTR target as per rfc3263
      // (section 4.1). 
      assert(!mPreferredNAPTR.key.empty());
      if (srv.key == mPreferredNAPTR.replacement && srv.target != Symbols::DOT)
      {
         if (srv.key.find("_sip._udp") == 0)
         {
            srv.transport = UDP;
         }
         else if (srv.key.find("_sip._tcp") == 0)
         {
            srv.transport = TCP;
         }
         else if (srv.key.find("_sips._tcp") == 0)
         {
            srv.transport = TLS;
         }
         else
         {
            DebugLog (<< "Skipping SRV " << srv.key);
            return aptr + dlen;
         }
         
         DebugLog (<< "Inserting SRV: " << srv);
         mSRVResults.insert(srv);
      }

      return aptr + dlen;

   }
   else if (type == T_A)
   {
      // The RR data is a four-byte Internet address. 
      if (dlen != 4)
      {
         DebugLog (<< "Failed parse of RR");
         return NULL;
      }

      struct in_addr addr;
      memcpy(&addr, aptr, sizeof(struct in_addr));
      DebugLog (<< "From additional: " << key << ":" << DnsUtil::inet_ntop(addr));
      mARecords[key].push_back(addr);
      return aptr + dlen;
   }
   else if (type == T_AAAA)
   {
      if (dlen != 16) // The RR is 128 bits of ipv6 address
      {
         DebugLog (<< "Failed parse of RR");
         return NULL;
      }
      struct in6_addr addr;
      memcpy(&addr, aptr, sizeof(struct in6_addr));
      DebugLog (<< "From additional: " << key << ":" << DnsUtil::inet_ntop(addr));
      mAAAARecords[key].push_back(addr);
      return aptr + dlen;
   }
   else // just skip it (we don't care :)
   {
      //DebugLog (<< "Skipping: " << key);
      return aptr + dlen;
   }
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
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
   aptr += len;

   // Make sure there's enough data after the name for the fixed part
   // of the question.
   if (aptr + QFIXEDSZ > abuf + alen)
   {
      free(name);
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }

   // Parse the question type and class. 
   //int type = DNS_QUESTION_TYPE(aptr);
   //int dnsclass = DNS_QUESTION_CLASS(aptr);
   aptr += QFIXEDSZ;
   
   free(name);
   return aptr;
}

// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::parseSRV(const unsigned char *aptr,
                    const unsigned char *abuf, 
                    int alen,
                    SRV& srv)
{
   char *name;
   int len=0;
   int status=0;

   // Parse the RR name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
   aptr += len;

   /* Make sure there is enough data after the RR name for the fixed
    * part of the RR.
    */
   if (aptr + RRFIXEDSZ > abuf + alen)
   {
      free(name);
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
  
   // Parse the fixed part of the RR, and advance to the RR data
   // field. 
   int type = DNS_RR_TYPE(aptr);
   int dlen = DNS_RR_LEN(aptr);
   aptr += RRFIXEDSZ;
   if (aptr + dlen > abuf + alen)
   {
      free(name);
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
   Data key = name;
   free(name);

   // Display the RR data.  Don't touch aptr. 
   if (type == T_SRV)
   {
      // The RR data is three two-byte numbers representing the
      // priority, weight, and port, followed by a domain name.
      srv.key = key;
      srv.priority = DNS__16BIT(aptr);
      srv.weight = DNS__16BIT(aptr + 2);
      srv.port = DNS__16BIT(aptr + 4);
      status = ares_expand_name(aptr + 6, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         DebugLog (<< "Failed parse of RR");
         return NULL;
      }
      srv.target = name;
      free(name);

      return aptr + dlen;

   }
   else
   {
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
}
      

// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::parseAAAA(const unsigned char *aptr,
                     const unsigned char *abuf, 
                     int alen,
                     in6_addr * aaaa)
{
   char *name;
   int len=0;
   int status=0;

   // Parse the RR name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
   aptr += len;

   /* Make sure there is enough data after the RR name for the fixed
    * part of the RR.
    */
   if (aptr + RRFIXEDSZ > abuf + alen)
   {
      free(name);
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
  
   // Parse the fixed part of the RR, and advance to the RR data
   // field. 
   int type = DNS_RR_TYPE(aptr);
   int dlen = DNS_RR_LEN(aptr);
   aptr += RRFIXEDSZ;
   if (aptr + dlen > abuf + alen)
   {
      free(name);
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
   Data key = name;
   free(name);

   // Display the RR data.  Don't touch aptr. 
   if (type == T_AAAA)
   {
      // The RR data is 128 bits of ipv6 address in network byte
      // order
      memcpy(aaaa, aptr, sizeof(in6_addr));
      return aptr + dlen;
   }
   else
   {
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
}

// adapted from the adig.c example in ares
const unsigned char* 
DnsResult::parseNAPTR(const unsigned char *aptr,
                      const unsigned char *abuf, 
                      int alen,
                      NAPTR& naptr)
{
   const unsigned char* p=0;
   char* name=0;
   int len=0;
   int status=0;

   // Parse the RR name. 
   status = ares_expand_name(aptr, abuf, alen, &name, &len);
   if (status != ARES_SUCCESS)
   {
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
   aptr += len;

   // Make sure there is enough data after the RR name for the fixed
   // part of the RR.
   if (aptr + RRFIXEDSZ > abuf + alen)
   {
      free(name);
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
  
   // Parse the fixed part of the RR, and advance to the RR data
   // field. 
   int type = DNS_RR_TYPE(aptr);
   int dlen = DNS_RR_LEN(aptr);
   aptr += RRFIXEDSZ;
   if (aptr + dlen > abuf + alen)
   {
      free(name);
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }

   Data key = name;
   free(name);

   // Look at the RR data.  Don't touch aptr. 
   if (type == T_NAPTR)
   {
      // The RR data is two two-byte numbers representing the
      // order and preference, followed by three character strings
      // representing flags, services, a regex, and a domain name.

      naptr.key = key;
      naptr.order = DNS__16BIT(aptr);
      naptr.pref = DNS__16BIT(aptr + 2);
      p = aptr + 4;
      len = *p;
      if (p + len + 1 > aptr + dlen)
      {
         DebugLog (<< "Failed parse of RR");
         return NULL;
      }
      naptr.flags = Data(p+1, len);
      
      p += len + 1;
      len = *p;
      if (p + len + 1 > aptr + dlen)
      {
         DebugLog (<< "Failed parse of RR");
         return NULL;
      }
      naptr.service = Data(p+1, len);

      p += len + 1;
      len = *p;
      if (p + len + 1 > aptr + dlen)
      {
         DebugLog (<< "Failed parse of RR");
         return NULL;
      }
      naptr.regex = Data(p+1, len);

      p += len + 1;
      status = ares_expand_name(p, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         DebugLog (<< "Failed parse of RR");
         return NULL;
      }
      naptr.replacement = name;
      
      free(name);
      return aptr + dlen;
   }
   else
   {
      DebugLog (<< "Failed parse of RR");
      return NULL;
   }
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

DnsResult::SRV::SRV() : cumulativeWeight(0)
{
}

bool 
DnsResult::SRV::operator<(const DnsResult::SRV& rhs) const
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
         if (transport < rhs.transport)
         {
            return true;
         }
         else if (transport == rhs.transport)
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

std::ostream& 
resip::operator<<(std::ostream& strm, const resip::DnsResult& result)
{
   strm << "target=" << result.mTarget;
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
        << " regex=" << naptr.regex
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
        << " c=" << srv.cumulativeWeight
        << " port=" << srv.port
        << " target=" << srv.target;
   return strm;
}


/* 
   Copyright (c) 2003, Jason Fischl
 
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    
   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
    
   * Neither the name of any of the copyright holders nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.
    
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
