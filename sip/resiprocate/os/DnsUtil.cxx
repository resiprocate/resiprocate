#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::UTIL

using namespace resip;
using namespace std;

Data 
DnsUtil::getLocalHostName()
{
   char buffer[256];
   if (int e = gethostname(buffer,256) == -1)
   {
       if ( e != 0 )
       {
           int err = errno;
           switch (err)
           {
               case WSANOTINITIALISED:
                   CritLog( << "could not find local hostname because netwrok not initialized:" << strerror(err) );
                   break;
               default:
                   CritLog( << "could not find local hostname:" << strerror(err) );
                   break;
           }
           throw Exception("could not find local hostname",__FILE__,__LINE__);
       }
   }
   return buffer;
}


Data 
DnsUtil::getLocalDomainName()
{
#if defined( __MACH__ ) || defined( WIN32 )
   assert(0);
 // !cj! TODO 
   return NULL;
#else
   char buffer[1024];
   if (int e = getdomainname(buffer,sizeof(buffer)) == -1)
   {
       if ( e != 0 )
       {
           int err = errno;
           CritLog(<< "Couldn't find domainname: " << strerror(err));
           throw Exception(strerror(err), __FILE__,__LINE__);
       }
   }
   return buffer;
#endif
}

Data
DnsUtil::inet_ntop(const struct in_addr& addr)
{
	  char str[256];
#if !defined(WIN32)
    ::inet_ntop(AF_INET, (u_int32_t*)(&addr), str, sizeof(str));
#else
   // !cj! TODO 
	u_int32_t i = *((u_int32_t*)(&addr));
	sprintf(str,"%d.%d.%d.%d\0",(i>>24)&0xFF,(i>>16)&0xFF,(i>>8)&0xFF,(i)&0xFF);
#endif
	return Data(str);
}

Data
DnsUtil::inet_ntop(const struct in6_addr& addr)
{
	 char str[256];
#if !defined(WIN32)
    ::inet_ntop(AF_INET6, (u_int32_t*)(&addr), str, sizeof(str));
#else
	 u_int32_t s[8];
	 u_int32_t* p = (u_int32_t*)(&addr);
	 for (int i=0;i<8;i++)
	 {
		 s[i] = *(p++);
	 }
	 sprintf(str,"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",s[7],s[6],s[5],s[4],s[3],s[2],s[1],s[0]);
#endif
	   return Data(str);
}

int
DnsUtil::inet_pton(const Data& printableIp, struct in_addr& dst)
{
#if !defined(WIN32)
   return ::inet_pton(AF_INET, printableIp.c_str(), &dst);
#else
   // !cj! TODO
	unsigned long i = inet_addr(printableIp.c_str());
	if ( i == INADDR_NONE )
	{
		return 0;
	}
	u_int32_t* d = reinterpret_cast<u_int32_t*>( &dst );
    *d = i;
   return 1;
#endif   
}

int
DnsUtil::inet_pton(const Data& printableIp, struct in6_addr& dst)
{
#if !defined(WIN32)
   return ::inet_pton(AF_INET6, printableIp.c_str(), &dst);
#else
   // !cj! TODO
   assert(0);
   return -1;
#endif   
}


bool 
DnsUtil::isIpV4Address(const Data& ipAddress)
{
   // ok, this is fairly monstrous but it works. 
   unsigned int p1,p2,p3,p4;
   int count=0;
   int result = sscanf( ipAddress.c_str(), 
                        "%u.%u.%u.%u%n",
                        &p1, &p2, &p3, &p4, &count );

   if ( (result == 4) && (p1 <= 255) && (p2 <= 255) && (p3 <= 255) && (p4 <= 255) && (count == int(ipAddress.size())) )
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool 
DnsUtil::isIpV6Address(const Data& ipAddress)
{
   if (ipAddress.empty())
   {
      return false;
   }

   // first character must be a hex digit
   if (!isxdigit(*ipAddress.data()))
   {
      return false;
   }

   switch (ipAddress.size())
   {
      case 1:
         return false;
      case 2:
         return (*(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
      case 3:
         return (*(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
      case 4:
         return (*(ipAddress.data()+4) == ':' ||
                 *(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
      default:
         return (*(ipAddress.data()+5) == ':' ||
                 *(ipAddress.data()+4) == ':' ||
                 *(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':');
   }
}

void
DnsUtil::canonicalizeIpV6Address(Data& ipV6Address)
{
   // !dlb! implement me
}

bool 
DnsUtil::isIpAddress(const Data& ipAddress)
{
   return isIpV4Address(ipAddress) || isIpV6Address(ipAddress);
}


std::list<std::pair<Data,Data> > 
DnsUtil::getInterfaces()
{
   std::list<std::pair<Data,Data> > results;
   
#if !defined(WIN32)
   struct ifconf ifc;
   
   int s = socket( AF_INET, SOCK_DGRAM, 0 );
   const int len = 100 * sizeof(struct ifreq);
   
   char buf[ len ];
   
   ifc.ifc_len = len;
   ifc.ifc_buf = buf;
   
   int e = ioctl(s,SIOCGIFCONF,&ifc);
   char *ptr = buf;
   int tl = ifc.ifc_len;
   int count=0;
  
   int maxRet = 10;
   while ( (tl > 0) && ( count < maxRet) )
   {
      struct ifreq* ifr = (struct ifreq *)ptr;
      
      int si = sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
      tl -= si;
      ptr += si;
      //char* name = ifr->ifr_ifrn.ifrn_name;
      char* name = ifr->ifr_name;
 
      struct ifreq ifr2;
      ifr2 = *ifr;
      
      e = ioctl(s,SIOCGIFADDR,&ifr2);

      struct sockaddr a = ifr2.ifr_addr;
      struct sockaddr_in* addr = (struct sockaddr_in*) &a;
      
      char str[256];
      ::inet_ntop(AF_INET, (u_int32_t*)(&addr->sin_addr.s_addr), str, sizeof(str));
      DebugLog (<< "Considering: " << name << " -> " << str);
      
      results.push_back(std::make_pair(name, str));
   }
#else // !WIN32
   assert(0);
#endif

   return results;
}
