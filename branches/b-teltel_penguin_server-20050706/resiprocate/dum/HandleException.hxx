#if !defined(RESIP_HANDLEEXCEPTION_HXX)
#define RESIP_HANDLEEXCEPTION_HXX

#include "resiprocate/os/BaseException.hxx"

namespace resip
{

class HandleException : public BaseException
{
   public:
      HandleException(const Data& msg, const Data& file, int line);
      virtual const char* name() const;
};

}

#endif
