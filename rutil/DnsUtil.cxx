#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(WIN32)
#if defined(__SUNPRO_CC) || defined (__sun__)
#define BSD_COMP /* !rk! needed to enable SIOCGIFCONF */
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <netdb.h>
#endif

#include <stdio.h>
#include <memory>

#include "rutil/compat.hxx"
#include "rutil/Socket.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinCompat.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

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
   resip_assert (ret != ERANGE);
#elif defined(__QNX__) || defined(__SUNPRO_CC)
   struct hostent hostbuf; 
   char buffer[8192];
   result = gethostbyname_r( host.c_str(), &hostbuf, buffer, sizeof(buffer), &herrno );
#elif defined(__APPLE__)
   // gethostbyname in os/x is thread-safe...
   // http://developer.apple.com/technotes/tn2002/pdf/tn2053.pdf
   result = gethostbyname( host.c_str() );
   ret = (result == 0);
#else
   resip_assert(0);
   return names;
#endif
   
   if (ret != 0 || result == 0)
   {
      Data msg;
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
      msg += host;

      DebugLog (<< "DNS lookup of " << host << " resulted in " << msg);
      throw Exception("no dns resolution:" + msg, __FILE__, __LINE__);
   }
   else
   {
      resip_assert(result);
      resip_assert(result->h_length == 4);
      char str[256];
      for (char** pptr = result->h_addr_list; *pptr != 0; pptr++)
      {
         inet_ntop(result->h_addrtype, (u_int32_t*)(*pptr), str, sizeof(str));
         names.push_back(str);
      }

      StackLog (<< "DNS lookup of " << host << ": canonical name: " << result->h_name 
                << " "
                << Inserter(names));
      
      return names;
   }
}

// The following statics ensure we can initialize the static storage of
// localHostName at runtime, instead of at global static initialization time.
// Under windows, when building a DLL you cannot call initNetwork and 
// other socket API's reliably from DLLMain, so we need to delay this call.
// We use the gate bool to ensure we don't need to do a mutex check everytime
// and we use a Mutex to ensure that multiple threads can invokve getLocalHostName
// for the first time, at the same time.
static Mutex getLocalHostNameInitializerMutex;
static bool  getLocalHostNameInitializerGate = false;
static Data localHostName;
const Data&
DnsUtil::getLocalHostName()
{
   if(!getLocalHostNameInitializerGate)
   {
      Lock lock(getLocalHostNameInitializerMutex);
      char buffer[MAXHOSTNAMELEN + 1];
      initNetwork();
      // can't assume the name is NUL terminated when truncation occurs,
      // so insert trailing NUL here
      buffer[0] = buffer[MAXHOSTNAMELEN] = '\0';
      if (gethostname(buffer,sizeof(buffer)-1) == -1)
      {
         int err = getErrno();
         switch (err)
         {
// !RjS! This makes no sense for non-windows. The
//       current hack (see the #define in .hxx) needs
//       to be reworked.
            case WSANOTINITIALISED:
               CritLog( << "could not find local hostname because network not initialized:" << strerror(err) );
               break;
            default:
               CritLog( << "could not find local hostname:" << strerror(err) );
               break;
         }
         throw Exception("could not find local hostname",__FILE__,__LINE__);
      }

      struct addrinfo* result=0;
      struct addrinfo hints;
      memset(&hints, 0, sizeof(hints));
      hints.ai_flags |= AI_CANONNAME;
      hints.ai_family |= AF_UNSPEC;
      int res = getaddrinfo(buffer, 0, &hints, &result);
      if (!res) 
      {
         // !jf! this should really use the Data class 
         if (strchr(result->ai_canonname, '.') != 0) 
         {
            strncpy(buffer, result->ai_canonname, sizeof(buffer));
         }
         else 
         {
            InfoLog( << "local hostname does not contain a domain part " << buffer);
         }
         freeaddrinfo(result);
      }
      else
      {
         InfoLog (<< "Couldn't determine local hostname. Error was: " << gai_strerror(res) << ". Returning empty string");
      }
   
      localHostName = buffer;
      getLocalHostNameInitializerGate = true;
   }
   return localHostName;
}

Data
DnsUtil::getLocalDomainName()
{
   Data lhn(getLocalHostName());
   size_t dpos = lhn.find(".");
   if (dpos != Data::npos)
   {
      return lhn.substr(dpos+1);
   }
   else
   {
#if defined( __APPLE__ ) || defined( WIN32 ) || defined(__SUNPRO_CC) || defined(__sun__) || defined( __ANDROID__ )
      throw Exception("Could not find domainname in local hostname",__FILE__,__LINE__);
#else
      DebugLog( << "No domain portion in hostname <" << lhn << ">, so using getdomainname");
      char buffer[MAXHOSTNAMELEN + 1];
      // can't assume the name is NUL terminated when truncation occurs,
      // so insert trailing NUL here
      buffer[0] = buffer[MAXHOSTNAMELEN] = '\0';
      if (int e = getdomainname(buffer,sizeof(buffer)-1) == -1)
      {
         if ( e != 0 )
         {
            int err = getErrno();
            CritLog(<< "Couldn't find domainname: " << strerror(err));
            throw Exception(strerror(err), __FILE__,__LINE__);
         }
      }
      DebugLog (<< "Found local domain name " << buffer);
     
      return Data(buffer);
#endif
   }
}

Data
DnsUtil::getLocalIpAddress(const Data& myInterface)
{
   Data result;
   std::list<std::pair<Data,Data> > ifs = DnsUtil::getInterfaces(myInterface);

   if (ifs.empty())
   {
      WarningLog( << "No interfaces matching "  << myInterface << " were found" );
      throw Exception("No interfaces matching", __FILE__, __LINE__);
   }
   else
   {
      InfoLog (<< "Local IP address for " << myInterface << " is " << ifs.begin()->second);
      return ifs.begin()->second;
   }
}

Data
DnsUtil::inet_ntop(const struct in_addr& addr)
{
   char str[256];
   inet_ntop(AF_INET, &addr, str, sizeof(str));
   return Data(str);
}

#ifdef USE_IPV6
Data
DnsUtil::inet_ntop(const struct in6_addr& addr)
{
   char str[256];
   inet_ntop(AF_INET6, &addr, str, sizeof(str));
   return Data(str);
}
#endif

Data
DnsUtil::inet_ntop(const struct sockaddr& addr)
{
#ifdef USE_IPV6
   if (addr.sa_family == AF_INET6)
   {
      const struct sockaddr_in6* addr6 = reinterpret_cast<const sockaddr_in6*>(&addr);
      return DnsUtil::inet_ntop(addr6->sin6_addr);
   }
   else
#endif
   {
      const struct sockaddr_in* addr4 = reinterpret_cast<const sockaddr_in*>(&addr);
      return DnsUtil::inet_ntop(addr4->sin_addr);
   }
}

int
DnsUtil::inet_pton(const Data& printableIp, struct in_addr& dst)
{
   return DnsUtil::inet_pton(AF_INET, printableIp.c_str(), &dst);
}

#ifdef USE_IPV6
int
DnsUtil::inet_pton(const Data& printableIp, struct in6_addr& dst)
{
   return DnsUtil::inet_pton(AF_INET6, printableIp.c_str(), &dst);
}
#endif


bool
DnsUtil::isIpV4Address(const Data& ipAddress)
{
   // ok, this is fairly monstrous but it works. It is also more than 10 times 
   // faster than the commented-out code below, in the worst case scenario.

   const char* first = ipAddress.data();
   const char* end = first + ipAddress.size();
   int octets = 0;
   while(octets++ < 4)
   {
      const char* last=first;

      // .bwc. I have tried using std::bitset instead of the 0 <= *last <= 9
      // check, but it is slower.
      while(last != end && *last >= '0' && *last <= '9' && last - first <= 3)
      {
         // Skip at most 3 decimals, without going past the end of the buffer.
         ++last;
      }

      // last should now point to either a '.', or the end of the buffer.

      switch(last-first) // number of decimals in this octet
      {
         case 2:
            if(*first == '0')
            {
               // Two-decimal octet can't begin with 0...
               // ?bwc? Maybe let this slide?
               return false;
            }
         case 1:
            // ... but a one-decimal octet can begin with a 0.
            break; // x. or xx.
         case 3:
            // xxx. (could be too large)
            // .bwc. I have tried implementing this with a reinterpret_cast 
            // and a UInt32 comparison (accounting for endianness), and memcmp, 
            // but both appear to be slower, even when using 
            // "255.255.255.255" (which maximizes the number of comparisons).
            if(*first != '1')
            {
               if(*first == '2')
               {
                  // Might have overflow if first digit is 2
                  if(*(first+1)>'5' || (*(first+1)=='5' && *(first+2)>'5'))
                  {
                     return false;
                  }
               }
               else
               {
                  // First digit greater than 2 means overflow, 0 not allowed.
                  return false;
               }
            }
            break;
         default:
            return false;
      }

      if(octets < 4)
      {
         if(last != end && *last == '.')
         {
            // Skip over the '.'
            ++last;
         }
         else
         {
            return false;
         }
      }
      first = last; // is now pointing at either the first digit in the next
                     // octet, or the end of the buffer.
   }

   return first==end;
//   unsigned int p1,p2,p3,p4;
//   int count=0;
//   int result = sscanf( ipAddress.c_str(),
//                        "%u.%u.%u.%u%n",
//                        &p1, &p2, &p3, &p4, &count );
//
//   if ( (result == 4) && (p1 <= 255) && (p2 <= 255) && (p3 <= 255) && (p4 <= 255) && (count == int(ipAddress.size())) )
//   {
//      return true;
//   }
//   else
//   {
//      return false;
//   }
}

// RFC 1884
bool
DnsUtil::isIpV6Address(const Data& ipAddress)
{
   if (ipAddress.empty())
   {
      return false;
   }

   // first character must be a hex digit or colon
   if (!isxdigit(*ipAddress.data()) &&
       *ipAddress.data() != ':')
   {
      return false;
   }

   switch (ipAddress.size())
   {
      case 1:
         return false;
      case 2:
         return (*(ipAddress.data()+1) == ':' ||
                 *(ipAddress.data()+0) == ':');
      case 3:
         return (*(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':' ||
                 *(ipAddress.data()+0) == ':');
      case 4:
         return (*(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':' ||
                 *(ipAddress.data()+0) == ':');
      default:

         return (*(ipAddress.data()+4) == ':' ||
                 *(ipAddress.data()+3) == ':' ||
                 *(ipAddress.data()+2) == ':' ||
                 *(ipAddress.data()+1) == ':' ||
                 *(ipAddress.data()+0) == ':');
   }
}

Data
DnsUtil::canonicalizeIpV6Address(const Data& ipV6Address)
{
#ifdef USE_IPV6
   struct in6_addr dst;
   int res = DnsUtil::inet_pton(ipV6Address, dst);
   if (res <= 0)
   {
      // .bwc. Until we supply an isIpV6Address that works, this is not an
      // error on anyone's part but our own. Don't log as a warning/error.
      InfoLog(<< ipV6Address << " is not a well formed IPV6 address");
      // .bwc. We should not assert in this function, because 
      // DnsUtil::isIpV6Address does not do a full validity check. If we have no
      // way of determining whether a V6 addr is valid before making this call,
      // this call _needs_ to be safe.
      // assert(0);
      return Data::Empty;
   }
   return DnsUtil::inet_ntop(dst);
#else
   // assert(0);

   return ipV6Address;
#endif
}

bool
DnsUtil::isIpAddress(const Data& ipAddress)
{
   return isIpV4Address(ipAddress) || isIpV6Address(ipAddress);
}


std::list<std::pair<Data,Data> >
DnsUtil::getInterfaces(const Data& matching)
{
   std::list<std::pair<Data,Data> > results;

#if !defined(WIN32)

   struct ifconf ifc;

   int s = socket( AF_INET, SOCK_DGRAM, 0 );
   resip_assert( s != INVALID_SOCKET );	// can run out of file descs
   const int len = 100 * sizeof(struct ifreq);
   int maxRet = 40;

   char buf[ len ];
   ifc.ifc_len = len;
   ifc.ifc_buf = buf;

   int e = ioctl(s,SIOCGIFCONF,&ifc);
   char *ptr = buf;
   int tl = ifc.ifc_len;
   int count=0;
  
   while ( (tl > 0) && ( count < maxRet) )
   {
      struct ifreq* ifr = (struct ifreq *)ptr;

      count++;

#if defined(__NetBSD__) || defined(__APPLE__)
      int si = sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len;
#else
      int si = sizeof(ifr->ifr_name) + sizeof(ifr->ifr_ifru);
#endif
      
      tl -= si;
      ptr += si;

      char* name = ifr->ifr_name;

      struct ifreq ifr2;
      ifr2 = *ifr;

      e = ioctl(s,SIOCGIFADDR,&ifr2);
      if ( e == -1 )
      {
         // no valid address for this interface, skip it    
         DebugLog (<< "Ignoring interface  " << name << " as there is no valid address" );
         continue;
      }
      struct sockaddr a = ifr2.ifr_addr;
      Data ip = DnsUtil::inet_ntop(a);
      
      e = ioctl(s,SIOCGIFFLAGS,&ifr2);
      if ( e == -1 )
      {
         // no valid flags for this interface, skip it 
         DebugLog (<< "Ignoring interface  " << name << " as there is no valid flags" );
         continue;
      }
      short flags = ifr2.ifr_flags;

#if 0
      // if this does not work on your OS, it is not used yet, 
      // comment it out and put a note of what OS it does not work for 
      struct ifmediareq media; 
      e = ioctl(s,SIOCGIFMEDIA,&media);
      int status = media.ifm_status;
      int active = media.ifm_active;
      DebugLog (<< "Media status=" << hex << status 
                << " active=" << hex << active << dec );  
#endif

#if 0
      // if this does not work on your OS, it is not used yet, 
      // comment it out and put a note of what OS it does not work for 
      e = ioctl(s,SIOCGIFPHYS,&ifr2);
      int phys= ifr2.ifr_phys;
      DebugLog (<< "Phys=" << hex << phys << dec );  
#endif

      DebugLog (<< "Considering: " << name << " -> " << ip
                << " flags=0x" << hex << flags << dec );
   
      if (  (flags & IFF_UP) == 0 ) 
      {  
         DebugLog (<< "  ignore because: interface is not up");
         continue;
      }

      if (  (flags & IFF_LOOPBACK) != 0 ) 
      {
         DebugLog (<< "  ignore because: interface is loopback");
         continue;
      }
      
      if (  (flags & IFF_RUNNING) == 0 ) 
      {
         DebugLog (<< "  ignore because: interface is not running");
         continue;
      }
      
      if ( ( (name[0]<'A') || (name[0]>'z') )
#if defined(__MACH__) && defined(__GNU__)   // for GNU HURD
            && (name[0] != '/')
#endif
         ) // should never happen
      {  
         DebugLog (<< "  ignore because: name looks bogus");
         resip_assert(0);
         continue;
      }

      if (matching == Data::Empty || matching == name)
      {
         DebugLog (<< "  using this");
         results.push_back(std::make_pair(Data(name), ip));
      }
   }

   close(s);
#else 
#if defined(WIN32)
   try
   {
      return WinCompat::getInterfaces(matching);
   }
   catch(WinCompat::Exception& e)
   {
      DebugLog (<< "  WinCompat::getInterfaces throws " << e.getMessage());
      return results;
   }
#else
   resip_assert(0);
#endif
#endif

   return results;
}

#ifdef __APPLE__
const Data DnsUtil::UInt8ToStr[]={
  "0.",  "1.",  "2.",  "3.",  "4.",  "5.",  "6.",  "7.",
  "8.",  "9.", "10.", "11.", "12.", "13.", "14.", "15.",
 "16.", "17.", "18.", "19.", "20.", "21.", "22.", "23.",
 "24.", "25.", "26.", "27.", "28.", "29.", "30.", "31.",
 "32.", "33.", "34.", "35.", "36.", "37.", "38.", "39.",
 "40.", "41.", "42.", "43.", "44.", "45.", "46.", "47.",
 "48.", "49.", "50.", "51.", "52.", "53.", "54.", "55.",
 "56.", "57.", "58.", "59.", "60.", "61.", "62.", "63.",
 "64.", "65.", "66.", "67.", "68.", "69.", "70.", "71.",
 "72.", "73.", "74.", "75.", "76.", "77.", "78.", "79.",
 "80.", "81.", "82.", "83.", "84.", "85.", "86.", "87.",
 "88.", "89.", "90.", "91.", "92.", "93.", "94.", "95.",
 "96.", "97.", "98.", "99.","100.","101.","102.","103.",
"104.","105.","106.","107.","108.","109.","110.","111.",
"112.","113.","114.","115.","116.","117.","118.","119.",
"120.","121.","122.","123.","124.","125.","126.","127.",
"128.","129.","130.","131.","132.","133.","134.","135.",
"136.","137.","138.","139.","140.","141.","142.","143.",
"144.","145.","146.","147.","148.","149.","150.","151.",
"152.","153.","154.","155.","156.","157.","158.","159.",
"160.","161.","162.","163.","164.","165.","166.","167.",
"168.","169.","170.","171.","172.","173.","174.","175.",
"176.","177.","178.","179.","180.","181.","182.","183.",
"184.","185.","186.","187.","188.","189.","190.","191.",
"192.","193.","194.","195.","196.","197.","198.","199.",
"200.","201.","202.","203.","204.","205.","206.","207.",
"208.","209.","210.","211.","212.","213.","214.","215.",
"216.","217.","218.","219.","220.","221.","222.","223.",
"224.","225.","226.","227.","228.","229.","230.","231.",
"232.","233.","234.","235.","236.","237.","238.","239.",
"240.","241.","242.","243.","244.","245.","246.","247.",
"248.","249.","250.","251.","252.","253.","254.","255."
};
#endif // __APPLE__

#if !(defined(WIN32) || defined(__CYGWIN__))
const char *DnsUtil::inet_ntop(int af, const void* src, char* dst,
                               size_t size)
{

#ifdef __APPLE__
   if(af==AF_INET)
   {
      // .bwc. inet_ntop4 seems to be implemented with sprintf on OS X.
      // This code is about 5-6 times faster. Linux has a well-optimized 
      // inet_ntop, however.
      const UInt8* bytes=(const UInt8*)src;
      Data dest(Data::Borrow, dst, sizeof("xxx.xxx.xxx.xxx."));
      dest.clear();
      dest.append(UInt8ToStr[bytes[0]].data(), UInt8ToStr[bytes[0]].size());
      dest.append(UInt8ToStr[bytes[1]].data(), UInt8ToStr[bytes[1]].size());
      dest.append(UInt8ToStr[bytes[2]].data(), UInt8ToStr[bytes[2]].size());
      dest.append(UInt8ToStr[bytes[3]].data(), UInt8ToStr[bytes[3]].size()-1);
      return dst;
   }
   else
#endif // __APPLE__
   {
      return ::inet_ntop(af, src, dst, size);
   }
}

int DnsUtil::inet_pton(int af, const char* src, void* dst)
{
   return ::inet_pton(af, src, dst);
}
#else
#define __restrict

#define  NS_INT16SZ   2
#define  NS_INADDRSZ  4
#define  NS_IN6ADDRSZ 16

const char* inet_ntop4(const u_char *src, char *dst, size_t size);
#ifdef USE_IPV6
const char * inet_ntop6(const u_char *src, char *dst, size_t size);
#endif
//adapted from freebsd inet_ntop.c(1.12) and inet_pton.c(1.5) for windows(non-compliant snprinf workaround)
/* const char *
 * inet_ntop4(src, dst, size)
 *	format an IPv4 address, more or less like inet_ntoa()
 * return:
 *	`dst' (as a const)
 * notes:
 *	(1) uses no statics
 *	(2) takes a u_char* not an in_addr as input
 * author:
 *	Paul Vixie, 1996.
 */
const char *
DnsUtil::inet_ntop(int af, const void * __restrict src, char * __restrict dst,
                   size_t size)
{
   switch (af) 
   {
      case AF_INET:
         return (inet_ntop4((const u_char *)src, dst, size));
#ifdef USE_IPV6
      case AF_INET6:
         return (inet_ntop6((const u_char *)src, dst, size));
#endif
      default:
         errno = EAFNOSUPPORT;
         return (NULL);
   }
   /* NOTREACHED */
}


static const char fmt[] = "%u.%u.%u.%u";
const char*
inet_ntop4(const u_char *src, char *dst, size_t size)
{
#ifdef WIN32
   if ( _snprintf(dst, size, fmt, src[0], src[1], src[2], src[3]) < 0)
#else
   if ( snprintf(dst, size, fmt, src[0], src[1], src[2], src[3]) < 0)
#endif
   {
      errno = ENOSPC;
      dst[size-1] = 0;
      return NULL;
   }
   return (dst);
}


#ifdef USE_IPV6
/* const char *
 * inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * author:
 *	Paul Vixie, 1996.
 */

const char *
inet_ntop6(const u_char *src, char *dst, size_t size)
{
   /*
    * Note that int32_t and int16_t need only be "at least" large enough
    * to contain a value of the specified size.  On some systems, like
    * Crays, there is no such thing as an integer variable with 16 bits.
    * Keep this in mind if you think this function should have been coded
    * to use pointer overlays.  All the world's not a VAX.
    */
   char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
   struct { int base, len; } best, cur;
   u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
   int i;

   /*
    * Preprocess:
    *	Copy the input (bytewise) array into a wordwise array.
    *	Find the longest run of 0x00's in src[] for :: shorthanding.
    */
   memset(words, '\0', sizeof words);
   for (i = 0; i < NS_IN6ADDRSZ; i++)
      words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
   best.base = -1;
   cur.base = -1;
   for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
      if (words[i] == 0) {
         if (cur.base == -1)
            cur.base = i, cur.len = 1;
         else
            cur.len++;
      } else {
         if (cur.base != -1) {
            if (best.base == -1 || cur.len > best.len)
               best = cur;
            cur.base = -1;
         }
      }
   }
   if (cur.base != -1) {
      if (best.base == -1 || cur.len > best.len)
         best = cur;
   }
   if (best.base != -1 && best.len < 2)
      best.base = -1;

   /*
    * Format the result.
    */
   tp = tmp;
   for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
      /* Are we inside the best run of 0x00's? */
      if (best.base != -1 && i >= best.base &&
          i < (best.base + best.len)) {
         if (i == best.base)
            *tp++ = ':';
         continue;
      }
      /* Are we following an initial run of 0x00s or any real hex? */
      if (i != 0)
         *tp++ = ':';
      /* Is this address an encapsulated IPv4? */
      if (i == 6 && best.base == 0 &&
          (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
         if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
            return (NULL);
         tp += strlen(tp);
         break;
      }
      tp += sprintf(tp, "%x", words[i]);
   }
   /* Was it a trailing run of 0x00's? */
   if (best.base != -1 && (best.base + best.len) ==
       (NS_IN6ADDRSZ / NS_INT16SZ))
      *tp++ = ':';
   *tp++ = '\0';

   /*
    * Check for overflow, copy, and we're done.
    */
   if ((size_t)(tp - tmp) > size) {
      errno = ENOSPC;
      return (NULL);
   }
   strcpy(dst, tmp);
   return (dst);
}

static int	inet_pton6(const char *src, u_char *dst);
#endif //USE_IPV6

static int	inet_pton4(const char *src, u_char *dst);

/* int
 * inet_pton(af, src, dst)
 *	convert from presentation format (which usually means ASCII printable)
 *	to network format (which is usually some kind of binary format).
 * return:
 *	1 if the address was valid for the specified address family
 *	0 if the address wasn't valid (`dst' is untouched in this case)
 *	-1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *	Paul Vixie, 1996.
 */
int
DnsUtil::inet_pton(int af, const char* src, void* dst)
{
   switch (af) {
      case AF_INET:
         return (inet_pton4(src, (u_char*) dst));
#ifdef USE_IPV6
      case AF_INET6:
         return (inet_pton6(src, (u_char*) dst));
#endif
      default:
         errno = EAFNOSUPPORT;
         return (-1);
   }
   /* NOTREACHED */
}

/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
static const char digits[] = "0123456789";
static int
inet_pton4(const char *src, u_char *dst)
{
   int saw_digit, octets, ch;
   u_char tmp[NS_INADDRSZ], *tp;

   saw_digit = 0;
   octets = 0;
   *(tp = tmp) = 0;
   while ((ch = *src++) != '\0') {
      const char *pch;

      if ((pch = strchr(digits, ch)) != NULL) {
         u_int newVal = u_int(*tp * 10 + (pch - digits));

         if (newVal > 255)
            return (0);
         *tp = newVal;
         if (! saw_digit) {
            if (++octets > 4)
               return (0);
            saw_digit = 1;
         }
      } else if (ch == '.' && saw_digit) {
         if (octets == 4)
            return (0);
         *++tp = 0;
         saw_digit = 0;
      } else
         return (0);
   }
   if (octets < 4)
      return (0);

   memcpy(dst, tmp, NS_INADDRSZ);
   return (1);
}

#ifdef USE_IPV6

/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
static const char xdigits_l[] = "0123456789abcdef",
                  xdigits_u[] = "0123456789ABCDEF";
static int
inet_pton6(const char *src, u_char *dst)
{
   u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
   const char *xdigits, *curtok;
   int ch, saw_xdigit;
   u_int val;

   memset((tp = tmp), '\0', NS_IN6ADDRSZ);
   endp = tp + NS_IN6ADDRSZ;
   colonp = NULL;
   /* Leading :: requires some special handling. */
   if (*src == ':')
      if (*++src != ':')
         return (0);
   curtok = src;
   saw_xdigit = 0;
   val = 0;
   while ((ch = *src++) != '\0') {
      const char *pch;

      if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
         pch = strchr((xdigits = xdigits_u), ch);
      if (pch != NULL) {
         val <<= 4;
         val |= (pch - xdigits);
         if (val > 0xffff)
            return (0);
         saw_xdigit = 1;
         continue;
      }
      if (ch == ':') {
         curtok = src;
         if (!saw_xdigit) {
            if (colonp)
               return (0);
            colonp = tp;
            continue;
         }
         if (tp + NS_INT16SZ > endp)
            return (0);
         *tp++ = (u_char) (val >> 8) & 0xff;
         *tp++ = (u_char) val & 0xff;
         saw_xdigit = 0;
         val = 0;
         continue;
      }
      if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
          inet_pton4(curtok, tp) > 0) {
         tp += NS_INADDRSZ;
         saw_xdigit = 0;
         break;	/* '\0' was seen by inet_pton4(). */
      }
      return (0);
   }
   if (saw_xdigit) {
      if (tp + NS_INT16SZ > endp)
         return (0);
      *tp++ = (u_char) (val >> 8) & 0xff;
      *tp++ = (u_char) val & 0xff;
   }
   if (colonp != NULL) {
      /*
       * Since some memmove()'s erroneously fail to handle
       * overlapping regions, we'll do the shift by hand.
       */
      const int n = int(tp - colonp);
      int i;

      for (i = 1; i <= n; i++) {
         endp[- i] = colonp[n - i];
         colonp[n - i] = 0;
      }
      tp = endp;
   }
   if (tp != endp)
      return (0);
   memcpy(dst, tmp, NS_IN6ADDRSZ);
   return (1);
}


#endif
#endif

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

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

