
#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if defined(__sparc)
#include <inttypes.h>
/* typedef unsigned char u_int8_t; */
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#include "sip2/util/Socket.hxx"
#include "sip2/util/Logger.hxx"

#include "sip2/sipstack/DnsResolver.hxx"
#include "sip2/sipstack/DnsMessage.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/sipstack/ParserCategories.hxx"
#include "sip2/sipstack/SipStack.hxx"


#define VOCAL_SUBSYSTEM Vocal2::Subsystem::SIP

using namespace Vocal2;
using namespace std;

DnsResolver::~DnsResolver()
{
}

void 
DnsResolver::stop(Id id)
{
   delete id;
}

int 
determinePort(const Data& scheme, Transport::Type transport)
{
   if (isEqualNoCase(scheme, Symbols::Sip))
   {
      if (transport != Transport::TLS)
      {
         return Symbols::DefaultSipPort;
      }
      return Symbols::SipTlsPort;
   }
   else
   {
      return Symbols::DefaultSipsPort;
   }
}

void
DnsResolver::lookup(const Data& transactionId,
                    const Via& via)
{
   //duplicate entry has not been eliminated
   DnsResolver::Id id = 0;
   Transport::Type transport = Transport::toTransport(via.transport());
   Data& target = via.exists(p_maddr) ? via.param(p_maddr) : via.sentHost();
   if (via.exists(p_received))
   {
      if (via.exists(p_rport))
      {
         id = lookupARecords(transactionId, via.param(p_received), via.param(p_rport), transport, false);
      }
      else
      {
         if (via.sentPort())
         {
            id = lookupARecords(transactionId, via.param(p_received), via.sentPort(), transport, false);
         }
         else
         {
            id = lookupARecords(transactionId, via.param(p_received), 
                                determinePort(via.protocolName(), transport), 
                                transport, false);
         }
      }
   }
   else if (via.exists(p_rport))
   {
      id = lookupARecords(transactionId,
                          target, 
                          via.param(p_rport), 
                          transport, false);
   }
   
   if (via.sentPort())
   {
      id = lookupARecords(transactionId,
                          target, 
                          via.sentPort(), 
                          transport, true, id);
   }
   else
   {
      id = lookupARecords(transactionId,
                          target, 
                          determinePort(via.protocolName(), transport), 
                          transport, true, id);
   }
}


void
DnsResolver::lookup(const Data& transactionId, const Uri& uri)
{
   Data& target = uri.exists(p_maddr) ? uri.param(p_maddr) : uri.host();
   bool isNumeric = isIpAddress(target);
   int port;
   
   Transport::Type transport;
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
            Entry* entry = new Entry(transactionId);
            mStack.mStateMacFifo.add(
               new DnsMessage(entry, 
                              transactionId, 
                              entry->tupleList.begin(), 
                              entry->tupleList.end(), 
                              true));
            return;
         }
      }
      else
      {
         //should be doing Naptr, Srv, a la 3263 Sec 4.1
         transport = Transport::UDP;
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
         
   lookupARecords(transactionId, target, port, transport, true);         
}
 

DnsResolver::Id
DnsResolver::lookupARecords(const Data& transactionId, 
                            const Data& host, int port, Transport::Type transport, bool complete, Id id)
{
   struct hostent hostbuf; 
   struct hostent* result=0;
   int ret=0;
   int herrno=0;
   char buffer[8192];

#if defined(__linux__)
   ret = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
   assert (ret != ERANGE);
#elif defined(WIN32)
   result = gethostbyname( host.c_str() );
   herrono = h_error;
#elif defined(__QNX__) || defined(__SUNPRO_CC)
   result = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno );
#else
#error "need to define some version of gethostbyname for your arch"
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
      }
   }
   else
   {
      assert(result);
      assert(result->h_length == 4);
      
      Entry* entry;
      if (id)
      {
         entry = id;
      }
      else
      {
         entry = new Entry(transactionId);
      }
      
      DebugLog (<< "DNS lookup of " << host << ": canonical name: " << result->h_name);
      bool first = true;
      TupleList::iterator start;
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         Transport::Tuple tuple;
         tuple.ipv4.s_addr = *((u_int32_t*)(*pptr));
         tuple.port = port;
         tuple.transportType = transport;
         tuple.transport = 0;
         
         entry->tupleList.push_back(tuple);
         if (first)
         {
            start = entry->tupleList.end();
            start--;
            first = false;
         }
         
#ifdef WIN32
         DebugLog (<< inet_ntoa(tuple.ipv4) << ":" << port);
#else
	 char str[256];
         DebugLog (<< inet_ntop(result->h_addrtype, &tuple.ipv4.s_addr, str, sizeof(str)) << ":" << port);
#endif

      }
      mStack.mStateMacFifo.add(new DnsMessage(entry, transactionId, start, entry->tupleList.end(), complete));
      return entry;
   }
   return 0;
}

bool
DnsResolver::isIpAddress(const Data& data)
{
   // ok, this is fairly monstrous but it works. 
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


