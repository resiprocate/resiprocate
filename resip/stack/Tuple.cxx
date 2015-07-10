#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Tuple.hxx"
#include "rutil/compat.hxx"

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include "rutil/ResipAssert.h"

#if !defined (WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#if defined(__APPLE__) && !defined(s6_addr16)
#define s6_addr16 __u6_addr.__u6_addr16
#endif
#endif

#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/Logger.hxx"
#ifdef USE_NETNS
#   include "rutil/NetNs.hxx"
#endif

using std::auto_ptr;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DNS

Tuple::Tuple() : 
   mFlowKey(0),
   mTransportKey(0),
   onlyUseExistingConnection(false),
   mTransportType(UNKNOWN_TRANSPORT)
{
   sockaddr_in* addr4 = (sockaddr_in*)&mSockaddr;
   memset(addr4, 0, sizeof(sockaddr_in));
   mSockaddr.sa_family = AF_INET;
}

Tuple::Tuple(const GenericIPAddress& genericAddress, TransportType type, 
             const Data& targetDomain) : 
   mFlowKey(0),
   mTransportKey(0),
   onlyUseExistingConnection(false),
   mTransportType(type),
   mTargetDomain(targetDomain)
{
   setSockaddr(genericAddress);
}


Tuple::Tuple(const Data& printableAddr, 
             int port,
             IpVersion ipVer,
             TransportType type,
             const Data& targetDomain,
             const Data& netNs) :
   mFlowKey(0),
   mTransportKey(0),
   onlyUseExistingConnection(false),
   mTransportType(type),
   mTargetDomain(targetDomain),
   mNetNs(netNs)
{
   if (ipVer == V4)
   {
      memset(&m_anonv4, 0, sizeof(m_anonv4));
      m_anonv4.sin_family = AF_INET;
      m_anonv4.sin_port = htons(port);

      if (printableAddr.empty())
      {
         m_anonv4.sin_addr.s_addr = htonl(INADDR_ANY); 
      }
      else
      {
         DnsUtil::inet_pton( printableAddr, m_anonv4.sin_addr);
      }
   }
   else
   {
#ifdef USE_IPV6
      memset(&m_anonv6, 0, sizeof(m_anonv6));
      m_anonv6.sin6_family = AF_INET6;
      m_anonv6.sin6_port = htons(port);
      if (printableAddr.empty())
      {
         m_anonv6.sin6_addr = in6addr_any;
      }
      else
      {
         DnsUtil::inet_pton( printableAddr, m_anonv6.sin6_addr);
      }
#else
      // avoid asserts since Tuples created via printableAddr can be created from items 
      // received from the wire or from configuration settings.  Just create an IPV4 inaddr_any tuple.
      //assert(0);  
      memset(&m_anonv4, 0, sizeof(m_anonv4));
      m_anonv4.sin_family = AF_INET;
      m_anonv4.sin_port = htons(port);
      m_anonv4.sin_addr.s_addr = htonl(INADDR_ANY); 
#endif
   }
}

Tuple::Tuple(const Data& printableAddr, 
             int port,
             TransportType ptype,
             const Data& targetDomain,
             const Data& netNs) : 
   mFlowKey(0),
   mTransportKey(0),
   onlyUseExistingConnection(false),
   mTransportType(ptype),
   mTargetDomain(targetDomain),
   mNetNs(netNs)
{
   if (DnsUtil::isIpV4Address(printableAddr))
   {
      memset(&m_anonv4, 0, sizeof(m_anonv4));
      
      DnsUtil::inet_pton( printableAddr, m_anonv4.sin_addr);
      m_anonv4.sin_family = AF_INET;
      m_anonv4.sin_port = htons(port);
   }
#ifdef USE_IPV6
   else if(DnsUtil::isIpV6Address(printableAddr))
   {
      memset(&m_anonv6, 0, sizeof(m_anonv6));
      DnsUtil::inet_pton( printableAddr, m_anonv6.sin6_addr);
      m_anonv6.sin6_family = AF_INET6;
      m_anonv6.sin6_port = htons(port);
   }
#endif
   else
   {
      // avoid asserts since Tuples created via printableAddr can be created from items 
      // received from the wire or from configuration settings.  Just create an IPV4 inaddr_any tuple.
      // assert(0);  
      memset(&m_anonv4, 0, sizeof(m_anonv4));
      m_anonv4.sin_family = AF_INET;
      m_anonv4.sin_port = htons(port);
      m_anonv4.sin_addr.s_addr = htonl(INADDR_ANY);
   }
}

Tuple::Tuple(const in_addr& ipv4,
             int port,
             TransportType ptype,
             const Data& targetDomain,
             const Data& netNs)
     :mFlowKey(0),
     mTransportKey(0),
     onlyUseExistingConnection(false),
     mTransportType(ptype),
     mTargetDomain(targetDomain),
     mNetNs(netNs)
{
   memset(&m_anonv4, 0, sizeof(sockaddr_in));
   m_anonv4.sin_addr = ipv4;
   m_anonv4.sin_port = htons(port);
   m_anonv4.sin_family = AF_INET;
}

#ifdef USE_IPV6
Tuple::Tuple(const in6_addr& ipv6,
             int port,
             TransportType ptype,
             const Data& targetDomaina,
             const Data& netNs)
     :mFlowKey(0),
     mTransportKey(0),
     onlyUseExistingConnection(false),
     mTransportType(ptype),
     mTargetDomain(targetDomaina),
     mNetNs(netNs)
{
   memset(&m_anonv6, 0, sizeof(sockaddr_in6));
   m_anonv6.sin6_addr = ipv6;
   m_anonv6.sin6_port = htons(port);
   m_anonv6.sin6_family = AF_INET6;
}
#endif

Tuple::Tuple(const struct sockaddr& addr, 
             TransportType ptype,
             const Data& targetDomain) : 
   mFlowKey(0),
   mTransportKey(0),
   onlyUseExistingConnection(false),
   mSockaddr(addr),
   mTransportType(ptype),
   mTargetDomain(targetDomain)
{
   if (addr.sa_family == AF_INET)   
   {
      m_anonv4 = (sockaddr_in&)(addr);
   }
#ifdef USE_IPV6
   else if (addr.sa_family == AF_INET6)
   {
      m_anonv6 = (sockaddr_in6&)(addr);
   }
#endif
   else
   {
      resip_assert(0);
   }
}

void
Tuple::copySockaddrAnyPort(sockaddr *sa)
{
   memcpy(sa, &mSockaddr, length());
   // zero the port number
   if (sa->sa_family == AF_INET)
   {
      ((sockaddr_in*)sa)->sin_port = 0;
   }
#ifdef USE_IPV6
   else if (sa->sa_family == AF_INET6)
   {
      ((sockaddr_in6*)sa)->sin6_port = 0;
   }
#endif
   else
   {
      resip_assert(0);
   }
}

void
Tuple::setSockaddr(const GenericIPAddress& addr)
{
  if (addr.isVersion4())
  {
     m_anonv4 = addr.v4Address;
  }
  else
#ifdef USE_IPV6
  {
     m_anonv6 = addr.v6Address;
  }
#else
  {
     resip_assert(0);
  }
#endif
}

#ifdef USE_NETNS
#   define TOKEN_SIZE 8
#   define TOKEN_IP_ADDRESS_OFFSET 4
#else
#   define TOKEN_SIZE 7
#   define TOKEN_IP_ADDRESS_OFFSET 3
#endif

void
Tuple::writeBinaryToken(const resip::Tuple& tuple, resip::Data& container, const Data& salt)
{
   // .bwc. Maybe should just write the raw sockaddr into a buffer, and tack
   // on the flowid and onlyUseExistingConnection flag. Would require 10 extra
   // bytes for V6, and 14 extra bytes for V4. 
   // V6: sin6_len(1), sin6_flowinfo(4), flowId(4), onlyUseExistingConnection(1)
   // V4: sin_family(2 instead of 1), sin_zero(8), flowId(4), onlyUseExistingConnection(1)
   UInt32 rawToken[TOKEN_SIZE];
   memset(&rawToken, 0, TOKEN_SIZE * 4);

   rawToken[0] = tuple.mFlowKey;

   rawToken[1] = tuple.mTransportKey;

   // 0xXXXX0000
   rawToken[2] += (tuple.getPort() << 16);

   // 0x0000XX00
   rawToken[2] += (tuple.getType() << 8);

   // 0x000000X0
   if(tuple.onlyUseExistingConnection)
   {
      rawToken[2] += 0x00000010;
   }

#ifdef USE_NETNS
   rawToken[3] = NetNs::getNetNsId(tuple.getNetNs());
#endif

#ifdef USE_IPV6
   if(tuple.ipVersion()==V6)
   {
      // 0x0000000X
      rawToken[2] += 0x00000001;
      in6_addr address = reinterpret_cast<const sockaddr_in6&>(tuple.getSockaddr()).sin6_addr;
      resip_assert(sizeof(address)==16);
      memcpy(&rawToken[TOKEN_IP_ADDRESS_OFFSET],&address,16);
   }
   else
#endif
   {
      in_addr address = reinterpret_cast<const sockaddr_in&>(tuple.getSockaddr()).sin_addr;
      resip_assert(sizeof(address)==4);
      memcpy(&rawToken[TOKEN_IP_ADDRESS_OFFSET],&address,4);
   }
   
   container.clear();
   container.reserve(((tuple.ipVersion()==V6) ? TOKEN_SIZE*4 : (TOKEN_SIZE-3)*4) + (salt.empty() ? 0 : 32));
   container.append((char*)&rawToken[0],(tuple.ipVersion()==V6) ? TOKEN_SIZE*4 : (TOKEN_SIZE-3)*4);

   if(!salt.empty())
   {
      // TODO - potentially use SHA1 HMAC if USE_SSL is defined for stronger encryption
      MD5Stream ms;
      ms << container << salt;
      container += ms.getHex();
   }
}


Tuple
Tuple::makeTupleFromBinaryToken(const resip::Data& binaryFlowToken, const Data& salt)
{
   // To check if size is valid, we first need the IP version, so make sure the token is at least
   // the size of an IPv4 token
   if(binaryFlowToken.size()<16)
   {
      // !bwc! Should not assert here, since this sort of thing
      // can come off the wire easily.
      // TODO Throw an exception here?
      DebugLog(<<"binary flow token was too small: " << binaryFlowToken.size());
      return Tuple();
   }

   const UInt32* rawToken=reinterpret_cast<const UInt32*>(binaryFlowToken.data());

   FlowKey mFlowKey=rawToken[0];
   TransportKey transportKey=rawToken[1];

   IpVersion version = (rawToken[2] & 0x00000001 ? V6 : V4);

   bool isRealFlow = (rawToken[2] & 0x00000010 ? true : false);

   UInt8 temp = (TransportType)((rawToken[2] & 0x00000F00) >> 8);
   if(temp >= MAX_TRANSPORT)
   {
      DebugLog(<<"Garbage transport type in flow token: " << temp );
      return Tuple();
   }
   TransportType type = (TransportType)temp;

   UInt16 port= (rawToken[2] >> 16);

   // Now that we have the version we can do a more accurate check on the size
   if(!((version==V4 && salt.empty() && binaryFlowToken.size()==(TOKEN_SIZE-3)*4) ||
        (version==V4 && !salt.empty() && binaryFlowToken.size()==(TOKEN_SIZE-3)*4 + 32) ||
        (version==V6 && salt.empty() && binaryFlowToken.size()==TOKEN_SIZE*4) ||
        (version==V6 && !salt.empty() && binaryFlowToken.size()==TOKEN_SIZE*4 + 32)))
   {
      DebugLog(<<"Binary flow token is the wrong size for its IP version.");
      return Tuple();
   }

   // If salt is specified, validate HMAC
   if(!salt.empty())
   {
      unsigned int tokenSizeLessHMAC = version == V4 ? (TOKEN_SIZE-3)*4 : TOKEN_SIZE*4;
      Data flowTokenLessHMAC(Data::Share, binaryFlowToken.data(), tokenSizeLessHMAC);
      Data flowTokenHMAC(Data::Share, binaryFlowToken.data()+tokenSizeLessHMAC, 32);
      MD5Stream ms;
      ms << flowTokenLessHMAC << salt;
      if(ms.getHex() != flowTokenHMAC)
      {
         DebugLog(<<"Binary flow token has invalid HMAC, not our token");
         return Tuple();
      }
   }

   Data netNs("");
#ifdef USE_NETNS
   int netNsId = rawToken[3];
   try
   {
      netNs = NetNs::getNetNsName(netNsId);
   }
   catch(NetNs::Exception e)
   {
       ErrLog(<< "Tuple binary token contained netns id: " << netNsId << "which does not exist." 
               << e);
   }
#endif

   if(version==V6)
   {
#ifdef USE_IPV6
      in6_addr address;
      resip_assert(sizeof(address)==16);
      memcpy(&address,&rawToken[TOKEN_IP_ADDRESS_OFFSET],16);
      Tuple result(address, port, type, Data::Empty, netNs);
#else
      Tuple result(resip::Data::Empty, port, type, Data::Empty, netNs);
#endif
      result.mFlowKey=(FlowKey)mFlowKey;
      result.mTransportKey = (TransportKey)transportKey;
      result.onlyUseExistingConnection=isRealFlow;
      return result;
   }

   in_addr address;
   resip_assert(sizeof(address)==4);
   memcpy(&address,&rawToken[TOKEN_IP_ADDRESS_OFFSET],4);
   Tuple result(address, port, type, Data::Empty, netNs);
   result.mFlowKey=(FlowKey)mFlowKey;
   result.mTransportKey = (TransportKey)transportKey;
   result.onlyUseExistingConnection=isRealFlow;
   return result;
}

Data 
Tuple::presentationFormat() const
{
#ifdef USE_IPV6
   if (isV4())
   {
      return Tuple::inet_ntop(*this);
   }
   else if (IN6_IS_ADDR_V4MAPPED(&m_anonv6.sin6_addr))
   {
      return DnsUtil::inet_ntop(*(reinterpret_cast<const in_addr*>(
                                   (reinterpret_cast<const unsigned char*>(&m_anonv6.sin6_addr) + 12))));
   }
   else
   {
      return Tuple::inet_ntop(*this);
   }
#else
      return Tuple::inet_ntop(*this);
#endif

}

void
Tuple::setPort(int port)
{
   if (mSockaddr.sa_family == AF_INET) // v4   
   {
      m_anonv4.sin_port = htons(port);
   }
   else
   {
#ifdef USE_IPV6
      m_anonv6.sin6_port = htons(port);
#else
      resip_assert(0);
#endif
   }
}

int 
Tuple::getPort() const
{
   if (mSockaddr.sa_family == AF_INET) // v4   
   {
      return ntohs(m_anonv4.sin_port);
   }
   else
   {
#ifdef USE_IPV6
      return ntohs(m_anonv6.sin6_port);
#else
      resip_assert(0);
#endif
   }
   
   return -1;
}

bool
Tuple::isAnyInterface() const
{
   if (isV4())
   {
      return m_anonv4.sin_addr.s_addr == htonl(INADDR_ANY); 
   }
#if defined (USE_IPV6)
   else
   {
      return memcmp(&m_anonv6.sin6_addr, &in6addr_any, sizeof(in6_addr)) == 0;
   }
#else
   return false;
#endif
}

static Tuple loopbackv4("127.0.0.1",0,UNKNOWN_TRANSPORT);
bool
Tuple::isLoopback() const
{
   if(ipVersion()==V4)
   {
      return isEqualWithMask(loopbackv4,8,true,true);
   }
   else if (ipVersion()==V6)
   {
#ifdef USE_IPV6
#if defined(__linux__) || defined(__APPLE__) || defined(WIN32)
      return IN6_IS_ADDR_LOOPBACK(&(m_anonv6.sin6_addr)) != 0;
#else
      return ((*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[0])) == 0) && 
             (*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[4])) == 0) && 
             (*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[8])) == 0) && 
             (*(const __uint32_t *)(const void *)(&(m_anonv6.sin6_addr.s6_addr[12])) == ntohl(1)));
#endif
#endif
   }
   else
   {
      resip_assert(0);
   }
   
   return false;
}

bool 
Tuple::isV4() const
{
   return mSockaddr.sa_family == AF_INET;
}

IpVersion 
Tuple::ipVersion() const
{
   return mSockaddr.sa_family == AF_INET ? V4 : V6;
}

static Tuple v4privateaddrbase1("10.0.0.0",0,UNKNOWN_TRANSPORT);
static Tuple v4privateaddrbase2("172.16.0.0",0,UNKNOWN_TRANSPORT);
static Tuple v4privateaddrbase3("192.168.0.0",0,UNKNOWN_TRANSPORT);

#ifdef USE_IPV6
static Tuple v6privateaddrbase("fc00::",0,UNKNOWN_TRANSPORT);
#endif

bool 
Tuple::isPrivateAddress() const
{
   if(ipVersion()==V4)
   {
      // RFC 1918
      return isEqualWithMask(v4privateaddrbase1,8,true,true) ||  // 10.0.0.0        -   10.255.255.255  (10/8 prefix)
             isEqualWithMask(v4privateaddrbase2,12,true,true) || // 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
             isEqualWithMask(v4privateaddrbase3,16,true,true) || // 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
             isLoopback();
   }
#ifdef USE_IPV6
   else if (ipVersion()==V6)
   {
      // RFC 4193
      // ?slg? should we look specifically for ipv4 mapped/compatible address and apply V4 rules to them?
      return isEqualWithMask(v6privateaddrbase,7,true,true) ||  // fc00::/7
             isLoopback();
   }
#endif
   else
   {
      resip_assert(0);
   }
   
   return false;
}

socklen_t
Tuple::length() const
{
   if (mSockaddr.sa_family == AF_INET) // v4
   {
      return sizeof(sockaddr_in);
   }
#ifdef USE_IPV6
   else  if (mSockaddr.sa_family == AF_INET6) // v6
   {
      return sizeof(sockaddr_in6);
   }
#endif

   resip_assert(0);
   return 0;
}


bool Tuple::operator==(const Tuple& rhs) const
{
   if (mSockaddr.sa_family == rhs.mSockaddr.sa_family)
   {
      if (mSockaddr.sa_family == AF_INET) // v4
      {
         return (m_anonv4.sin_port == rhs.m_anonv4.sin_port &&
                 mTransportType == rhs.mTransportType &&
                 memcmp(&m_anonv4.sin_addr, &rhs.m_anonv4.sin_addr, sizeof(in_addr)) == 0 &&
                 rhs.mNetNs == mNetNs);
      }
      else // v6
      {
#ifdef USE_IPV6
         return (m_anonv6.sin6_port == rhs.m_anonv6.sin6_port &&
                 mTransportType == rhs.mTransportType &&
                 memcmp(&m_anonv6.sin6_addr, &rhs.m_anonv6.sin6_addr, sizeof(in6_addr)) == 0 &&
                 rhs.mNetNs == mNetNs);
#else
         resip_assert(0);
         return false;
#endif
      }
   }
   else
   {
      return false;
   }

   // !dlb! don't include connection 
}

bool
Tuple::operator<(const Tuple& rhs) const
{
   if (mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (mTransportType > rhs.mTransportType)
   {
      return false;
   }

#ifdef USE_NETNS
   // netns needs to be checked before port and address as the port/address 
   // comparison bails out in equal case.  Ideally netns comparison should
   // be last as its the most expensive comparison.  For now putting it here
   // for minimal code change
   else if(mNetNs < rhs.mNetNs)
   {
       return(true);
   }
   else if(mNetNs > rhs.mNetNs)
   {
       return(false);
   }
#endif

   else if (mSockaddr.sa_family == AF_INET && rhs.mSockaddr.sa_family == AF_INET)
   {
      int c=memcmp(&m_anonv4.sin_addr,
                   &rhs.m_anonv4.sin_addr,
                   sizeof(in_addr));

      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (m_anonv4.sin_port < rhs.m_anonv4.sin_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
#ifdef USE_IPV6
   else if (mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      int c = memcmp(&m_anonv6.sin6_addr,
                     &rhs.m_anonv6.sin6_addr,
                     sizeof(in6_addr));
      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
      else if (m_anonv6.sin6_port < rhs.m_anonv6.sin6_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else if (mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif

   else
   {
      //assert(0);
      return false;
   }
}

EncodeStream&
resip::operator<<(EncodeStream& ostrm, const Tuple& tuple)
{
   ostrm << "[ " ;
   
#ifdef USE_IPV6
   if (tuple.mSockaddr.sa_family == AF_INET6)
   {
      ostrm << "V6 " << DnsUtil::inet_ntop(tuple.m_anonv6.sin6_addr) << " port=" << tuple.getPort();
   }
   else
#endif
   if (tuple.mSockaddr.sa_family == AF_INET)
   {
      ostrm << "V4 " << Tuple::inet_ntop(tuple) << ":" << tuple.getPort();
   }
   else
   {
      resip_assert(0);
   }

   ostrm << " " << Tuple::toData(tuple.mTransportType);

   if (!tuple.mTargetDomain.empty())
   {
       ostrm << " targetDomain=" << tuple.mTargetDomain;
   }
   
   if(tuple.mFlowKey != 0)
   {
      ostrm << " flowKey=" << tuple.mFlowKey;
   }

   if(tuple.mTransportKey != 0)
   {
      ostrm << " transportKey=" << tuple.mTransportKey;
   }

#ifdef USE_NETNS
      ostrm << " mNetNs=" << tuple.mNetNs;
#endif

   ostrm << " ]";
   
   return ostrm;
}

size_t 
Tuple::hash() const
{
   // !dlb! do not include the connection
#ifdef USE_IPV6
   if (mSockaddr.sa_family == AF_INET6)
   {
      const sockaddr_in6& in6 =
         reinterpret_cast<const sockaddr_in6&>(mSockaddr);

      return size_t(Data(Data::Share, (const char *)&in6.sin6_addr.s6_addr, sizeof(in6.sin6_addr.s6_addr)).hash() +
#ifdef USE_NETNS
                    mNetNs.hash() +
#endif
                    5*in6.sin6_port +
                    25*mTransportType);
   }
   else
#endif
   {
      const sockaddr_in& in4 =
         reinterpret_cast<const sockaddr_in&>(mSockaddr);
         
      return size_t(in4.sin_addr.s_addr +
#ifdef USE_NETNS
                    mNetNs.hash() +
#endif
                    5*in4.sin_port +
                    25*mTransportType);
   }    
}

HashValueImp(resip::Tuple, data.hash());

TransportType
Tuple::toTransport(const Data& transportName)
{
   return resip::toTransportType(transportName); // TransportTypes.hxx
};

const Data&
Tuple::toData(TransportType type)
{
   return resip::toData(type);  // TransportTypes.hxx
}

const Data&
Tuple::toDataLower(TransportType type)
{
   return resip::toDataLower(type);  // TransportTypes.hxx
}

Data
Tuple::inet_ntop(const Tuple& tuple)
{
#ifdef USE_IPV6
   if (!tuple.isV4())
   {
      const sockaddr_in6& addr = reinterpret_cast<const sockaddr_in6&>(tuple.getSockaddr());
      return DnsUtil::inet_ntop(addr.sin6_addr);
   }
   else
#endif
   {
      const sockaddr_in& addr = reinterpret_cast<const sockaddr_in&>(tuple.getSockaddr());
      return DnsUtil::inet_ntop(addr.sin_addr);
   }
}


bool
Tuple::isEqualWithMask(const Tuple& compare, short mask, bool ignorePort, bool ignoreTransport) const
{
   if(ignoreTransport || getType() == compare.getType())  // check if transport type matches
   {
      if (mSockaddr.sa_family == compare.getSockaddr().sa_family && mSockaddr.sa_family == AF_INET) // v4
      {
         sockaddr_in* addr1 = (sockaddr_in*)&mSockaddr;
         sockaddr_in* addr2 = (sockaddr_in*)&compare.getSockaddr();

         return ((ignorePort || addr1->sin_port == addr2->sin_port)  &&
                 (addr1->sin_addr.s_addr & htonl((0xFFFFFFFF << (32 - mask)))) == 
                  (addr2->sin_addr.s_addr & htonl((0xFFFFFFFF << (32 - mask)))));
      }
#ifdef USE_IPV6
      else if (mSockaddr.sa_family == compare.getSockaddr().sa_family && mSockaddr.sa_family == AF_INET6) // v6
      {
         sockaddr_in6* addr1 = (sockaddr_in6*)&mSockaddr;
         sockaddr_in6* addr2 = (sockaddr_in6*)&compare.getSockaddr();

         if(ignorePort || addr1->sin6_port == addr2->sin6_port)
         {
            UInt32 mask6part;
            UInt32 temp;
            bool match=true;
            for(int i = 3; i >= 0; i--)
            {
               if(mask <= 32*i)
               {
                  mask6part = 0;
               }
               else
               {
                  temp = mask - 32*i;
                  if(temp >= 32)
                  {
                     mask6part = 0xffffffff;
                  }
                  else
                  {
                     mask6part = 0xffffffff << (32 - temp);
                  }
               }
#ifdef WIN32
               if((*((unsigned long*)&addr1->sin6_addr.u.Word[i*2]) & htonl(mask6part)) != 
                  (*((unsigned long*)&addr2->sin6_addr.u.Word[i*2]) & htonl(mask6part)))
#elif defined(sun)
               // sun has no s6_addr16
               if((*((unsigned long*)&addr1->sin6_addr._S6_un._S6_u32[i]) & htonl(mask6part)) !=
                  (*((unsigned long*)&addr2->sin6_addr._S6_un._S6_u32[i]) & htonl(mask6part)))
#elif defined(__APPLE__) || defined(__OpenBSD__) || defined(__FreeBSD__)
               // bsd has no s6_addr16
               if((*((unsigned long*)&addr1->sin6_addr.__u6_addr.__u6_addr32[i]) & htonl(mask6part)) != 
                  (*((unsigned long*)&addr2->sin6_addr.__u6_addr.__u6_addr32[i]) & htonl(mask6part)))				  
#else
               if((*((UInt32*)&addr1->sin6_addr.s6_addr16[i*2]) & htonl(mask6part)) != 
                  (*((UInt32*)&addr2->sin6_addr.s6_addr16[i*2]) & htonl(mask6part)))
#endif
               {
                  match=false;
                  break;
               }
            }
            if(match)
            {
               return true;
            }
         }
      }
#endif
   }
   return false;
}


// special comparitors
bool
Tuple::AnyInterfaceCompare::operator()(const Tuple& lhs,
                                       const Tuple& rhs) const
{
   if (lhs.mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (lhs.mTransportType > rhs.mTransportType)
   {
      return false;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET && rhs.mSockaddr.sa_family == AF_INET)
   {
      if (lhs.m_anonv4.sin_port < rhs.m_anonv4.sin_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
#ifdef USE_IPV6
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      if (lhs.m_anonv6.sin6_port < rhs.m_anonv6.sin6_port)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif
   else
   {
      return false;
   }
};

bool
Tuple::AnyPortCompare::operator()(const Tuple& lhs,
                                  const Tuple& rhs) const
{
   if (lhs.mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (lhs.mTransportType > rhs.mTransportType)
   {
      return false;
   }

   // transport types equal, so compare addresses
   if (lhs.mSockaddr.sa_family == AF_INET && rhs.mSockaddr.sa_family == AF_INET)
   {
      int c = memcmp(&lhs.m_anonv4.sin_addr,
                     &rhs.m_anonv4.sin_addr,
                     sizeof(in_addr));

      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
   }
#ifdef USE_IPV6
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      int c = memcmp(&lhs.m_anonv6.sin6_addr,
                     &rhs.m_anonv6.sin6_addr,
                     sizeof(in6_addr));
      if (c < 0)
      {
         return true;
      }
      else if (c > 0)
      {
         return false;
      }
   }
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif
#ifdef USE_NETNS
   // transport type and addresses are equal, so compare netns
   if(lhs.mNetNs < rhs.mNetNs)
   {
       //DebugLog(<< "AnyPortCompare netns less than (l=" << lhs.mNetNs << ", r=" << rhs.mNetNs);
       return(true);
   }
   else if(rhs.mNetNs < lhs.mNetNs)
   {
       //DebugLog(<< "AnyPortCompare netns greater than (l=" << lhs.mNetNs << ", r=" << rhs.mNetNs);
       return(false);
   }
   //DebugLog(<< "AnyPortCompare netns equal to (l=\"" << lhs.mNetNs << "\", r=\"" << rhs.mNetNs << "\"");
#endif

   return false;
}

bool
Tuple::FlowKeyCompare::operator()(const Tuple& lhs,
                                  const Tuple& rhs) const
{
   if (lhs == rhs)
   {
      return lhs.mFlowKey < rhs.mFlowKey;
   }
   return lhs < rhs;
};

GenericIPAddress 
Tuple::toGenericIPAddress() const
{
   if (isV4())
   {
      return GenericIPAddress(m_anonv4);
   }
   else
#ifdef USE_IPV6
  {
      return GenericIPAddress(m_anonv6);
  }
#else
  {
     resip_assert(0);
     return m_anonv4; //bogus
  }
#endif
}

bool
Tuple::AnyPortAnyInterfaceCompare::operator()(const Tuple& lhs,
                                              const Tuple& rhs) const
{
   if (lhs.mTransportType < rhs.mTransportType)
   {
      return true;
   }
   else if (lhs.mTransportType > rhs.mTransportType)
   {
      return false;
   }
#ifdef USE_IPV6
   else if (lhs.mSockaddr.sa_family == AF_INET6 &&
            rhs.mSockaddr.sa_family == AF_INET)
   {
      return true;
   }
   else if (lhs.mSockaddr.sa_family == AF_INET &&
            rhs.mSockaddr.sa_family == AF_INET6)
   {
      return false;
   }
#endif
   else
   {
      return false;
   }
};

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
