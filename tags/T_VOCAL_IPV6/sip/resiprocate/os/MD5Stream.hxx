#ifndef MD5Stream_hxx
#define MD5Stream_hxx

#include "sip2/util/Data.hxx"
#include "sip2/util/vmd5.hxx"

namespace Vocal2
{

class MD5Stream
{
   public:
      MD5Stream();
      
      Data getHex();
      Data get();

      MD5Stream& write(const char* s, size_t size);
   private:
      MD5Context mContext;
};

MD5Stream& operator<<(MD5Stream& strm, const Data& d);

MD5Stream& operator<<(MD5Stream& strm, const char* s);

 
}


#endif
