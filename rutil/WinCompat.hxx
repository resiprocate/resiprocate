#if !defined(resip_WinCompat_hxx)
#define resip_WinCompat_hxx

#if defined(WIN32)
#include <Iphlpapi.h>
#include <list>

#include "rutil/BaseException.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/GenericIPAddress.hxx"

namespace resip
{

/**
   @brief Class for handling compatibility across the multiple versions of 
      Windows.
*/
class WinCompat
{
   public:
      enum Version
      {
         NotWindows,
         // Legacy (Win9x - no longer supported, retained for compatibility)
         Windows95,
         Windows98,
         Windows98SE,
         WindowsME,
         // NT family
         WindowsNT,
         Windows2000,
         WindowsXP,
         Windows2003Server,
         WindowsVista,
         WindowsServer2008,
         Windows7,
         WindowsServer2008R2,
         Windows8,
         WindowsServer2012,
         Windows81,
         WindowsServer2012R2,
         Windows10,
         WindowsServer2016,
         WindowsServer2019,
         WindowsServer2022,
         Windows11,
         WindowsUnknown
      };

      static Version getVersion();

      class Exception final : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, int line);
            const char* name() const noexcept override { return "TransportException"; }
      };

      static GenericIPAddress determineSourceInterface(const GenericIPAddress& destination);
      static std::list<std::pair<Data,Data> > getInterfaces(const Data& matching);
      static void destroyInstance();

      static bool windowsEventLog(WORD type, WORD numStrings, LPCTSTR* strings);

   private:
      static WinCompat* instance();
      static WinCompat* mInstance;

      static GenericIPAddress determineSourceInterfaceWithIPv6(const GenericIPAddress& destination);
      static GenericIPAddress determineSourceInterfaceWithoutIPv6(const GenericIPAddress& destination);
      typedef DWORD (WINAPI * GetBestInterfaceExProc)(const sockaddr *, DWORD *);
      typedef DWORD (WINAPI * GetAdaptersAddressesProc)(ULONG, DWORD, VOID *, IP_ADAPTER_ADDRESSES *, ULONG *);
      typedef DWORD (WINAPI * GetAdaptersInfoProc)(PIP_ADAPTER_INFO, PULONG);
      typedef DWORD (WINAPI * GetBestRouteProc)(DWORD dwDestAddr, DWORD dwSourceAddr, PMIB_IPFORWARDROW pBestRoute);
      typedef DWORD (WINAPI * GetIpAddrTableProc)(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize, BOOL bOrder);

      WinCompat();
      ~WinCompat();

      GetBestInterfaceExProc getBestInterfaceEx;
      GetAdaptersAddressesProc getAdaptersAddresses;
      GetAdaptersInfoProc getAdaptersInfo;
      GetBestRouteProc getBestRoute;
      GetIpAddrTableProc getIpAddrTable;
      bool loadLibraryWithIPv4Failed;
      bool loadLibraryWithIPv6Failed;
      HMODULE hLib;
};

}

#endif // WIN32
#endif
