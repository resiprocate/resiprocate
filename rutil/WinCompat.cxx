#include <Winsock2.h>
#include <Iphlpapi.h>
#include <atlconv.h>

#include "rutil/Tuple.hxx"
#include "rutil/WinCompat.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


using namespace resip;

WinCompat::Exception::Exception(const Data& msg, const Data& file, const int line) :
   BaseException(msg,file,line)
{
}


WinCompat::Version
WinCompat::getVersion()
{
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
   loadLibraryAlreadyFailed(false), getBestInterfaceEx(0), getAdaptersAddresses(0)
{

   // Note:  IPHLPAPI has been known to conflict with some thirdparty DLL's.
   //        If you don't care about Win95/98/Me as your target system - then
   //        you can define NO_IPHLPAPI so that you are not required to link with this 
   //        library. (SLG)
#if !defined (NO_IPHLPAPI)
   // check to see if the GetAdaptersAddresses() is in the IPHLPAPI library
   HINSTANCE hLib = LoadLibraryA("iphlpapi.dll");
   if (hLib == NULL)
   {
      loadLibraryAlreadyFailed = true;
      return;
   }

   getBestInterfaceEx = (GetBestInterfaceExProc) GetProcAddress(hLib, "GetBestInterfaceEx");
   getAdaptersAddresses = (GetAdaptersAddressesProc) GetProcAddress(hLib, "GetAdaptersAddresses");
   if (getAdaptersAddresses == NULL || getBestInterfaceEx == NULL)
   {   
      loadLibraryAlreadyFailed = true;
      return;
   }
#else
   loadLibraryAlreadyFailed = true;
#endif
}


#if !defined(NO_IPHLPAPI)
// !slg! - This function is horribly slow (upto 200ms) and can cause serious performance issues for servers.
//         We should consider finding more efficient APIs, or caching some of the results.
Tuple
WinCompat::determineSourceInterfaceWithIPv6(const Tuple& destination)
{
   if (instance()->loadLibraryAlreadyFailed || (getVersion() < WinCompat::WindowsXP))
   {
      throw Exception("Library iphlpapi.dll with IPv6 support not available", __FILE__,__LINE__);
   }

   DWORD dwBestIfIndex;
   const sockaddr* saddr = &destination.getSockaddr();

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
                   Tuple tuple(*AI->FirstUnicastAddress->Address.lpSockaddr, destination.getType());
                   LocalFree(pAdapterAddresses);
                   return(tuple);
                }
            } 
         }
      }
      
      LocalFree(pAdapterAddresses);
      throw Exception("Can't find source address for destination", __FILE__,__LINE__);
   }

   return Tuple();
}


Tuple
WinCompat::determineSourceInterfaceWithoutIPv6(const Tuple& destination)
{

   // try to figure the best route to the destination
   MIB_IPFORWARDROW bestRoute;
   memset(&bestRoute, 0, sizeof(bestRoute));
   const sockaddr_in& sin = (const sockaddr_in&)destination.getSockaddr();
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
     
   struct in_addr sourceIP;
   sourceIP.s_addr = 0;
         
   if (NO_ERROR == GetIpAddrTable(pIpAddrTable, &addrSize, FALSE)) 
   {
      // try to find a match
      for (DWORD i=0; i<pIpAddrTable->dwNumEntries; i++) 
      {
         MIB_IPADDRROW &entry = pIpAddrTable->table[i];
   	   
         ULONG addr = pIpAddrTable->table[i].dwAddr;
         ULONG gw = bestRoute.dwForwardNextHop;
         if( (entry.dwIndex == bestRoute.dwForwardIfIndex) &&
             (entry.dwAddr & entry.dwMask) == (bestRoute.dwForwardNextHop & entry.dwMask) )
         {
            sourceIP.s_addr = entry.dwAddr;
            break;
         }
      }
   }
   
   delete [] (char *) pIpAddrTable;
   return Tuple(sourceIP, 0, destination.getType());
}
#endif // !defined(NO_IPHLPAPI)

Tuple
WinCompat::determineSourceInterface(const Tuple& destination)
{
// Note:  IPHLPAPI has been known to conflict with some thirdparty DLL's.
//        If you don't care about Win95/98/Me as your target system - then
//        you can define NO_IPHLPAPI so that you are not required to link with this 
//        library. (SLG)

#if !defined(NO_IPHLPAPI)
#if defined(USE_IPV6)
   try
   {
      if(destination.ipVersion() == V6)
      {
         return  determineSourceInterfaceWithIPv6(destination);
      }
      else
      {
         return determineSourceInterfaceWithoutIPv6(destination);
      }
   }
   catch (...)
   {
   }
#endif
   try
   {
      return determineSourceInterfaceWithoutIPv6(destination);
   }
   catch (...)
   {
   }
#else
   assert(0);
#endif
   return Tuple();
}


std::list<std::pair<Data,Data> >
WinCompat::getInterfaces(const Data& matching)
{
   if (instance()->loadLibraryAlreadyFailed || (getVersion() < WinCompat::WindowsXP))
   {
      throw Exception("Library iphlpapi.dll with IPv6 support not available", __FILE__,__LINE__);
   }

   // Obtain the size of the structure
   IP_ADAPTER_ADDRESSES *pAdapterAddresses;
   std::list<std::pair<Data,Data> > results;
   DWORD dwRet, dwSize;
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
