#if !defined(RESIP_USAGEUSEEXCEPTION_HXX)
#define RESIP_USAGEUSEEXCEPTION_HXX 

#include "resiprocate/os/BaseException.hxx"

namespace resip
{

class UsageUseException : public BaseException
{
   public:
      UsageUseException(const Data& msg, const Data& file, const int line)
         : BaseException(msg, file, line) {}
      const char* name() const { return "UsageUseException"; }
};
 
}

#endif

