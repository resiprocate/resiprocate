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
      //typedef enum Version
      // above is not valid C++ syntax
      enum Version
      {
         NotWindows,
         Windows95,
         Windows98,
         Windows98SE,
         WindowsME,
         WindowsNT,
         Windows2000,
         WindowsXP,
         Windows2003Server,
         WindowsUnknown
      };

      static Version getVersion();

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "TransportException"; }
      };

      static GenericIPAddress determineSourceInterface(const GenericIPAddress& destination);
      static std::list<std::pair<Data,Data> > getInterfaces(const Data& matching);
      static void destroyInstance();

   private:
      static WinCompat* instance();
      static WinCompat* mInstance;

      static GenericIPAddress WinCompat::determineSourceInterfaceWithIPv6(const GenericIPAddress& destination);
      static GenericIPAddress WinCompat::determineSourceInterfaceWithoutIPv6(const GenericIPAddress& destination);
      typedef DWORD (WINAPI * GetBestInterfaceExProc)(const sockaddr *, DWORD *);
      typedef DWORD (WINAPI * GetAdaptersAddressesProc)(ULONG, DWORD, VOID *, IP_ADAPTER_ADDRESSES *, ULONG *);
      typedef DWORD (WINAPI * GetAdaptersInfoProc)(PIP_ADAPTER_INFO, PULONG);

      WinCompat();
      ~WinCompat();

      GetBestInterfaceExProc getBestInterfaceEx;
      GetAdaptersAddressesProc getAdaptersAddresses;
      GetAdaptersInfoProc getAdaptersInfo;
      bool loadLibraryWithIPv4Failed;
      bool loadLibraryWithIPv6Failed;
      HMODULE hLib;
};

}

#endif // WIN32
#endif
