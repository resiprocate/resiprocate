#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <sys/types.h>
#endif

#include <stdio.h>
#include <errno.h>

#include "resiprocate/util/Socket.hxx"

#include "resiprocate/sipstack/Symbols.hxx"
#include "resiprocate/sipstack/Uri.hxx"

#include "resiprocate/util/Socket.hxx"
#include "resiprocate/util/Data.hxx"
#include "resiprocate/util/Logger.hxx"
#include "resiprocate/util/ParseBuffer.hxx"

#include "Resolver.hxx"

#define VOCAL_SUBSYSTEM Vocal2::Subsystem::SIP

using namespace Vocal2;

Resolver::Resolver(const Uri& uri) : 
   mHost(uri.host()),
   mPort(uri.port() ? uri.port() : 5060)
{
   bool isNumeric = isIpAddress(mHost);
   if (!uri.exists(p_transport) )
   {
      if (isNumeric)
      {
         if (uri.scheme() == Symbols::Sip)
         {
            mTransport = Transport::UDP;
         }
         else if (uri.scheme() == Symbols::Sips)
         {
            mTransport = Transport::TCP;
         }
      }
      else // not numeric
      {
         if (1) // uri.portSpecified()) // !jf!
         {
            if (uri.scheme() == Symbols::Sip)
            {
               mTransport = Transport::UDP;
            }
            else if (uri.scheme() == Symbols::Sips)
            {
               mTransport = Transport::TCP;
            }
         }
         else // NAPTR query - yuck! 
         {
            // Rohan, step up to the plate buddy.
            mTransport = Transport::UDP; // !jf! not done yet
         }
      }
   }

   if (!isNumeric)
   {
      if (1) // !jf! uri.portSpecified())
      {
         lookupARecords();
         // do an A or AAAA DNS lookup
         
      }
      else
      {
         // do SRV lookup on result of NAPTR lookup
         
         // if no NAPTR lookup, do SRV lookup on _sips for sips and _sip for sip
         
         // if no result on SRV lookup, do an A or AAAA lookup
         
      }
   }
   else
   {
      Transport::Tuple tuple;
      if (inet_pton(AF_INET, mHost.c_str(), &tuple.ipv4.s_addr) <= 0)
      {
         DebugLog( << "inet_pton failed to parse address: " << mHost << " " << strerror(errno));
         assert(0);
      }
      tuple.port = mPort;
      tuple.transportType = mTransport;
      
      mNextHops.push_back(tuple);
   }
   
}


Resolver::Resolver(const Data& host, int port, Transport::Type transport) 
   :  mTransport(transport),
      mHost(host),
      mPort(port)
{
   bool isNumeric = isIpAddress(mHost);
   if (!isNumeric)
   {
      if (1) // !jf! uri.portSpecified())
      {
         lookupARecords();
         // do an A or AAAA DNS lookup
         
      }
      else
      {
         // do SRV lookup on result of NAPTR lookup
         
         // if no NAPTR lookup, do SRV lookup on _sips for sips and _sip for sip
         
         // if no result on SRV lookup, do an A or AAAA lookup
         
      }
   }
   else
   {
      Transport::Tuple tuple;
      if (inet_pton(AF_INET, mHost.c_str(), &tuple.ipv4.s_addr) <= 0)
      {
         DebugLog( << "inet_pton failed to parse address: " << mHost << " " << strerror(errno));
         assert(0);
      }
      tuple.port = mPort;
      tuple.transportType = mTransport;
      
      mNextHops.push_back(tuple);
   }
}


void
Resolver::lookupARecords()
{
   struct hostent hostbuf; 
   struct hostent* result=0;

   int herrno=0;
   char buffer[8192];
#ifdef __QNX__
   result = gethostbyname_r (mHost.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno);
   if (result == 0)
   {
#else

#if defined( WIN32 ) || defined( __MACH__ ) || defined (__SUNPRO_CC) || defined(__FreeBSD__)
	assert(0); // !cj! 
	int ret = -1;
#else
        int ret = gethostbyname_r (mHost.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
#endif
   assert (ret != ERANGE);

   if (ret != 0)
   {
#endif
      switch (herrno)
      {
         case HOST_NOT_FOUND:
            InfoLog ( << "host not found: " << mHost);
            break;
         case NO_DATA:
            InfoLog ( << "no data found for: " << mHost);
            break;
         case NO_RECOVERY:
            InfoLog ( << "no recovery lookup up: " << mHost);
            break;
         case TRY_AGAIN:
            InfoLog ( << "try again: " << mHost);
            break;
      }
   }
   else
   {
      assert(result);
      assert(result->h_length == 4);
   
      DebugLog (<< "DNS lookup of " << mHost << ": canonical name: " << result->h_name);
      char str[256];
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         Transport::Tuple tuple;
         tuple.ipv4.s_addr = *((u_int32_t*)(*pptr));
         tuple.port = mPort;
         tuple.transportType = mTransport;

         mNextHops.push_back(tuple);
#ifndef WIN32
         DebugLog (<< inet_ntop(AF_INET, &tuple.ipv4.s_addr, str, sizeof(str)));
#endif

      }
   }
}

bool
Resolver::isIpAddress(const Data& data)
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
 

Data
Resolver::getHostName()
{
   char buffer[255];
   if (gethostname(buffer, sizeof(buffer)) < 0)
   {
      InfoLog (<< "Failed gethostname() " << strerror(errno));
      return "localhost";
   }
   else
   {
      return Data(buffer);
   }
}
