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

   public:
      DataBuffer(Data& str);
      virtual ~DataBuffer();

   protected:
      virtual int sync();
      virtual int overflow(int c = -1);
};

// To use:
// Data result(4096, true); // size zero, capacity 4096
// DataStream ds(result);
// msg->encode(ds);
//
// result contains the encoded message
// -- may be larger than previously allocated

class DataStream : public std::iostream
{
      // data
      DataBuffer _streambuf;

   public:
      DataStream(Data& str);
      ~DataStream();
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

}

#endif
