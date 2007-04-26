#include "resiprocate/os/SysLogBuf.hxx"
#include "resiprocate/os/SysLogStream.hxx"

using resip::SysLogStream;
using resip::SysLogBuf;

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif

SysLogStream::SysLogStream() :
   std::ostream (this)
{
}

SysLogStream::SysLogStream(int facility, int level) :
   SysLogBuf(facility, level),
   std::ostream(this)
{
}

SysLogStream::~SysLogStream()
{
}
