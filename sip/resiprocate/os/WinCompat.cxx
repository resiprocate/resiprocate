#if defined(WIN32)
#include <Winsock2.h>
#include <Iphlpapi.h>
#endif

#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/WinCompat.hxx"

using namespace resip;

WinCompat::Exception::Exception(const Data& msg, const Data& file, const int line) :
   BaseException(msg,file,line)
{
}

WinCompat::Version
WinCompat::getVersion()
{
#if defined(WIN32)
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
             (entry.dwAddr & entry.dwMask) == (bestRoute.dwForwardNextHop & entry.dwMask) ) {
            sourceIP.s_addr = entry.dwAddr;
            break;
         }
      }
   }

   delete [] (char *) pIpAddrTable;
   return Tuple(sourceIP, 0, destination.getType());
#else
   assert(0);
   return Tuple();
#endif
}
