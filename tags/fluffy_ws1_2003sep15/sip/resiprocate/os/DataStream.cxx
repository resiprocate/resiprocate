#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Data.hxx"

using namespace resip;

DataBuffer::DataBuffer(Data& str)
   : mStr(str)
{
   char* gbuf = const_cast<char*>(mStr.mBuf);
   setg(gbuf, gbuf, gbuf+mStr.size());
   // expose the excess capacity as the put buffer
   setp(gbuf+mStr.mSize, gbuf+mStr.mCapacity);
}

DataBuffer::~DataBuffer()
{
}

int
DataBuffer::sync()
{
   size_t len = pptr() - pbase();
   if (len > 0) 
   {
      size_t pos = gptr() - eback();  // remember the get position
      mStr.mSize += len;
      char* gbuf = const_cast<char*>(mStr.data());
      // reset the get buffer
      setg(gbuf, gbuf+pos, gbuf+mStr.size()); 
      // reset the put buffer
      setp(gbuf + mStr.mSize, gbuf + mStr.mCapacity);
   }
   return 0;
}

int
DataBuffer::overflow(int c)
{
   // sync, but reallocate
   size_t len = pptr() - pbase();
   if (len > 0) 
   {
      size_t pos = gptr() - eback();  // remember the get position

      // update the length
      mStr.mSize += len;

      // resize the underlying Data and reset the input buffer
      mStr.resize(((mStr.mCapacity+16)*3)/2, true);

      char* gbuf = const_cast<char*>(mStr.mBuf);
      // reset the get buffer
      setg(gbuf, gbuf+pos, gbuf+mStr.mSize); 
      // reset the put buffer
      setp(gbuf + mStr.mSize, gbuf + mStr.mCapacity);
   }
   if (c != -1) 
   {
      mStr.mBuf[mStr.mSize] = c;
      pbump(1);
      return c;
   }
   return 0;
}

iDataStream::iDataStream(Data& str)
   : std::istream(0),
     mStreambuf(str)
{
   init(&mStreambuf);
}

iDataStream::~iDataStream()
{
}

oDataStream::oDataStream(Data& str)
   : std::ostream(0),
     mStreambuf(str)
{
   init(&mStreambuf);
}

oDataStream::~oDataStream()
{
   flush();
}

DataStream::DataStream(Data& str)
   : std::iostream(0),
     mStreambuf(str)
{
   init(&mStreambuf);
}

DataStream::~DataStream()
{
   flush();
}
