#include "resiprocate/os/CountStream.hxx"

using namespace resip;

static const int BuffSize(2048);
// singleton buffer -- not really used
static char Buffer[BuffSize];

CountBuffer::CountBuffer()
   : mCount(0)
{
   setp(Buffer, Buffer+BuffSize);
}

CountBuffer::~CountBuffer()
{}

int
CountBuffer::sync()
{
   size_t len = pptr() - pbase();
   if (len > 0) 
   {
      mCount += len;
      // reset the put buffer
      setp(Buffer, Buffer + BuffSize);
   }
   return 0;
}

int
CountBuffer::overflow(int c)
{
   sync();
   if (c != -1) 
   {
      pbump(1);
      return c;
   }
   return 0;
}

CountStream::CountStream()
   : std::ostream(0),
     mStreamBuf()
{
   init(&mStreamBuf);
}

CountStream::~CountStream()
{}

