#if !defined(RESIP_TUPLE_HXX)
#define RESIP_TUPLE_HXX

#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/HashMap.hxx"

#include "resiprocate/os/compat.hxx"

#ifdef WIN32

#else
#include <netinet/in.h>
#endif

namespace resip
{

typedef enum 
{
   UNKNOWN_TRANSPORT = 0,
   UDP,
   TCP,
   TLS,
   SCTP,
   DCCP,
   MAX_TRANSPORT
} TransportType;

class Data;
class Transport;
class Connection;

// WARNING!!
// When you change this structure, make sure to update the hash function,
// operator== and operator< to be consistent with the new structure. For
// instance, the Connection* and Transport* change value in the Tuple over
// its lifetime so they must not be included in the hash or comparisons. 

class Tuple
{
   public:
      Tuple();
#ifdef USE_IPV6
	  Tuple(const in6_addr& pipv6,
            int pport,
            TransportType ptype);
#endif
	  Tuple(const in_addr& pipv4,
            int pport,
            TransportType ptype);
      
      struct sockaddr* sockaddr() const;

      bool operator<(const Tuple& rhs) const;
      bool operator==(const Tuple& rhs) const;
      
      bool v6;
      struct in_addr ipv4;
#ifdef USE_IPV6
      struct in6_addr ipv6;
#endif

      int port;
      TransportType transportType;
      Transport* transport;
      Connection* connection;
      
      static TransportType toTransport( const Data& );
      static const Data& toData( TransportType );
};

std::ostream& operator<<(std::ostream& strm, const Tuple& tuple);

}

#if  defined(__INTEL_COMPILER )

namespace std
{
size_t hash_value(const resip::Tuple& tuple);
}

#elif defined(HASH_MAP_NAMESPACE)

namespace __gnu_cxx
{

struct hash<resip::Tuple>
{
      size_t operator()(const resip::Tuple& addrPort) const;
};
 
}

#endif // hash stuff


#endif
