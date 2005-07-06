#if !defined(resip_WinCompat_hxx)
#define resip_WinCompat_hxx

#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/Tuple.hxx"

namespace resip
{

class WinCompat
{
   public:
      //typedef enum Version
      // above is not valid C++ syntax
      enum Version
      {
         Windows2003Server,
         WindowsXP,
         Windows2000,
         WindowsNT,
         Windows98,
         Windows98SE,
         Windows95,
         WindowsME,
         WindowsUnknown,
         NotWindows
      };

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "TransportException"; }
      };

      static Version getVersion();
      static Tuple determineSourceInterface(const Tuple& destination);
};

}

#endif
