#if !defined(RESIP_DUMEXCEPTION_HXX)
#define RESIP__DUMEXCEPTION_HXX

#include "resiprocate/os/BaseException.hxx"

namespace resip
{

class DumException : public BaseException
{
   public:
      DumException(const Data& msg, const Data& file, const int line)
         : BaseException(msg, file, line) {}
      const char* name() const { return "DumException"; }
};
 
}

#endif

