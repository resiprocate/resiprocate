#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/KeepAliveTimeout.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/os/DataStream.hxx"

using namespace resip;

KeepAliveTimeout::KeepAliveTimeout(const Tuple& target)
   : mTarget(target)
{
}

KeepAliveTimeout::KeepAliveTimeout(const KeepAliveTimeout& timeout)
{
   mTarget = timeout.target();
}

KeepAliveTimeout::~KeepAliveTimeout()
{
}

Message*
KeepAliveTimeout::clone() const
{
   return new KeepAliveTimeout(*this);
}

Data
KeepAliveTimeout::brief() const
{
   Data data;
   DataStream strm(data);
   encode(strm);
   strm.flush();
   return data;
}

std::ostream& 
KeepAliveTimeout::encode(std::ostream& strm) const
{
   strm << mTarget;
   return strm;
}
