#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#endif

#include <stdio.h>
#include <errno.h>

#include "sip2/util/compat.hxx"
#include "sip2/util/Socket.hxx"
#include "sip2/util/Logger.hxx"

#include "sip2/sipstack/DnsResolver.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/sipstack/ParserCategories.hxx"
#include "sip2/sipstack/SipStack.hxx"
#include "sip2/sipstack/TransactionState.hxx"


#define VOCAL_SUBSYSTEM Vocal2::Subsystem::TRANSACTION

using namespace Vocal2;
using namespace std;

DnsResolver::DnsResolver(SipStack& stack) : mStack(stack)
{
#if defined(USE_ARES)
   int status=0;
   if ((status = ares_init(&mChannel)) != ARES_SUCCESS)
   {
      ErrLog (<< "Failed to initialize async dns library (ares)");
      char* errmem=0;
      ErrLog (<< ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
      throw Exception("failed to initialize ares", __FILE__,__LINE__);
   }
#endif
}


DnsResolver::~DnsResolver()
{
#if defined(USE_ARES)
   ares_destroy(mChannel);
#endif
}


void
DnsResolver::buildFdSet(FdSet& fdset)
{
#if defined(USE_ARES)
   int size = ares_fds(mChannel, &fdset.read, &fdset.write);
   if ( size > fdset.size )
   {
      fdset.size = size;
   }
#endif
}

void
DnsResolver::process(FdSet& fdset)
{
#if defined(USE_ARES)
   ares_process(mChannel, &fdset.read, &fdset.write);
#endif
}


static int 
determinePort(const Data& scheme, Transport::Type transport)
{
   if ( isEqualNoCase(scheme, Symbols::Sips) || (transport == Transport::TLS) )
   {
      return Symbols::DefaultSipsPort;
   }
   
   return Symbols::DefaultSipPort;
}


static Data
determineSrvPrefix(const Data& scheme, Transport::Type transport)
{
   Data prefix;

   if ( isEqualNoCase(scheme, Symbols::Sips) )
   {
      prefix = Symbols::SrvSips;
   }
   else
   {
      prefix = Symbols::SrvSip;
   }

   if ( (transport == Transport::TLS) || (transport == Transport::TCP) )
   {
      prefix += "." + Symbols::SrvTcp;
   }
   else
   {
      prefix += "." + Symbols::SrvUdp;
   }

   return prefix;
}

void
DnsResolver::lookup(const Data& transactionId, const Via& via)
{
   //duplicate entry has not been eliminated
   Transport::Type transport = Transport::toTransport(via.transport());
   Data& target = via.exists(p_maddr) ? via.param(p_maddr) : via.sentHost();
   if (via.exists(p_received))
   {
      if (via.exists(p_rport))
      {
         lookupARecords(transactionId, via.param(p_received), via.param(p_rport).port(), transport);
         // try with via.sentPort() too, even if via.exists(p_rport)?
      }
      else
      {
         if (via.sentPort())
         {
            lookupARecords(transactionId, via.param(p_received), via.sentPort(), transport);
            // try with default port too, even if via.sentPort()?
         }
         else
         {
            lookupARecords(transactionId, via.param(p_received),  determinePort(via.protocolName(), transport), transport);
         }
      }
   }
   else if (via.exists(p_rport))
   {
      lookupARecords(transactionId, target,  via.param(p_rport).port(), transport);
   }
   else if (via.sentPort())
   {
      lookupARecords(transactionId, target, via.sentPort(), transport);
      // try with default port too, even if via.sentPort()?
   }
   else
   {
      lookupARecords(transactionId, target, determinePort(via.protocolName(), transport), transport);
   }
}


void
DnsResolver::lookup(const Data& transactionId, const Uri& uri)
{
   Data& target = uri.exists(p_maddr) ? uri.param(p_maddr) : uri.host();
   bool isNumeric = isIpAddress(target);
   int port;
   
   Transport::Type transport;

   if (uri.scheme() == Symbols::Sips)
   {
      transport = Transport::TLS;
   }
   else
   {
      if (uri.exists(p_transport))
      {
         transport = Transport::toTransport(uri.param(p_transport));
      }
      else
      {
         if (isNumeric || uri.port() != 0)
         {
            if (uri.scheme() == Symbols::Sip)
            {
               transport = Transport::UDP;
            }
            else if (uri.scheme() == Symbols::Sips)
            {
               transport = Transport::TCP;
            }
            else
            {
               assert(0);
               mStack.mStateMacFifo.add(new DnsMessage(transactionId));
               return;
            }
         }
         else
         {
#if 0
            /*
            !rk! Perform NAPTR and SRV lookups per RFC 3263 sections
            4.1 & 4.2.	We could get a list of targets here.  You also have
            to filter out results that are not usable (e.g. SCTP entries if
            you don't support that, SIPS entries if you don't do TLS).

            1. Foreach NAPTR result in the sorted/ordered list of compatible
               NAPTR records => "R" (Replacement) and "T" (transport) {
        	 Foreach SRV result in the sorted list of SRV records
        	 found for R => "H" (Host) and "P" (Port) {
        	   Foreach A/AAAA result found for H => "A" {
        	     lookupARecords(transactionId, A, P, T);
        	   }
        	 }
               }

            2. If more results are needed, choose TCP for SIPS and UDP
            for SIP.  Do an SRV lookup on the target using _sips or
            _sip as appropriate along with _tcp or _udp as determined
            in the last sentence.  If the SRV query returns no results,
            resolve the target into a list of A/AAAA records to try
            in sequence with the determined protocol and transport and
            default port, 5060 for SIP or 5061 for SIPS.
            */

            return;
#else
            DebugLog(<<"Should be doing NAPTR+SRV per RFC 3263 s4.1, 4.2");
            transport = Transport::UDP;

#if defined(USE_ARES)
            DebugLog(<<"For now assuming UDP and just doing SRV");
            port = determinePort(uri.scheme(), transport);
            Request* request = new Request(mStack, transactionId,
               target, port, transport);
            Data srvTarget =
               determineSrvPrefix(uri.scheme(), transport)
               + "." + target;
            ares_query(mChannel, srvTarget.c_str(), C_IN, T_SRV,
               DnsResolver::aresCallbackSrv, request);
            return;
#endif
#endif
         }
      }
   }

   if (uri.port())
   {
      port = uri.port();
   }
   else
   {
      port = determinePort(uri.scheme(), transport);
   }
         
   lookupARecords(transactionId, target, port, transport);         
}
 

void
DnsResolver::lookupARecords(const Data& transactionId, const Data& host, int port, Transport::Type transport)

{
   TransactionState* txn = mStack.mTransactionMap.find(transactionId);
   if (!txn)
   {
      DebugLog(<< "DNS lookup for non-existent transaction");
      return;
   }
   txn->registerDnsLookup();

#if defined(USE_ARES)
   Request* request = new Request(mStack, transactionId, host, port, transport);
   ares_gethostbyname(mChannel, host.c_str(), AF_INET, DnsResolver::aresCallbackHost, request);
#else   
   struct hostent* result=0;
   int ret=0;
   int herrno=0;

#if defined(__linux__)
   struct hostent hostbuf; 
   char buffer[8192];
   ret = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
   assert (ret != ERANGE);
#elif defined(WIN32) 
   result = gethostbyname( host.c_str() );
   herrno = WSAGetLastError();
#elif defined( __MACH__ ) || defined (__FreeBSD__)
   result = gethostbyname( host.c_str() );
   herrno = h_errno;
#elif defined(__QNX__) || defined(__SUNPRO_CC)
   struct hostent hostbuf; 
   char buffer[8192];
   result = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno );
#else
#   error "need to define some version of gethostbyname for your arch"
#endif

   if ( (ret!=0) || (result==0) )
   {
      switch (herrno)
      {
         case HOST_NOT_FOUND:
            InfoLog ( << "host not found: " << host);
            break;
         case NO_DATA:
            InfoLog ( << "no data found for: " << host);
            break;
         case NO_RECOVERY:
            InfoLog ( << "no recovery lookup up: " << host);
            break;
         case TRY_AGAIN:
            InfoLog ( << "try again: " << host);
            break;
		 default:
			 ErrLog( << "DNS Resolver got error" << herrno << " looking up " << host );
			 assert(0);
			 break;
      }
   }
   else
   {
      assert(result);
      assert(result->h_length == 4);
      
      DnsMessage* dns = new DnsMessage(transactionId);
      DebugLog (<< "DNS lookup of " << host << ": canonical name: " << result->h_name);
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         Transport::Tuple tuple;
         tuple.ipv4.s_addr = *((u_int32_t*)(*pptr));
         tuple.port = port;
         tuple.transportType = transport;
         tuple.transport = 0;
         
         DebugLog(<< tuple);
         dns->mTuples.push_back(tuple);
      }
      mStack.mStateMacFifo.add(dns);
   }
#endif
}

bool
DnsResolver::isIpAddress(const Data& data)
{
   // ok, this is fairly monstrous but it works. 
    // !cj! - won't work for IPV6
   unsigned int p1,p2,p3,p4;
   int count=0;
   int result = sscanf( data.c_str(), 
                        "%u.%u.%u.%u%n",
                        &p1, &p2, &p3, &p4, &count );

   if ( (result == 4) && (p1 <= 255) && (p2 <= 255) && (p3 <= 255) && (p4 <= 255) && (count == int(data.size())) )
   {
      return true;
   }
   else
   {
      return false;
   }
} 

void 
DnsResolver::aresCallbackHost(void *arg, int status, struct hostent* result)
{
#if defined(USE_ARES)
   std::auto_ptr<Request> request(reinterpret_cast<Request*>(arg));

   DebugLog (<< "Received dns update: " << request->tid);
   DnsMessage* dns = new DnsMessage(request->tid);

   if (status != ARES_SUCCESS)
   {
      char* errmem=0;
      InfoLog (<< "Failed async dns query: " << ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
   }
   else
   {
      DebugLog (<< "DNS lookup canonical name: " << result->h_name);
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
	 Transport::Tuple tuple;
	 tuple.ipv4.s_addr = *((u_int32_t*)(*pptr));
	 tuple.port = request->port;
	 tuple.transportType = request->transport;
	 tuple.transport = 0;
	 
	 DebugLog(<< tuple);
	 dns->mTuples.push_back(tuple);
      }
   }

   request->stack.mStateMacFifo.add(dns);
#endif
}


void
DnsResolver::aresCallbackSrv(void *arg, int pstatus,
   unsigned char *abuf, int alen)
{
#if defined(USE_ARES)
   std::auto_ptr<Request> request(reinterpret_cast<Request*>(arg));
   int status, len;
   char *name, *errmem;
   const unsigned char *aptr = abuf + HFIXEDSZ;
   DnsMessage *dns = new DnsMessage(request->tid);

   DebugLog (<< "Received SRV result for: " << request->tid << " for "
             << request->host);

   if (pstatus != ARES_SUCCESS)
   {
      errmem = 0;
      InfoLog (<< "SRV lookup failed: " << ares_strerror(pstatus, &errmem));
      ares_free_errmem(errmem);
      goto SrvParseDone;
   }

   // !rk!  Throw away all of the "questions" in the DNS packet.  Is there
   // not just a way to skip right over these without iterating like this?
   for (int i = 0; i < DNS_HEADER_QDCOUNT(abuf); i++)
   {
      status = ares_expand_name(aptr, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         errmem = 0;
         InfoLog (<< "Bad DNS question: " << ares_strerror(status, &errmem));
         ares_free_errmem(errmem);
         goto SrvParseDone;
      }
      free(name);
      aptr += len + QFIXEDSZ;
   }

   for (int i = 0; i < DNS_HEADER_ANCOUNT(abuf); i++)
   {
      Srv srv;

      status = ares_expand_name(aptr, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         errmem = 0;
         InfoLog (<< "Bad DNS RR: " << ares_strerror(status, &errmem));
         ares_free_errmem(errmem);
         goto SrvParseDone;
      }

      aptr += len;
      int dlen = DNS_RR_LEN(aptr);
      aptr += RRFIXEDSZ;

      srv.priority = DNS__16BIT(aptr);
      srv.weight = DNS__16BIT(aptr+2);
      srv.port = DNS__16BIT(aptr+4);

      status = ares_expand_name(aptr+6, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
      {
         errmem = 0;
         InfoLog (<< "Bad DNS answer: " << ares_strerror(status, &errmem));
         ares_free_errmem(errmem);
         goto SrvParseDone;
      }
      srv.host = name;
      srv.transport = request->transport;
      dns->mSrvs.insert(srv);

      free(name);
      aptr += dlen;
   }

SrvParseDone:
   if (!dns->mSrvs.size())
   {
      // Add the request target as the only result
      InfoLog (<< "Adding fallback SRV to queue A/AAAA lookup");
      Srv srv;
      srv.priority = 65535;
      srv.weight = 0;
      srv.port = request->port;
      srv.host = request->host;
      srv.transport = request->transport;
      dns->mSrvs.insert(srv);
   }

   for (SrvIterator s = dns->mSrvs.begin(); s != dns->mSrvs.end(); s++)
   {
      DebugLog(<< "SRV entry " << s->host << " with priority " << s->priority);
   }

   request->stack.mStateMacFifo.add(dns);
#endif
}


Data 
DnsResolver::DnsMessage::brief() const 
{ 
   Data result;
   DataStream strm(result);
   strm << "DnsMessage: tid=" << mTransactionId;
   // could output the tuples
   strm.flush();
   return result;
}

std::ostream& 
DnsResolver::DnsMessage::encode(std::ostream& strm) const
{
   strm << "Dns: tid=" << mTransactionId;
   for (DnsResolver::TupleIterator i=mTuples.begin(); i != mTuples.end(); i++)
   {
      strm << *i << ",";
   }
   return strm;
}
