#include <util/DataStream.hxx>

using namespace Vocal2;

DataBuffer::DataBuffer(Data& str)
   : _str(str)
{
   char* gbuf = const_cast<char*>(_str.data());
   setg(gbuf, gbuf, gbuf+_str.size());
   setp(_buf, _buf + sizeof(_buf));
}

DataBuffer::~DataBuffer()
{
}

int
DataBuffer::sync()
{
   size_t l = pptr() - pbase();
   if (l > 0) 
   {
      size_t pos = gptr() - eback();  // remember the get position
      _str.append(_buf, l);
      setp(_buf, _buf+sizeof(_buf));
      char* gbuf = const_cast<char*>(_str.data());
      setg(gbuf, gbuf+pos, gbuf+_str.size()); // reset the get buffer
   }
   return 0;
}

int
DataBuffer::overflow(int c)
{
   sync();
   if (c != -1) 
   {
      *_buf = c;
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
