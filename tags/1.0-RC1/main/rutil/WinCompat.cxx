#include <Winsock2.h>
#include <Iphlpapi.h>
#include <atlbase.h>

#include "rutil/GenericIPAddress.hxx"
#include "rutil/WinCompat.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace resip;

class WinCompatInstanceCleaner
{
public:
    ~WinCompatInstanceCleaner() { WinCompat::destroyInstance(); }
} winCompatInstanceCleaner;

WinCompat::Exception::Exception(const Data& msg, const Data& file, const int line) :
   BaseException(msg,file,line)
{
}


WinCompat::Version
WinCompat::getVersion()
{
#ifdef UNDER_CE
#define OSVERSIONINFOEX OSVERSIONINFO
#endif
   OSVERSIONINFOEX osvi;
   BOOL bOsVersionInfoEx;

   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
   {
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
      {
         return WindowsUnknown;
      }
   }

   switch (osvi.dwPlatformId)
   {
      // Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:

         // Test for the specific product family.
         if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
         {
            return WinCompat::Windows2003Server;
         }
         else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
         {
            return WinCompat::WindowsXP;
         }
         else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
         {
            return WinCompat::Windows2000;
         }
         else if ( osvi.dwMajorVersion <= 4 )
         {
            return WinCompat::WindowsNT;
         }
         break;
         
         // Test for the Windows 95 product family.
      case VER_PLATFORM_WIN32_WINDOWS:
         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
         {
            return WinCompat::Windows95;
         } 
         else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
         {
            if ( osvi.szCSDVersion[1] == 'A' )
            {
               return WinCompat::Windows98SE;
            }
            else
            {
               return WinCompat::Windows98;
            }
         }
         else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
         {
            return WinCompat::WindowsME;
         } 
         break;
         
      default:
         return WinCompat::WindowsUnknown;
   }

   return WinCompat::WindowsUnknown;
}


WinCompat* WinCompat::mInstance = 0;


WinCompat *
WinCompat::instance()
{
   static Mutex mutex;
   if (!mInstance)
   {
      Lock lock(mutex);
      if (!mInstance)
      {
         mInstance = new WinCompat();
      }
   }
   return mInstance;
}


WinCompat::WinCompat() :
   getBestInterfaceEx(0), 
   getAdaptersAddresses(0),
   getAdaptersInfo(0),
   loadLibraryWithIPv4Failed(false), 
   loadLibraryWithIPv6Failed(false),
   hLib(0)
{

   // Note:  IPHLPAPI has been known to conflict with some thirdparty DLL's.
   //        If you don't care about Win95/98/Me as your target system - then
   //        you can define NO_IPHLPAPI so that you are not required to link with this 
   //        library. (SLG)
#if !defined (NO_IPHLPAPI)
   // check to see if the GetAdaptersAddresses() is in the IPHLPAPI library
   HINSTANCE hLib = LoadLibrary(TEXT("iphlpapi.dll"));
   if (hLib == NULL)
   {
      loadLibraryWithIPv6Failed = true;
      loadLibraryWithIPv4Failed = true;
      return;
   }

   getBestInterfaceEx = (GetBestInterfaceExProc) GetProcAddress(hLib, TEXT("GetBestInterfaceEx"));
   getAdaptersAddresses = (GetAdaptersAddressesProc) GetProcAddress(hLib, TEXT("GetAdaptersAddresses"));
   getAdaptersInfo = (GetAdaptersInfoProc) GetProcAddress(hLib, TEXT("GetAdaptersInfo"));
   if (getAdaptersAddresses == NULL || getBestInterfaceEx == NULL)
   {   
      loadLibraryWithIPv6Failed = true;
   }
   if (getAdaptersInfo == NULL)
   {
      loadLibraryWithIPv4Failed = true;
   }
#else
   loadLibraryWithIPv6Failed = true;
   loadLibraryWithIPv4Failed = true;
#endif
}

void WinCompat::destroyInstance()
{
    delete mInstance;
    mInstance = 0;
}

WinCompat::~WinCompat()
{
   if(hLib != NULL)
   {
      FreeLibrary(hLib);
   }
}


#if !defined(NO_IPHLPAPI)
// !slg! - This function is horribly slow (upto 200ms) and can cause serious performance issues for servers.
//         We should consider finding more efficient APIs, or caching some of the results.
GenericIPAddress
WinCompat::determineSourceInterfaceWithIPv6(const GenericIPAddress& destination)
{
   if (instance()->loadLibraryWithIPv6Failed)
   {
      throw Exception("Library iphlpapi.dll with IPv6 support not available", __FILE__,__LINE__);
   }

   DWORD dwBestIfIndex;
   const sockaddr* saddr = &destination.address;

   if ((instance()->getBestInterfaceEx)((sockaddr *)saddr, &dwBestIfIndex) != NO_ERROR)
   {
      throw Exception("Can't find source address for destination", __FILE__,__LINE__);
   }

   // Obtain the size of the structure
   IP_ADAPTER_ADDRESSES *pAdapterAddresses;
   DWORD dwRet, dwSize;
   DWORD flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
   dwRet = (instance()->getAdaptersAddresses)(saddr->sa_family, flags, NULL, NULL, &dwSize);
   if (dwRet == ERROR_BUFFER_OVERFLOW)  // expected error
   {
      // Allocate memory
      pAdapterAddresses = (IP_ADAPTER_ADDRESSES *) LocalAlloc(LMEM_ZEROINIT,dwSize);
      if (pAdapterAddresses == NULL) 
      {
         throw Exception("Can't find source address for destination", __FILE__,__LINE__);
      }

      // Obtain network adapter information (IPv6)
      dwRet = (instance()->getAdaptersAddresses)(saddr->sa_family, flags, NULL, pAdapterAddresses, &dwSize);
      if (dwRet != ERROR_SUCCESS) 
      {
         LocalFree(pAdapterAddresses);
         throw Exception("Can't find source address for destination", __FILE__,__LINE__);
      } 
      else 
      {
         IP_ADAPTER_ADDRESSES *AI;
         int i;
         for (i = 0, AI = pAdapterAddresses; AI != NULL; AI = AI->Next, i++) 
         {
             if (AI->FirstUnicastAddress != NULL) 
             {
                if (AI->FirstUnicastAddress->Address.lpSockaddr->sa_family != saddr->sa_family)
                   continue;
                if ((saddr->sa_family == AF_INET6 && AI->Ipv6IfIndex == dwBestIfIndex) ||
                   (saddr->sa_family == AF_INET && AI->IfIndex == dwBestIfIndex))
                {
                   GenericIPAddress ipaddress(*AI->FirstUnicastAddress->Address.lpSockaddr);
                   LocalFree(pAdapterAddresses);
                   return(ipaddress);
                }
            } 
         }
      }
      
      LocalFree(pAdapterAddresses);
      throw Exception("Can't find source address for destination", __FILE__,__LINE__);
   }

   return GenericIPAddress();
}


GenericIPAddress
WinCompat::determineSourceInterfaceWithoutIPv6(const GenericIPAddress& destination)
{
   if (instance()->loadLibraryWithIPv4Failed)
   {
      throw Exception("Library iphlpapi.dll not available", __FILE__,__LINE__);
   }

   // try to figure the best route to the destination
   MIB_IPFORWARDROW bestRoute;
   memset(&bestRoute, 0, sizeof(bestRoute));
   const sockaddr_in& sin = (const sockaddr_in&)destination.address;
   if (NO_ERROR != GetBestRoute(sin.sin_addr.s_addr, 0, &bestRoute)) 
   {
      throw Exception("Can't find source address for destination", __FILE__,__LINE__);
   }
      
   // look throught the local ip address to find one that match the best route.
   PMIB_IPADDRTABLE  pIpAddrTable = NULL;
   ULONG addrSize = 0;
         
   // allocate the space
   if (ERROR_INSUFFICIENT_BUFFER == GetIpAddrTable(NULL, &addrSize, FALSE))
   {
      pIpAddrTable = (PMIB_IPADDRTABLE) new char [addrSize];
   } 
   else 
   {
      throw Exception("Can't find source address for destination", __FILE__,__LINE__);
   }
     
   struct sockaddr_in sourceIP;
   memset(&sourceIP, 0, sizeof(sockaddr_in));
         
   if (NO_ERROR == GetIpAddrTable(pIpAddrTable, &addrSize, FALSE)) 
   {
      enum ENICEntryPreference {ENICUnknown = 0, ENextHopNotWithinNICSubnet, ENICSubnetIsAll1s, ENICServicesNextHop};
      ENICEntryPreference eCurrSelection = ENICUnknown;

      sourceIP.sin_family = AF_INET;
      // try to find a match
      for (DWORD i=0; i<pIpAddrTable->dwNumEntries; i++) 
      {
         MIB_IPADDRROW &entry = pIpAddrTable->table[i];
   	   
         ULONG addr = pIpAddrTable->table[i].dwAddr;
         ULONG gw = bestRoute.dwForwardNextHop;
         if(entry.dwIndex == bestRoute.dwForwardIfIndex)    // Note: there MAY be > 1 entry with the same index, see AddIPAddress.
         {
            if( (entry.dwAddr & entry.dwMask) == (bestRoute.dwForwardNextHop & entry.dwMask) )
            {
               sourceIP.sin_addr.s_addr = entry.dwAddr;
               eCurrSelection = ENICServicesNextHop;
               break;
            }
            else if (entry.dwMask == 0xffffffff && eCurrSelection < ENICSubnetIsAll1s)
            {
               sourceIP.sin_addr.s_addr = entry.dwAddr;
               eCurrSelection = ENICSubnetIsAll1s;    // Lucent/Avaya VPN has Subnet Mask of 255.255.255.255. Illegal perhaps but we should use
                                                      // if cannot find one that serves next hop.
            }
            else if (eCurrSelection < ENextHopNotWithinNICSubnet)
            {
               sourceIP.sin_addr.s_addr = entry.dwAddr;
               eCurrSelection = ENextHopNotWithinNICSubnet;   // should use if nothing else works since bestRoute told us to use this NIC.
            }
         }
      }

      if (eCurrSelection != ENICServicesNextHop)
      {
         // rarely happens but it does. We would fail before with the old code, let's see what the ip table looks like.
         in_addr subnet, netMask, nextHop;
         subnet.s_addr = bestRoute.dwForwardDest;
         netMask.s_addr = bestRoute.dwForwardMask;
         nextHop.s_addr = bestRoute.dwForwardNextHop;
         // best-route
         DebugLog(<< "Best Route - subnet=" <<DnsUtil::inet_ntop(subnet)
                  <<" net-mask=" <<DnsUtil::inet_ntop(netMask)
                  <<" next-hop=" <<DnsUtil::inet_ntop(nextHop)
                  <<" if-index=" <<bestRoute.dwForwardIfIndex );
         // ip-table
         for (DWORD i=0; i<pIpAddrTable->dwNumEntries; i++)
         {
            MIB_IPADDRROW & entry = pIpAddrTable->table[i];
            in_addr nicIP, nicMask;
            nicIP.s_addr = entry.dwAddr;
            nicMask.s_addr = entry.dwMask;
            DebugLog(<<"IP Table entry " <<i+1 <<'/' <<pIpAddrTable->dwNumEntries <<" if-index=" <<entry.dwIndex
                     <<" NIC IP=" <<DnsUtil::inet_ntop(nicIP)
                     <<" NIC Mask=" <<DnsUtil::inet_ntop(nicMask) );
         }
      }
   }
   
   delete [] (char *) pIpAddrTable;
   return GenericIPAddress(sourceIP);
}
#endif // !defined(NO_IPHLPAPI)

GenericIPAddress
WinCompat::determineSourceInterface(const GenericIPAddress& destination)
{
// Note:  IPHLPAPI has been known to conflict with some thirdparty DLL's.
//        If you don't care about Win95/98/Me as your target system - then
//        you can define NO_IPHLPAPI so that you are not required to link with this 
//        library. (SLG)

#if !defined(NO_IPHLPAPI)

   if(destination.isVersion6())
   {
      return  determineSourceInterfaceWithIPv6(destination);
   }
   else
   {
      return determineSourceInterfaceWithoutIPv6(destination);
   }  

#else
   assert(0);
#endif
   return GenericIPAddress();
}


std::list<std::pair<Data,Data> >
WinCompat::getInterfaces(const Data& matching)
{
   if (instance()->loadLibraryWithIPv6Failed)
   {
      if (instance()->loadLibraryWithIPv4Failed)
      {
         throw Exception("Library iphlpapi.dll not available", __FILE__,__LINE__);
      }

      // Do IPV4 only lookup - useful for Windows versions less than W2k3 and WinXp
      // Note:  This query does not include the Loopback Adapter
      PIP_ADAPTER_INFO pAdaptersInfo=NULL;
      std::list<std::pair<Data,Data> > results;
      DWORD dwRet, dwSize=0;
      dwRet = (instance()->getAdaptersInfo)(pAdaptersInfo, &dwSize);
      if (dwRet == ERROR_BUFFER_OVERFLOW)  // expected error
      {
         // Allocate memory
         pAdaptersInfo = (PIP_ADAPTER_INFO) LocalAlloc(LMEM_ZEROINIT,dwSize);
         if (pAdaptersInfo == NULL) 
         {
            throw Exception("Can't query for adapters info - LocalAlloc error", __FILE__,__LINE__);
         }
         dwRet = (instance()->getAdaptersInfo)(pAdaptersInfo, &dwSize);
         if (dwRet != ERROR_SUCCESS) 
         {
            LocalFree(pAdaptersInfo);
            throw Exception("Can't query for adapters info - GetAdaptersInfo", __FILE__,__LINE__);
         } 
         else 
         {
            IP_ADAPTER_INFO *AI;
            int i;
            for (i = 0, AI = pAdaptersInfo; AI != NULL; AI = AI->Next, i++) 
            {
               //Data name(AI->AdapterName);
               Data name(AI->Description);
               if(matching == Data::Empty || name == matching)
               {
                  results.push_back(std::make_pair(name, Data(AI->IpAddressList.IpAddress.String)));
               }
            }
            LocalFree(pAdaptersInfo);
         }
      }
      return results;
   }

   // Use IPV6 compatible query
   // Note:  This query includes the Loopback Adapter
   // Obtain the size of the structure
   IP_ADAPTER_ADDRESSES *pAdapterAddresses;
   std::list<std::pair<Data,Data> > results;
   DWORD dwRet, dwSize=0;
   DWORD flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
   dwRet = (instance()->getAdaptersAddresses)(AF_UNSPEC, flags, NULL, NULL, &dwSize);
   if (dwRet == ERROR_BUFFER_OVERFLOW)  // expected error
   {
      // Allocate memory
      pAdapterAddresses = (IP_ADAPTER_ADDRESSES *) LocalAlloc(LMEM_ZEROINIT,dwSize);
      if (pAdapterAddresses == NULL) 
      {
         throw Exception("Can't query for adapter addresses - LocalAlloc error", __FILE__,__LINE__);
      }

      // Obtain network adapter information (IPv6)
      dwRet = (instance()->getAdaptersAddresses)(AF_UNSPEC, flags, NULL, pAdapterAddresses, &dwSize);
      if (dwRet != ERROR_SUCCESS) 
      {
         LocalFree(pAdapterAddresses);
         throw Exception("Can't query for adapter addresses - GetAdapterAddresses", __FILE__,__LINE__);
      } 
      else 
      {
         IP_ADAPTER_ADDRESSES *AI;
         USES_CONVERSION;
         int i;
         for (i = 0, AI = pAdapterAddresses; AI != NULL; AI = AI->Next, i++) 
         {
            if (AI->FirstUnicastAddress != NULL) 
            {
               //Data name(AI->AdapterName); 
               Data name(W2A(AI->FriendlyName));
               if(matching == Data::Empty || name == matching)
               {
                  results.push_back(std::make_pair(name, DnsUtil::inet_ntop(*AI->FirstUnicastAddress->Address.lpSockaddr)));
               }
            } 
         }
         LocalFree(pAdapterAddresses);
      }      
   }
   return results;
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
