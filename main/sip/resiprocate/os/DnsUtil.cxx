
#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#endif
#include <stdio.h>

#include "DnsUtil.hxx"
#include "Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::UTIL

using namespace resip;
using namespace std;

list<Data> 
DnsUtil::lookupARecords(const Data& host)
{
   list<Data> names;

   if (DnsUtil::isIpV4Address(host))
   {
      names.push_back(host);
      return names;
   }

   struct hostent* result=0;
   int ret=0;
   int herrno=0;

#if defined(__linux__)
   struct hostent hostbuf; 
   char buffer[8192];
   ret = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &result, &herrno);
   assert (ret != ERANGE);
#elif defined(__QNX__) || defined(__SUNPRO_CC)
   struct hostent hostbuf; 
   char buffer[8192];
   result = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno );
#elif defined( __MACH__ ) || defined (__FreeBSD__) || defined( WIN32 )
   result = gethostbyname( host.c_str() );
   herrno = h_errno;
#else
#error "need to define some version of gethostbyname for your arch"
#endif
   
   if (ret != 0 || result == 0)
   {
      string msg;
      switch (herrno)
      {
         case HOST_NOT_FOUND:
            msg = "host not found: ";
            break;
         case NO_DATA:
            msg = "no data found for: ";
            break;
         case NO_RECOVERY:
            msg = "no recovery lookup up: ";
            break;
         case TRY_AGAIN:
            msg = "try again: ";
            break;
      }
      msg += host.c_str();
      throw Exception("no dns resolution", __FILE__, __LINE__);
   }
   else
   {
      assert(result);
      assert(result->h_length == 4);
      DebugLog (<< "DNS lookup of " << host << ": canonical name: " << result->h_name);
      char str[256];
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
#if WIN32
 // !cj! TODO 
		  assert(0);
#else
		  inet_ntop(result->h_addrtype, (u_int32_t*)(*pptr), str, sizeof(str));
#endif        
		  names.push_back(str);
      }
      return names;
   }
}
      
std::list<DnsUtil::Srv> 
DnsUtil::lookupSRVRecords(const Data& host)
{
   list<DnsUtil::Srv> records;
   assert(0);
   return records;
}

Data 
DnsUtil::getHostByAddr(const Data& ipAddress)
{
   if (!DnsUtil::isIpV4Address(ipAddress))
   {
      return ipAddress;
   }
       
   struct in_addr addrStruct;
   int ret=0;
#ifdef WIN32
	assert(0);
#else
    ret = inet_aton(ipAddress.c_str(), &addrStruct);
#endif

   if (ret == 0)
   {
      throw Exception("Not a valid ip address.", __FILE__, __LINE__);
   }

   struct hostent h;
   struct hostent* hp;
   int localErrno;
   hp = &h;
   
#if defined(__GLIBC__)
   char buf[8192];
   hostent* pres;
   ret = gethostbyaddr_r (&addrStruct,
                          sizeof(addrStruct),
                          AF_INET,
                          &h,
                          buf,
                          8192,
                          &pres,
                          &localErrno);
#elif defined(__linux__) || defined(__QNX__)  || defined(__SUNPRO_CC)
   char buf[8192];
   ret = gethostbyaddr_r ((char *)(&addrStruct),
                          sizeof(addrStruct),
                          AF_INET,
                          &h,
                          buf,
                          8192,
                          &localErrno);
#elif defined(__MACH__) || defined(__FreeBSD__) || defined( WIN32 )
   hp = gethostbyaddr( (char *)(&addrStruct),
                       sizeof(addrStruct),
                       AF_INET);
   localErrno = h_errno;
#else
#error no implementation for critical function
#endif
   if (ret != 0)
   {
      throw Exception("getHostByAddr failed to lookup PTR", __FILE__, __LINE__);
   }
   return Data(hp->h_name);
}


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
DnsUtil::getLocalIpAddress() 
{
   return lookupARecords(getLocalHostName()).front();
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


Data
DnsUtil::getIpAddress(const struct in_addr& addr)
{
   char str[256];
#ifdef WIN32
// !cj! TODO 
   assert(0);
#else
   inet_ntop(AF_INET, (u_int32_t*)(&addr), str, sizeof(str));
#endif
   return Data(str);
}
