#if defined(_WIN32)
#include <Winsock2.h>
#include <Iphlpapi.h>
#endif

#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/WinCompat.hxx"

using namespace resip;

WinCompat::Exception::Exception(const char* msg, const char* file, const int line) :
BaseException(msg,file,line)
{
}

WinCompat::Version
WinCompat::getVersion()
{
#if defined(_WIN32)
   BOOL bOsVersionInfoEx;
#ifdef _WIN32_WCE
   OSVERSIONINFO osvi;
   ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#else
   OSVERSIONINFOEX osvi;
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#endif

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

   return WindowsUnknown;
#else
   return WinCompat::NotWindows;
#endif
}

Tuple
WinCompat::determineSourceInterface(const Tuple& destination)
{
   // Note:  IPHLPAPI has been known to conflict with some thirdparty DLL's if linked in
   //        statically.  If you don't care about Win95/98/Me as your target system - then
   //        you can define NO_IPHLPAPI so that you are not required to link with this 
   //        library. (SLG)
#if defined(WIN32) && !defined(NO_IPHLPAPI)  
#ifndef _WIN32_WCE
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
         if(entry.dwIndex == bestRoute.dwForwardIfIndex) 
         {
            bool valid = false;
            if (gw) // !nash! somehow on Windows Vista this value is 0, may be bug inside GetBestRoute() API in the IPHLPAPI lib
            {
               if ((entry.dwAddr & entry.dwMask) == (gw & entry.dwMask))
               {
                  valid = true;
               }
            }
            else
            {
               valid = true;
            }
            if (valid)
            {
               sourceIP.s_addr = entry.dwAddr;
               break;
            }
         }
      }
   }

   delete [] (char *) pIpAddrTable;
   return Tuple(sourceIP, 0, destination.getType());
#else
   return Tuple();
#endif

#else
   assert(0);
   return Tuple();
#endif
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
