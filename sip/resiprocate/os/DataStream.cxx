#include "util/DataStream.hxx"

using namespace Vocal2;

DataBuffer::DataBuffer(Data& str)
   : _str(str)
{
   char* gbuf = const_cast<char*>(_str.mBuf);
   setg(gbuf, gbuf, gbuf+_str.size());
   // expose the excess capacity as the put buffer
   setp(gbuf+_str.mSize, gbuf+_str.mCapacity);
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
      _str.mSize += len;
      char* gbuf = const_cast<char*>(_str.data());
      // reset the get buffer
      setg(gbuf, gbuf+pos, gbuf+_str.size()); 
      // reset the put buffer
      setp(gbuf + _str.mSize, gbuf + _str.mCapacity);
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
      _str.mSize += len;

      // resize the underlying Data and reset the input buffer
      _str.resize(((_str.mCapacity+16)*3)/2, true);

      char* gbuf = const_cast<char*>(_str.mBuf);
      // reset the get buffer
      setg(gbuf, gbuf+pos, gbuf+_str.mSize); 
      // reset the put buffer
      setp(gbuf + _str.mSize, gbuf + _str.mCapacity);
   }
   if (c != -1) 
   {
      _str.mBuf[_str.mSize] = c;
      pbump(1);
      return c;
   }
   return 0;
}

iDataStream::iDataStream(Data& str)
   : std::istream(0),
     _streambuf(str)
{
   init(&_streambuf);
}

iDataStream::~iDataStream()
{
   // empty
}

oDataStream::oDataStream(Data& str)
   : std::ostream(0),
     _streambuf(str)
{
   init(&_streambuf);
}

oDataStream::~oDataStream()
{
// empty
}

DataStream::DataStream(Data& str)
   : std::iostream(0),
     _streambuf(str)
{
   init(&_streambuf);
}

DataStream::~DataStream()
{
}
