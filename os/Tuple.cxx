#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <cassert>

#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/HashMap.hxx"

using namespace resip;

Tuple::Tuple() : 
   v6(false),
   port(0), 
   transportType(UNKNOWN_TRANSPORT), 
   transport(0),
   connection(0)
{
   memset(&ipv4, 0, sizeof(ipv4));
#ifdef USE_IPV6
   memset(&ipv6, 0, sizeof(ipv6));
#endif
}

Tuple::Tuple(const in_addr& pipv4,
             int pport,
             TransportType ptype)
   : v6(false),
     ipv4(pipv4),
     port(pport),
     transportType(ptype),
     transport(0),
     connection(0)
{
}

#ifdef USE_IPV6
Tuple::Tuple(const in6_addr& pipv6,
             int pport,
             TransportType ptype)
   : v6(true),
     ipv6(pipv6),
     port(pport),
     transportType(ptype),
     transport(0),
     connection(0)
{
}
#endif


bool Tuple::operator==(const Tuple& rhs) const
{
   if (v6 && rhs.v6)
   {
#if USE_IPV6
	   return ( (memcmp(&ipv6, &ipv6, sizeof(ipv6)) == 0) &&
               (port == rhs.port) &&
               (transportType == rhs.transportType));
#else
		assert(0);
		return false;
#endif	
   }
   else if (!v6 && !rhs.v6)
   {
      return ( (memcmp(&ipv4, &ipv4, sizeof(ipv4)) == 0) &&
               (port == rhs.port) &&
               (transportType == rhs.transportType));
   }
   else
   {
      return false;
   }
   
   // !dlb! don't include connection 
}

bool Tuple::operator<(const Tuple& rhs) const
{
   int c=0;
   
   if (v6 && rhs.v6)
   {
      c = memcmp(&ipv4, &rhs.ipv4, sizeof(ipv4));
   }
   else if (!v6 && !rhs.v6)
   {
      c = memcmp(&ipv4, &ipv4, sizeof(ipv4));
   }
   else if (v6 && !rhs.v6)
   {
      return true;
   }
   else if (!v6 && rhs.v6)
   {
      return false;
   }
   else
   {
      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (port < rhs.port)
      {
         return true;
      }
      else if (port > rhs.port)
      {
         return false;
      }
      else
      {
         return transportType < rhs.transportType;
      }
   }
   // !jf!
   return false;
}

std::ostream&
resip::operator<<(std::ostream& ostrm, const Tuple& tuple)
{
	ostrm << "[ " ;

#if defined(WIN32) 
//	ostrm   << inet_ntoa(tuple.ipv4);
	
#else	
	char str[256];
    if (tuple.v6)
    {
       ostrm << inet_ntop(AF_INET6, &tuple.ipv6.s6_addr, str, sizeof(str));       
    }
    else
    {
       ostrm << inet_ntop(AF_INET, &tuple.ipv4.s_addr, str, sizeof(str));
    }
#endif	
	
	ostrm  << " , " 
	       << tuple.port
	       << " , "
	       << Tuple::toData(tuple.transportType) 
	       << " ,transport="
	       << tuple.transport 
           << " ,connection=" 
           << tuple.connection
	       << " ]";
	
	return ostrm;
}


#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )

size_t 
__gnu_cxx::hash<resip::Tuple>::operator()(const resip::Tuple& tuple) const
{
   // !dlb! do not include the connection
   if (tuple.v6)
   {
      return size_t(tuple.ipv6.s6_addr + 5*tuple.port + 25*tuple.transportType);
   }
   else
   {
      return size_t(tuple.ipv4.s_addr + 5*tuple.port + 25*tuple.transportType);
   }
}

#elif  defined(__INTEL_COMPILER )
size_t 
std::hash_value(const resip::Tuple& tuple) 
{
   // !dlb! do not include the connection
   if (tuple.v6)
   {
      return size_t(tuple.ipv6.s6_addr + 5*tuple.port + 25*tuple.transportType);
   }
   else
   {
      return size_t(tuple.ipv4.s_addr + 5*tuple.port + 25*tuple.transportType);
   }
}

#endif

static const Data transportNames[MAX_TRANSPORT] =
{
   Data("UNKNOWN_TRANSPORT"),
   Data("UDP"),
   Data("TCP"),
   Data("TLS"),
   Data("SCTP"),
   Data("DCCP")
};

TransportType
Tuple::toTransport(const Data& type)
{
   for (TransportType i = UNKNOWN_TRANSPORT; i < MAX_TRANSPORT; 
        i = static_cast<TransportType>(i + 1))
   {
      if (isEqualNoCase(type, transportNames[i]))
      {
         return i;
      }
   }
   return UNKNOWN_TRANSPORT;
};

const Data&
Tuple::toData(TransportType type)
{
   assert(type >= UNKNOWN_TRANSPORT && type < MAX_TRANSPORT);
   return transportNames[type];
}



