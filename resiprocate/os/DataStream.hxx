#ifndef DataStream_hxx
#define DataStream_hxx

#include <iostream>
#include <util/Data.hxx>

namespace Vocal2
{

class DataBuffer : public std::streambuf 
{
      // data
      Data& _str;
      char _buf[128];

   public:
      DataBuffer(Data& str);
      virtual ~DataBuffer();

   protected:
      virtual int sync();
      virtual int overflow(int c = -1);
};

class iDataStream : public std::istream 
{
      // data
      DataBuffer _streambuf;

   public:
      iDataStream(Data& str);
      ~iDataStream();
};

class oDataStream : public std::ostream {
      // data
      DataBuffer _streambuf;

   public:
      oDataStream(Data& str);
      ~oDataStream();
};

class DataStream : public std::iostream
{
      // data
      DataBuffer _streambuf;

   public:
      DataStream(Data& str);
      ~DataStream();
};

}

#endif
