#include "rutil/ParseException.hxx"

namespace resip
{

ParseException::ParseException(const Data& msg, 
                                 const Data& context, 
                                 const Data& file, 
                                 const int line)
   : resip::BaseException(msg, file, line),
   mContext(context)
{}

const char* 
ParseException::name() const noexcept
{ 
   return "ParseException"; 
}

const Data& 
ParseException::getContext() const noexcept
{
   return mContext;
}

}
