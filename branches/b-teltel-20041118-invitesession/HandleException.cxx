#include "resiprocate/dum/HandleException.hxx"

using namespace resip;

HandleException::HandleException(const Data& msg,const Data& file,int line)
   : BaseException(msg, file, line)
{
}

const char*
HandleException::name() const
{
   return "HandleException";
}
