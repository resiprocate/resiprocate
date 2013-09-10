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

ParseException::~ParseException() throw() 
{}

const char* 
ParseException::name() const 
{ 
   return "ParseException"; 
}

const Data& 
ParseException::getContext() const
{
   return mContext;
}

}
