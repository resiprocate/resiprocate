#include "resiprocate/os/SysLogBuf.hxx"
#include "resiprocate/os/SysLogStream.hxx"

using resip::SysLogStream;
using resip::SysLogBuf;


SysLogStream::SysLogStream() :
   std::ostream (this)
   //std::ostream (_buf = new SysLogBuf)
{
}

SysLogStream::~SysLogStream()
{
   //delete _buf;
}
