#include "sip2/util/MD5Stream.hxx"

using namespace Vocal2;

MD5Stream::MD5Stream()
{
   MD5Init(&mContext);
}

Data 
MD5Stream::getHex()
{
   unsigned char digestBuf[16];
   MD5Final(digestBuf, &mContext);
   Data digest((const char*)digestBuf,16, false);
   return digest.hex();   
}

Data 
MD5Stream::get()
{
   unsigned char digestBuf[16];
   MD5Final(digestBuf, &mContext);
   Data digest((const char*)digestBuf,16, false);
   return digest;
}

MD5Stream&
MD5Stream::write(const char* s, size_t size)
{
   MD5Update(&mContext, reinterpret_cast <unsigned const char*>(s), size);
   return *this;
}

MD5Stream& 
Vocal2::operator<<(MD5Stream& strm, const Data& d)
{
   return strm.write(d.data(), d.size());
}

MD5Stream& 
Vocal2::operator<<(MD5Stream& strm, const char* s)
{
   return strm.write(s, strlen(s));
}
