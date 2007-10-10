#include <algorithm>
#include <cassert>
#include <ctype.h>
#include <math.h>

#if defined(HAVE_CONFIG_HXX)
#include "resip/stack/config.hxx"
#endif

#include "rutil/Data.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/vmd5.hxx"
#include "rutil/Coders.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef WIN32
#include "Winsock2.h"
#endif

using namespace resip;
using namespace std;

const Data Data::Empty("", 0);
const Data::size_type Data::npos = UINT_MAX;

Data::PreallocateType::PreallocateType(int)
{}

const Data::PreallocateType Data::Preallocate(0);

const bool Data::isCharHex[256] = 
{
// 0       1       2       3       4       5       6       7       8       9       a   b   c   d   e   f
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //0
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //1
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //2
   true,   true,   true,   true,   true,   true,   true,   true,   true,   true,  false,  false,  false,  false,  false,  false,  //3
   false,   true,   true,   true,   true,   true,   true,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //4
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //5
   false,   true,   true,   true,   true,   true,   true,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //6
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //8
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //9
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //a
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //b
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //c
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //d
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  //e
   false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false,  false   //f
};

//must be lowercase for MD5
static const char hexmap[] = "0123456789abcdef";

int 
hexpair2int(char high, char low)
{
   int val;
    
   switch(high)
   {
      case '0':
         val = 0x00;
         break;
      case '1':
         val = 0x10;
         break;
      case '2':
         val = 0x20;
         break;
      case '3':
         val =  0x30;
         break;
      case '4':
         val =  0x40;
         break;
      case '5':
         val =  0x50;
         break;
      case '6':
         val =  0x60;
         break;
      case '7':
         val =  0x70;
         break;
      case '8':
         val =  0x80;
         break;
      case '9':
         val =  0x90;
         break;
      case 'A':
      case 'a':
         val =  0xA0;
         break;
      case 'B':
      case 'b':
         val =  0xB0;
         break;
      case 'C':
      case 'c':
         val =  0xC0;
         break;
      case 'D':
      case 'd':
         val =  0xD0;
         break;
      case 'E':
      case 'e':
         val =  0xE0;
         break;
      case 'F':
      case 'f':
         val =  0xF0;
         break;
      default:
         return '?';
         break;
   }

   switch(low)
   {
      case '0':
         if (!val)
         {
            return '?';
         }
         break;
      case '1':
         val += 0x01;
         break;
      case '2':
         val += 0x02;
         break;
      case '3':
         val +=  0x03;
         break;
      case '4':
         val +=  0x04;
         break;
      case '5':
         val +=  0x05;
         break;
      case '6':
         val +=  0x06;
         break;
      case '7':
         val +=  0x07;
         break;
      case '8':
         val +=  0x08;
         break;
      case '9':
         val +=  0x09;
         break;
      case 'A':
      case 'a':
         val +=  0x0A;
         break;
      case 'B':
      case 'b':
         val +=  0x0B;
         break;
      case 'C':
      case 'c':
         val +=  0x0C;
         break;
      case 'D':
      case 'd':
         val +=  0x0D;
         break;
      case 'E':
      case 'e':
         val +=  0x0E;
         break;
      case 'F':
      case 'f':
         val +=  0x0F;
         break;
      default:
         return '?';
         break;
   }
   return val;
}

bool
Data::init(DataLocalSize<RESIP_DATA_LOCAL_SIZE> arg)
{
   return true;
}

Data::Data() 
   : mSize(0),
     mBuf(mPreBuffer),
     mCapacity(LocalAlloc),
     mMine(Borrow)
{
   mBuf[mSize] = 0;
}

Data::Data(int capacity,
           const Data::PreallocateType&) 
   : mSize(0),
     mBuf(capacity > LocalAlloc 
          ? new char[capacity + 1]
          : mPreBuffer),
     mCapacity(capacity > LocalAlloc
               ? capacity
               : LocalAlloc),
     mMine(capacity > LocalAlloc ? Take : Borrow)
{
   assert( capacity >= 0 );
   mBuf[mSize] = 0;
}

#ifdef DEPRECATED_PREALLOC
// pre-allocate capacity
Data::Data(int capacity, bool) 
   : mSize(0),
     mBuf(capacity > LocalAlloc 
          ? new char[capacity + 1]
          : mPreBuffer),
     mCapacity(capacity > LocalAlloc
               ? capacity
               : LocalAlloc),
     mMine(capacity > LocalAlloc ? Take : Borrow)
{
   assert( capacity >= 0 );
   mBuf[mSize] = 0;
}
#endif

Data::Data(const char* str, int length) 
   : mSize(length),
     mBuf(mSize > LocalAlloc 
          ? new char[mSize + 1]
          : mPreBuffer),
     mCapacity(mSize > LocalAlloc
               ? mSize
               : LocalAlloc),
     mMine(mSize > LocalAlloc ? Take : Borrow)
{
   if (mSize > 0)
   {
      assert(str);
      memcpy(mBuf, str, mSize);
   }
   mBuf[mSize]=0;
}

Data::Data(const unsigned char* str, int length) 
   : mSize(length),
     mBuf(mSize > LocalAlloc 
          ? new char[mSize + 1]
          : mPreBuffer),
     mCapacity(mSize > LocalAlloc
               ? mSize
               : LocalAlloc),
     mMine(mSize > LocalAlloc ? Take : Borrow)
{
   if (mSize > 0)
   {
      assert(str);
      memcpy(mBuf, str, mSize);
   }
   mBuf[mSize]=0;
}

// share memory KNOWN to be in a surrounding scope
// wears off on, c_str, operator=, operator+=, non-const
// operator[], append, reserve
Data::Data(const char* str, int length, bool) 
   : mSize(length),
     mBuf(const_cast<char*>(str)),
     mCapacity(mSize),
     mMine(Share)
{
   assert(str);
}

Data::Data(ShareEnum se, const char* buffer, int length)
   : mSize(length),
     mBuf(const_cast<char*>(buffer)),
     mCapacity(mSize),
     mMine(se)
{
   assert(buffer);
}

Data::Data(ShareEnum se, const char* buffer)
   : mSize(strlen(buffer)),
     mBuf(const_cast<char*>(buffer)),
     mCapacity(mSize),
     mMine(se)
{
   assert(buffer);
}

Data::Data(ShareEnum se, const Data& staticData)
   : mSize(staticData.mSize),
     mBuf(staticData.mBuf),
     mCapacity(mSize),
     mMine(Share)
{
   // !dlb! maybe:
   // if you are trying to use Take, but make sure that you unset the mMine on
   // the staticData
   assert(se == Share); // makes no sense to call this with 'Take'.
}
//=============================================================================

Data::Data(const char* str) 
   : mSize(str ? strlen(str) : 0),
     mBuf(mSize > LocalAlloc
          ? new char[mSize + 1]
          : mPreBuffer),
     mCapacity(mSize > LocalAlloc
               ? mSize
               : LocalAlloc),
     mMine(mSize > LocalAlloc ? Take : Borrow)
{
   if (str)
   {
      memcpy(mBuf, str, mSize+1);
   }
   else
   {
      mBuf[mSize] = 0;
   }
}

Data::Data(const string& str)
   : mSize(str.size()),
     mBuf(mSize > LocalAlloc
          ? new char[mSize + 1]
          : mPreBuffer),
     mCapacity(mSize > LocalAlloc
               ? mSize
               : LocalAlloc),
     mMine(mSize > LocalAlloc ? Take : Borrow)
{
   memcpy(mBuf, str.c_str(), mSize + 1);
}

Data::Data(const Data& data) 
   : mSize(data.mSize),
     mBuf(mSize > LocalAlloc
          ? new char[mSize + 1]
          : mPreBuffer),
     mCapacity(mSize > LocalAlloc
               ? mSize
               : LocalAlloc),
     mMine(mSize > LocalAlloc ? Take : Borrow)
{
   if (mSize)
   {
      memcpy(mBuf, data.mBuf, mSize);
   }
   mBuf[mSize] = 0;
}

// -2147483646
static const int IntMaxSize = 12;

Data::Data(int val)
   : mSize(0),
     mBuf(IntMaxSize > LocalAlloc 
          ? new char[IntMaxSize + 1]
          : mPreBuffer),
     mCapacity(IntMaxSize > LocalAlloc
               ? IntMaxSize
               : LocalAlloc),
     mMine(IntMaxSize > LocalAlloc ? Take : Borrow)
{
   if (val == 0)
   {
      mBuf[0] = '0';
      mBuf[1] = 0;
      mSize = 1;
      return;
   }

   bool neg = false;
   
   int value = val;
   if (value < 0)
   {
      value = -value;
      neg = true;
   }

   int c = 0;
   int v = value;
   while (v /= 10)
   {
      ++c;
   }

   if (neg)
   {
      ++c;
   }

   mSize = c+1;
   mBuf[c+1] = 0;
   
   v = value;
   while (v)
   {
      mBuf[c--] = '0' + v%10;
      v /= 10;
   }

   if (neg)
   {
      mBuf[0] = '-';
   }
}

static const int MaxLongSize = (sizeof(unsigned long)/sizeof(int))*IntMaxSize;
Data::Data(unsigned long value)
   : mSize(0),
     mBuf(MaxLongSize > LocalAlloc 
          ? new char[MaxLongSize + 1]
          : mPreBuffer),
     mCapacity(MaxLongSize > LocalAlloc
               ? MaxLongSize
               : LocalAlloc),
     mMine(MaxLongSize > LocalAlloc ? Take : Borrow)
{
   if (value == 0)
   {
      mBuf[0] = '0';
      mBuf[1] = 0;
      mSize = 1;
      return;
   }

   int c = 0;
   unsigned long v = value;
   while (v /= 10)
   {
      ++c;
   }

   mSize = c+1;
   mBuf[c+1] = 0;
   
   v = value;
   while (v)
   {
      unsigned int digit = v%10;
      unsigned char d = (char)digit;
      mBuf[c--] = '0' + d;
      v /= 10;
   }
}

#ifndef RESIP_FIXED_POINT
static const int DoubleMaxSize = MaxLongSize + Data::MaxDigitPrecision;
Data::Data(double value, 
           Data::DoubleDigitPrecision precision)
   : mSize(0),
     mBuf(DoubleMaxSize + precision > LocalAlloc 
          ? new char[DoubleMaxSize + precision + 1]
          : mPreBuffer),
     mCapacity(DoubleMaxSize + precision > LocalAlloc
               ? DoubleMaxSize + precision
               : LocalAlloc),
     mMine(DoubleMaxSize + precision > LocalAlloc ? Take : Borrow)
{
   assert(precision >= 0);
   assert(precision < MaxDigitPrecision);

   double v = value;
   bool neg = (value < 0.0);
   
   if (neg)
   {
      v = -v;
   }

   Data m((unsigned long)v);

   // remainder
   v = v - floor(v);

   int p = precision;
   while (p--)
   {
      v *= 10;
   }

   int dec = (int)floor(v+0.5);
   Data d(precision, Data::Preallocate);

   if (dec == 0)
   {
      d = "0";
   }
   else
   {
      d.mBuf[precision] = 0;
      p = precision;
      // neglect trailing zeros
      bool significant = false;
      while (p--)
      {
         if (dec % 10 || significant)
         {
            significant = true;
            ++d.mSize;
            d.mBuf[p] = '0' + (dec % 10);
         }
         else
         {
            d.mBuf[p] = 0;
         }
         
         dec /= 10;
      }
   }

   if (neg)
   {
      mBuf[0] = '-';
      memcpy(mBuf+1, m.mBuf, m.size());
      mBuf[1+m.size()] = '.';
      memcpy(mBuf+1+m.size()+1, d.mBuf, d.size()+1);
      mSize = m.size() + d.size() + 2;
   }
   else
   {
      if (mCapacity < m.size() + d.size() + 1)
      {
         resize(m.size() + d.size() + 1, false);
      }
      memcpy(mBuf, m.mBuf, m.size());
      mBuf[m.size()] = '.';
      memcpy(mBuf+m.size()+1, d.mBuf, d.size()+1);
      mSize = m.size() + d.size() + 1;
   }

   assert(mBuf[mSize] == 0);
}
#endif

Data::Data(unsigned int value)
   : mSize(0),
     mBuf(IntMaxSize > LocalAlloc 
          ? new char[IntMaxSize + 1]
          : mPreBuffer),
     mCapacity(IntMaxSize > LocalAlloc
               ? IntMaxSize
               : LocalAlloc),
     mMine(IntMaxSize > LocalAlloc ? Take : Borrow)
{
   if (value == 0)
   {
      mBuf[0] = '0';
      mBuf[1] = 0;
      mSize = 1;
      return;
   }

   int c = 0;
   unsigned long v = value;
   while (v /= 10)
   {
      ++c;
   }

   mSize = c+1;
   mBuf[c+1] = 0;
   
   v = value;
   while (v)
   {
      unsigned int digit = v%10;
      unsigned char d = (char)digit;
      mBuf[c--] = '0' + d;
      v /= 10;
   }
}

// 18446744073709551615
static const int UInt64MaxSize = 20;

Data::Data(UInt64 value)
   : mSize(0),
     mBuf(UInt64MaxSize > LocalAlloc 
          ? new char[UInt64MaxSize + 1]
          : mPreBuffer),
     mCapacity(UInt64MaxSize > LocalAlloc
               ? UInt64MaxSize
               : LocalAlloc),
     mMine(UInt64MaxSize > LocalAlloc ? Take : Borrow)
{
   if (value == 0)
   {
      mBuf[0] = '0';
      mBuf[1] = 0;
      mSize = 1;
      return;
   }

   int c = 0;
   UInt64 v = value;
   while (v /= 10)
   {
      ++c;
   }

   mSize = c+1;
   mBuf[c+1] = 0;
   
   v = value;
   while (v)
   {
      UInt64 digit = v%10;
      unsigned char d = (char)digit;
      mBuf[c--] = '0' + d;
      v /= 10;
   }
}

static const int CharMaxSize = 1;
Data::Data(char c)
   : mSize(1),
     mBuf(CharMaxSize > LocalAlloc 
          ? new char[CharMaxSize + 1]
          : mPreBuffer),
     mCapacity(CharMaxSize > LocalAlloc
               ? CharMaxSize
               : LocalAlloc),
     mMine(CharMaxSize > LocalAlloc ? Take : Borrow)
{
   mBuf[0] = c;
   mBuf[1] = 0;
}

Data::Data(bool value)
   : mSize(value ? 4 : 5), 
     mBuf(value ? const_cast<char*>("true") : const_cast<char*>("false")),
     mCapacity(value ? 4 : 5),
     mMine(Borrow)
{}

Data::~Data()
{
   if (mMine == Take)
   {
      delete[] mBuf;
   }
}

bool 
resip::operator==(const Data& lhs, const Data& rhs)
{
   if (lhs.mSize != rhs.mSize)
   {
      return false;
   }
   return memcmp(lhs.mBuf, rhs.mBuf, lhs.mSize) == 0;
}

bool 
resip::operator==(const Data& lhs, const char* rhs)
{
   assert(rhs); // .dlb. not consistent with constructor
   if (memcmp(lhs.mBuf, rhs, lhs.mSize) != 0)
   {
      return false;
   }
   else
   {
      // make sure the string terminates at size
      return (rhs[lhs.mSize] == 0);
   }
}

bool
resip::operator<(const Data& lhs, const Data& rhs)
{
   int res = memcmp(lhs.mBuf, rhs.mBuf, resipMin(lhs.mSize, rhs.mSize));

   if (res < 0)
   {
      return true;
   }
   else if (res > 0)
   {
      return false;
   }
   else
   {
      return (lhs.mSize < rhs.mSize);
   }
}

bool
resip::operator<(const Data& lhs, const char* rhs)
{
   assert(rhs);
   Data::size_type l = strlen(rhs);
   int res = memcmp(lhs.mBuf, rhs, resipMin(lhs.mSize, l));

   if (res < 0)
   {
      return true;
   }
   else if (res > 0)
   {
      return false;
   }
   else
   {
      return (lhs.mSize < l);
   }
}

bool
resip::operator<(const char* lhs, const Data& rhs)
{
   assert(lhs);
   Data::size_type l = strlen(lhs);
   int res = memcmp(lhs, rhs.mBuf, resipMin(l, rhs.mSize));

   if (res < 0)
   {
      return true;
   }
   else if (res > 0)
   {
      return false;
   }
   else
   {
      return (l < rhs.mSize);
   }
}

Data& 
Data::operator=(const Data& data)
{
   assert(mBuf);
   
   if (&data != this)
   {
      if (mMine == Share)
      {
         resize(data.mSize, false);
      }
      else
      {
         if (data.mSize > mCapacity)
         {
            resize(data.mSize, false);
         }
      }
      
      mSize = data.mSize;
      // could overlap!
      if (mSize > 0)
      {
         memmove(mBuf, data.mBuf, mSize);
      }
      mBuf[mSize] = 0;
   }
   return *this;
}

size_t
Data::truncate(size_type len)
{
   if (len < mSize)
   {
      (*this)[len] = 0;
      mSize = len;
   }

   return mSize;
}

Data 
Data::operator+(const Data& data) const
{
   Data tmp(mSize + data.mSize, Data::Preallocate);
   tmp.mSize = mSize + data.mSize;
   tmp.mCapacity = tmp.mSize;
   memcpy(tmp.mBuf, mBuf, mSize);
   memcpy(tmp.mBuf + mSize, data.mBuf, data.mSize);
   tmp.mBuf[tmp.mSize] = 0;

   return tmp;
}

Data& 
Data::operator+=(const Data& data)
{
   return append(data.data(), data.size());
}

Data& 
Data::operator+=(const char* str)
{
   assert(str);
   return append(str, strlen(str));
}

Data&
Data::operator^=(const Data& rhs)
{
   if (mCapacity < rhs.mSize)
   {
      resize(rhs.mSize, true);
   }
   if (mSize < rhs.mSize)
   {
      memset(mBuf+mSize, 0, mCapacity - mSize);
   }

   char* c1 = mBuf;
   char* c2 = rhs.mBuf;
   char* end = c2 + rhs.mSize;
   while (c2 != end)
   {
      *c1++ ^= *c2++;
   }
   mSize = resipMax(mSize, rhs.mSize);
   
   return *this;
}

Data&
Data::operator+=(char c)
{
   return append(&c, 1);
}

char& 
Data::operator[](size_type p)
{
   assert(p < mSize);
   own();
   return mBuf[p];
}


char& 
Data::at(size_type p)
{
   if (p >= mCapacity)
   {
      resize(p+1, true);
   }
   else
   {
      own();
      if (p > mSize)
      {
         mSize = p + 1;
         mBuf[mSize] = 0;
      }
   }
   return mBuf[p];
}

char 
Data::operator[](size_type p) const
{
   assert(p < mSize);
   return mBuf[p];
}

Data& 
Data::operator=(const char* str)
{
   assert(str);
   size_type l = strlen(str);

   if (mMine == Share)
   {
      resize(l, false);
   }
   else
   {
      if (l > mCapacity)
      {
         resize(l, false);
      }
   }
      
   mSize = l;
   // could conceivably overlap
   memmove(mBuf, str, mSize+1);

   return *this;
}

Data 
Data::operator+(const char* str) const
{
   assert(str);
   size_t l = strlen(str);
   Data tmp(mSize + l, Data::Preallocate);
   tmp.mSize = mSize + l;
   tmp.mCapacity = tmp.mSize;
   memcpy(tmp.mBuf, mBuf, mSize);
   memcpy(tmp.mBuf + mSize, str, l+1);

   return tmp;
}

void
Data::reserve(size_type len)
{
   if (len > mCapacity)
   {
      resize(len, true);
   }
}

Data&
Data::append(const char* str, size_type len)
{
   assert(str);
   if (mCapacity < mSize + len)
   {
      // .dlb. pad for future growth?
      resize(((mSize + len +16)*3)/2, true);
   }
   else
   {
      if (mMine == Share)
      {
         resize(mSize + len, true);
      }
   }

   // could conceivably overlap
   memmove(mBuf + mSize, str, len);
   mSize += len;
   mBuf[mSize] = 0;

   return *this;
}


Data
Data::operator+(char c) const
{
   Data tmp(mSize + 1, Data::Preallocate);
   tmp.mSize = mSize + 1;
   tmp.mCapacity = tmp.mSize;
   memcpy(tmp.mBuf, mBuf, mSize);
   tmp.mBuf[mSize] = c;
   tmp.mBuf[mSize+1] = 0;

   return tmp;
}

const char* 
Data::c_str() const
{
   if (mMine == Data::Share || mSize == mCapacity)
   {
      const_cast<Data*>(this)->resize(mSize+1,true);
   }
   // mostly is zero terminated, but not by DataStream
   mBuf[mSize] = 0;
   return mBuf;
}

const char* 
Data::data() const
{
   return mBuf;
}

const char* 
Data::begin() const
{
   return mBuf;
}

const char* 
Data::end() const
{
   return mBuf + mSize;
}


void
Data::own() const
{
   if (mMine == Share)
   {
      const_cast<Data*>(this)->resize(mSize, true);
   }
}

// generate additional capacity
void
Data::resize(size_type newCapacity, 
             bool copy)
{
   assert(newCapacity >= mCapacity || mMine == Data::Share);

   char *oldBuf = mBuf;
   mBuf = new char[newCapacity+1];
   if (copy)
   {
      memcpy(mBuf, oldBuf, mSize);
      mBuf[mSize] = 0;
   }
   if (mMine == Take)
   {
      delete[] oldBuf;
   }
   mMine = Take;
   mCapacity = newCapacity;
}

Data
Data::md5() const
{
   MD5Context context;
   MD5Init(&context);
   MD5Update(&context, reinterpret_cast < unsigned const char* > (mBuf), mSize);

   unsigned char digestBuf[16];
   MD5Final(digestBuf, &context);
   Data digest(digestBuf,16);
   Data ret = digest.hex();
   
   return ret;
}

Data 
Data::escaped() const
{ 
   Data ret((int)((size()*11)/10), Data::Preallocate);

   const char* p = data();
   for (size_type i=0; i < size(); ++i)
   {
      unsigned char c = *p++;

      if ( c == 0x0d )
      {
         if ( i+1 < size() )
         {
            if ( *p == 0x0a )
            {
               // found a CRLF sequence
               ret += c;
               c = *p++; i++;
               ret += c;
               continue;
            }
         }
      }
      
      if ( !isprint(c) )
      {
         ret +='%';
         
         int hi = (c & 0xF0)>>4;
         int low = (c & 0x0F);
	   
         ret += hexmap[hi];
         ret += hexmap[low];
      }
      else
      {
         ret += c;
      }
   }

   return ret;
}

Data 
Data::charEncoded() const
{ 
   Data ret((int)((size()*11)/10), Data::Preallocate);

   const char* p = data();
   for (size_type i=0; i < size(); ++i)
   {
      unsigned char c = *p++;

      if ( c == 0x0d )
      {
         if ( i+1 < size() )
         {
            if ( *p == 0x0a )
            {
               // found a CRLF sequence
               ret += c;
               c = *p++; i++;
               ret += c;
               continue;
            }
         }
      }
      
      if ( !isprint(c) ||
           // rfc 3261 reserved + mark + space + tab
           strchr(" \";/?:@&=+%$,\t-_.!~*'()", c))
      {
         ret +='%';
         
         int hi = (c & 0xF0)>>4;
         int low = (c & 0x0F);
	   
         ret += hexmap[hi];
         ret += hexmap[low];
      }
      else
      {
         ret += c;
      }
   }

   return ret;
}

Data
Data::charUnencoded() const
{
   Data ret(size(), Data::Preallocate);

   const char* p = data();
   for (size_type i = 0; i < size(); ++i)
   {
      unsigned char c = *p++;
      if (c == '%')
      {
         if ( i+2 < size())
         {
            const char* high = strchr(hexmap, *p++);
            const char* low = strchr(hexmap, *p++);

            // !rwm! changed from high==0 || low==0
            if (high == 0 && low == 0)
            {
               assert(0);
               // ugh
               return ret;
            }
            
            int highInt = high - hexmap;
            int lowInt = low - hexmap;
            ret += char(highInt<<4 | lowInt);
            i += 2;
         }
         else
         {
            break;
         }
      }
      else
      {
         ret += c;
      }
   }
   return ret;
}

Data
Data::urlEncoded() const
{
   Data buffer;
   DataStream strm(buffer);
   urlEncode(strm);
   strm.flush();
   return buffer;
}

Data
Data::urlDecoded() const
{
   Data buffer;
   DataStream strm(buffer);
   urlDecode(strm);
   strm.flush();
   return buffer;
}

std::ostream&
Data::urlDecode(std::ostream& s) const
{
   unsigned int i = 0;
   for (const char* p = data(); p != data()+size(); ++p, ++i)
   {
      unsigned char c = *p;
      if (c == '%')
      {
         if (i+2 < size())
         {
            s << (char) hexpair2int( *(p+1), *(p+2));
            p += 2;
         }
         else
         {
            break;
         }
      }
      else if (c == '+')
      {
         s << ' ';
      }
      else
      {
         s << c;
      }
   }
   return s;
}

bool urlNonEncodedChars[256] = {false};
bool 
urlNonEncodedCharsInitFn()
{
   // query part of HTTP URL can be a pchar, slash, or question
   // pchar is unreserved, subdelims, colon, at-sign

   for (int i = 0; i < 256; ++i)
   {
      unsigned char c(i);
      urlNonEncodedChars[c] = (isalpha(c) ||  // unreserved
                               isdigit(c) ||  // unreserved
                               c == '-' ||  // these first 4 are unreserved
                               c == '_' ||
                               c == '.' ||
                               c == '~' ||
                               c == '!' ||  // these are subdelims (allowed in pchars)
                               c == '$' ||
//                               c == '&' ||  // while these are allowed subdelims, this is an error
//                               c == '+' ||  // I believe this is an error as well.
                               c == '\'' ||
                               c == '(' ||
                               c == ')' ||
                               c == '*' ||
                               c == ',' ||
                               c == ';' ||
                               c == '=' ||
                               c == ':' ||   // next two explicitly allowed in pchar
                               c == '@' ||   
                               c == '/' ||   // next two explicitly allowed in query in addition to pchar
                               c == '?');    
   }

   return false;
}

static bool dummy = urlNonEncodedCharsInitFn();

// e.g. http://www.blooberry.com/indexdot/html/topics/urlencoding.htm
std::ostream&
Data::urlEncode(std::ostream& s) const
{
   for (const char* p = data(); p != data() + size(); ++p)
   {
      unsigned char c = *p;

      // pchar, slash, questionmark
      // pchar = unreserved, sub-delims, colon, at-sign
      // unreserved = alphanum, hyphen, underscore, period, tilde
      // subdelims = bang (!), dollar, ampersand (oops!), plus, single-quote, lparen, rparen, asterisk, comma, semicolon, equal
      //if (isalpha(c) || isdigit(c) || strchr("-_.~!$+'()*,;=:@/?", c)) //
      // strchr is inefficient and wrong
      if (urlNonEncodedChars[c])
      {
         s << c;
      }
      else
      {
         if (c == 0x20)
         {
            s << '+';
         }
         else
         {
            s << '%' << (hexmap[(c & 0xF0)>>4]) << (hexmap[(c & 0x0F)]);
         }
      }
   }

   return s;
}

Data
Data::xmlCharDataEncode() const
{
   Data buffer;
   DataStream strm(buffer);
   xmlCharDataEncode(strm);
   strm.flush();
   return buffer;
}

Data
Data::xmlCharDataDecode() const
{
   Data buffer;
   DataStream strm(buffer);
   xmlCharDataDecode(strm);
   strm.flush();
   return buffer;
}

// http://www.w3.org/TR/REC-xml/#syntax
std::ostream&
Data::xmlCharDataEncode(std::ostream& s) const
{
   for (const char* p = data(); p != data() + size(); ++p)
   {
      unsigned char c = *p;

      switch(c)
      {
      case '&':
         s << "&amp;";
         break;
      case '<':
         s << "&lt;";
         break;
      case '>':
         s << "&gt;";
         break;
      case '\'':
         s << "&apos;";
         break;
      case '\"':
         s << "&quot;";
         break;
      default:
         s << c;
      }
   }

   return s;
}

std::ostream&
Data::xmlCharDataDecode(std::ostream& s) const
{
   unsigned int i = 0;
   for (const char* p = data(); p != data()+size(); ++p, ++i)
   {
      unsigned char c = *p;
      if (c == '&')
      {
         // look for amp;
         if(i+4 < size() && 
            *(p+1) == 'a' && *(p+2) == 'm' && *(p+3) == 'p' && *(p+4) == ';')
         {
            s << '&';
            p += 4;
         }
         // look for lt;
         else if(i+3 < size() && 
            *(p+1) == 'l' && *(p+2) == 't' && *(p+3) == ';')
         {
            s << '<';
            p += 3;
         }
         // look for gt;
         else if(i+3 < size() &&
            *(p+1) == 'g' && *(p+2) == 't' && *(p+3) == ';')
         {
            s << '>';
            p += 3;
         }
         // look for apos;
         else if(i+5 < size() && 
            *(p+1) == 'a' && *(p+2) == 'p' && *(p+3) == 'o' && *(p+4) == 's' && *(p+5) == ';')
         {
            s << '\'';
            p += 5;
         }
         // look for quot;
         else if(i+5 < size() && 
            *(p+1) == 'q' && *(p+2) == 'u' && *(p+3) == 'o' && *(p+4) == 't' && *(p+5) == ';')
         {
            s << '\"';
            p += 5;
         }
         else // if no conversion found - just leave characters in data
         {
            s << c;
         }
      }
      else
      {
         s << c;
      }
   }
   return s;
}

Data
Data::trunc(size_t s) const
{
   if (size() <= s)
   {
      return *this;
   }
   else
   {
      return Data(data(), s) + "..";
   }
}

Data
Data::hex() const
{
   Data ret( 2*mSize, Data::Preallocate);

   const char* p = mBuf;
   char* r = ret.mBuf;
   for (size_type i=0; i < mSize; ++i)
   {
      unsigned char temp = *p++;

      int hi = (temp & 0xf0)>>4;
      int low = (temp & 0xf);

      *r++ = hexmap[hi];
      *r++ = hexmap[low];
   }
   *r = 0;
   ret.mSize = 2*mSize;
   return ret;
}

Data&
Data::lowercase()
{
   own();
   char* p = mBuf;
   for (size_type i=0; i < mSize; ++i)
   {
      *p = tolower(*p);
      ++p;
   }
   return *this;
}

Data&
Data::uppercase()
{
   own();
   char* p = mBuf;
   for (size_type i=0; i < mSize; ++i)
   {
      *p = toupper(*p);
      ++p;
   }
   return *this;
}

Data&
Data::clear()
{
   mSize = 0;
   return *this;
}

int 
Data::convertInt() const
{
   int val = 0;
   char* p = mBuf;
   const char* const end = mBuf + mSize;
   int s = 1;

   for (; p != end; ++p)
   {
      if (!isspace(*p))
      {
         goto sign_char;
      }
   }
   return val;
sign_char:

   if (*p == '-')
   {
      s = -1;
      ++p;
   }
   else if (*p == '+')
   {
      ++p;
   }

   for(; p != end; ++p)
   {
      if (!isdigit(*p))
      {
         break;
      }
      val *= 10;
      val += (*p) - '0';
   }
   return s*val;
}

unsigned long
Data::convertUnsignedLong() const
{
   unsigned long val = 0;
   char* p = mBuf;
   const char* const end = mBuf + mSize;

   for (; p != end; ++p)
   {
      if (!isspace(*p))
      {
         goto sign_char;
      }
   }
   return val;
sign_char:

   if (*p == '+')
   {
      ++p;
   }

   for(; p != end; ++p)
   {
      if (!isdigit(*p))
      {
         break;
      }
      val *= 10;
      val += (*p) - '0';
   }
   return val;
}

UInt64
Data::convertUInt64() const
{
   UInt64 val = 0;
   char* p = mBuf;
   const char* const end = mBuf + mSize;

   for (; p != end; ++p)
   {
      if (!isspace(*p))
      {
         goto sign_char;
      }
   }
   return val;
sign_char:

   if (*p == '+')
   {
      ++p;
   }

   for(; p != end; ++p)
   {
      if (!isdigit(*p))
      {
         break;
      }
      val *= 10;
      val += (*p) - '0';
   }
   return val;
}

size_t
Data::convertSize() const
{
   size_t val = 0;
   char* p = mBuf;
   const char* const end = mBuf + mSize;

   for (; p != end; ++p)
   {
      if (!isspace(*p))
      {
         goto sign_char;
      }
   }
   return val;
sign_char:

   if (*p == '+')
   {
      ++p;
   }

   for(; p != end; ++p)
   {
      if (!isdigit(*p))
      {
         break;
      }
      val *= 10;
      val += (*p) - '0';
   }
   return val;
}

#ifndef RESIP_FIXED_POINT
double 
Data::convertDouble() const
{
   long val = 0;
   char* p = mBuf;
   const char* const end = p + mSize;
   int s = 1;

   for (; p != end; ++p)
   {
      if (!isspace(*p))
      {
         goto sign_char;
      }
   }
   return val;
sign_char:

   if (*p == '-')
   {
      s = -1;
      ++p;
   }
   else if (*p == '+')
   {
      ++p;
   }

   for(; p != end; ++p)
   {
      if (*p == '.')
      {
         goto decimals;
      }
      if (!isdigit(*p))
      {
         return s*val;
      }
      val *= 10;
      val += (*p) - '0';
   }
   return s*val;

decimals:
   ++p;
   long d = 0;
   double div = 1.0;
   for(; p != end; ++p)
   {
      if (!isdigit(*p)) 
      {
         break;
      }
      d *= 10;
      d += *p - '0';
      div *= 10;
   }
   return s*(val + d/div);
}
#endif

bool
Data::prefix(const Data& pre) const
{
   if (pre.size() > size())
   {
      return false;
   }

   return memcmp(data(), pre.data(), pre.size()) == 0;
}

bool
Data::postfix(const Data& post) const
{
   if (post.size() > size())
   {
      return false;
   }

   return memcmp(data() + (size()-post.size()), post.data(), post.size()) == 0;
}

Data 
Data::substr(size_type first, size_type count) const
{
   assert(first <= mSize);
   if ( count == Data::npos)
   {
      return Data(mBuf+first, mSize-first);
   }
   else
   {
      assert(first + count <= mSize);
      return Data(mBuf+first, count);
   }
}

Data::size_type
Data::find(const Data& match, 
           size_type start) const
{
   if (start >= mSize) 
   {
      return Data::npos;
   }
   else
   {
      ParseBuffer pb(mBuf+start, mSize-start);
      pb.skipToChars(match);
      if (pb.eof()) 
      {
         return Data::npos;
      }
      else
      {
         return pb.position() - pb.start() + start;
      }
   }
}

int
Data::replace(const Data& match, 
              const Data& replaceWith)
{
   assert(!match.empty());

   int count = 0;

   const int incr = replaceWith.size() - match.size();
   for (size_type offset = find(match, 0); 
        offset != Data::npos; 
        offset = find(match, offset+replaceWith.size()))
   {
      if (mSize + incr >= mCapacity)
      {
         resize((mCapacity + incr) * 3 / 2, true);
      }
      else
      {
         own();
      }

      // move the memory forward (or backward)
      memmove(mBuf + offset + replaceWith.size(), mBuf + offset + match.size(), mSize - offset - match.size());
      memcpy(mBuf + offset, replaceWith.data(), replaceWith.size());
      mSize += incr;

      ++count;
   }

   return count;
}

ostream& 
resip::operator<<(ostream& strm, const Data& d)
{
   return strm.write(d.mBuf, d.mSize);
}

// random permutation of 0..255
static const unsigned char randomPermutation[256] = 
{
   44, 9, 46, 184, 21, 30, 92, 231, 79, 7, 166, 237, 173, 72, 91, 123, 
   212, 183, 16, 99, 85, 45, 190, 130, 118, 107, 169, 119, 100, 179, 251, 177,
   23, 125, 12, 101, 121, 246, 61, 38, 156, 114, 159, 57, 181, 145, 198, 182,
   58, 215, 174, 225, 82, 178, 150, 161, 63, 103, 32, 203, 68, 151, 139, 55, 
   143, 2, 36, 110, 209, 154, 204, 89, 62, 17, 187, 226, 31, 105, 195, 208,
   49, 56, 238, 172, 37, 3, 234, 206, 134, 233, 19, 148, 64, 4, 10, 224,
   144, 88, 93, 191, 20, 131, 138, 199, 243, 244, 39, 50, 214, 87, 6, 84,
   185, 112, 171, 75, 192, 193, 239, 69, 106, 43, 194, 1, 78, 67, 116, 200,
   83, 70, 213, 25, 59, 137, 52, 13, 153, 42, 232, 0, 133, 210, 76, 33,
   255, 236, 124, 104, 65, 201, 53, 155, 140, 254, 54, 196, 120, 146, 216, 29,
   28, 86, 245, 90, 98, 26, 81, 115, 180, 66, 102, 136, 167, 51, 109, 132,
   77, 175, 14, 202, 222, 48, 223, 188, 40, 242, 157, 5, 128, 229, 71, 127,
   164, 207, 247, 8, 80, 149, 94, 160, 47, 117, 135, 176, 129, 142, 189, 97,
   11, 250, 221, 218, 96, 220, 35, 197, 152, 126, 219, 74, 170, 252, 163, 41,
   95, 27, 34, 22, 205, 230, 241, 186, 168, 228, 253, 249, 113, 108, 111, 211,
   235, 217, 165, 122, 15, 141, 158, 147, 240, 24, 162, 18, 60, 73, 227, 248
};

size_t
Data::rawHash(const unsigned char* c, size_t size)
{
   // 4 byte Pearson's hash
   // essentially random hashing

   union 
   {
         size_t st;
         unsigned char bytes[4];
   };
   st = 0; // suppresses warnings about unused st
   bytes[0] = randomPermutation[0];
   bytes[1] = randomPermutation[1];
   bytes[2] = randomPermutation[2];
   bytes[3] = randomPermutation[3];

   const unsigned char* end = c + size;
   for ( ; c != end; ++c)
   {
      bytes[0] = randomPermutation[*c ^ bytes[0]];
      bytes[1] = randomPermutation[*c ^ bytes[1]];
      bytes[2] = randomPermutation[*c ^ bytes[2]];
      bytes[3] = randomPermutation[*c ^ bytes[3]];
   }

   // convert from network to host byte order
   return ntohl(st);
}

// use only for ascii characters!
size_t 
Data::rawCaseInsensitiveHash(const unsigned char* c, size_t size)
{

   union 
   {
         size_t st;
         unsigned char bytes[4];
   };
   st = 0; // suppresses warnings about unused st
   bytes[0] = randomPermutation[0];
   bytes[1] = randomPermutation[1];
   bytes[2] = randomPermutation[2];
   bytes[3] = randomPermutation[3];

   const unsigned char* end = c + size;
   for ( ; c != end; ++c)
   {
      unsigned char cc = tolower(*c);
      bytes[0] = randomPermutation[cc ^ bytes[0]];
      bytes[1] = randomPermutation[cc ^ bytes[1]];
      bytes[2] = randomPermutation[cc ^ bytes[2]];
      bytes[3] = randomPermutation[cc ^ bytes[3]];
   }

   // convert from network to host byte order
   return ntohl(st);
}

Data
bits(size_t v)
{
   Data ret;
   for (unsigned int i = 0; i < 8*sizeof(size_t); ++i)
   {
      ret += ('0' + v%2);
      v /= 2;
   }

   return ret;
}

size_t
Data::hash() const
{
   return rawHash((const unsigned char*)(this->data()), this->size());
}

size_t
Data::caseInsensitivehash() const
{
   return rawCaseInsensitiveHash((const unsigned char*)(this->data()), this->size());
}

HashValueImp(resip::Data, data.hash());

Data 
Data::base64decode() const
{
   // see RFC 3548 
   // this will decode normal and URL safe alphabet 
#if 0
   return Base64Coder::decode( *this );
#else
   static signed char base64Lookup[128] = 
      {
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 
         -1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 
         -1,-1,-1,62,-1,62,-2,63,52,53, 
         54,55,56,57,58,59,60,61,-1,-1, 
         -1,-2,-1,-1,-1,0, 1, 2, 3, 4,
         5, 6, 7, 8, 9, 10,11,12,13,14, 
         15,16,17,18,19,20,21,22,23,24, 
         25,-1,-1,-1,-1,63,-1,26,27,28, 
         29,30,31,32,33,34,35,36,37,38, 
         39,40,41,42,43,44,45,46,47,48, 
         49,50,51,-1,-1,-1,-1,-1            
      };

   int wc=0;
   int val=0;
   Data bin;
   bin.reserve( size()*3/4 );
   
   for( unsigned int i=0; i<size(); i++ )
   {
      unsigned int x = mBuf[i] & 0x7F;
      char c1,c2,c3;
      
      int v =  base64Lookup[x];

      if ( v >= 0 )
      {
         val = val << 6;
         val |= v;
         wc++;
         
         if ( wc == 4 )
         {
            c3 = char( val & 0xFF ); val = val >> 8;
            c2 = char( val & 0xFF ); val = val >> 8;
            c1 = char( val & 0xFF ); val = val >> 8;

            bin += c1;
            bin += c2;
            bin += c3;
            
            wc=0;
            val=0;
         }
      }
      if ( base64Lookup[x] == -2 )
      {
         if (wc==2) val = val<<12;
         if (wc==3) val = val<<6;
         
         c3 = char( val & 0xFF ); val = val >> 8;
         c2 = char( val & 0xFF ); val = val >> 8;
         c1 = char( val & 0xFF ); val = val >> 8;
         
         unsigned int xNext = mBuf[i] & 0x7F;
         if ( (i+1<size() ) && ( base64Lookup[xNext] == -2 ))
         {
            bin += c1;
            i++;
         }
         else
         {
            bin += c1;
            bin += c2;
         }

         break;
      }
   }

   return bin;
#endif
}


Data 
Data::base64encode(bool useSafeSet) const
{
   // see RFC 3548 
   static unsigned char codeCharUnsafe[] = 
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
   static unsigned char codeCharSafe[] = 
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
   
   unsigned char* codeChar = useSafeSet ? codeCharSafe : codeCharUnsafe;
   
   int srcLength = this->size();
   unsigned int dstLimitLength = srcLength*4/3 + 1 + 2; // +2 for the == chars
   unsigned char * dstData = new unsigned char[dstLimitLength];
   unsigned int dstIndex = 0;
   
   const char * p = static_cast<const char *>( this->data() );
   
   for(int index=0;index<srcLength;index+=3)
   {
      unsigned char codeBits = (p[index] & 0xfc)>>2;
      
      assert(codeBits < 64);
      dstData[dstIndex++] = codeChar[codeBits]; // c0 output
      assert(dstIndex <= dstLimitLength);
      
      // do second codeBits
      codeBits = ((p[index]&0x3)<<4);
      if (index+1 < srcLength)
      {
         codeBits |= ((p[index+1]&0xf0)>>4);
      }
      assert(codeBits < 64);
      dstData[dstIndex++] = codeChar[codeBits]; // c1 output
      assert(dstIndex <= dstLimitLength);
      
      if (index+1 >= srcLength) 
      {
         dstData[dstIndex++] = codeChar[64];
         assert(dstIndex <= dstLimitLength);
         dstData[dstIndex++] = codeChar[64];
         assert(dstIndex <= dstLimitLength);
         break; // encoded d0 only
      }
      
      // do third codeBits
      codeBits = ((p[index+1]&0xf)<<2);
      if (index+2 < srcLength)
      {
         codeBits |= ((p[index+2]&0xc0)>>6);
      }
      assert(codeBits < 64);
      dstData[dstIndex++] = codeChar[codeBits]; // c2 output
      assert(dstIndex <= dstLimitLength);
      
      if (index+2 >= srcLength) 
      {
         dstData[dstIndex++] = codeChar[64];   
         assert(dstIndex <= dstLimitLength);
         break; // encoded d0 d1 only
      }
      
      // do fourth codeBits
      codeBits = ((p[index+2]&0x3f));
      assert(codeBits < 64);
      dstData[dstIndex++] = codeChar[codeBits]; // c3 output
      assert(dstIndex <= dstLimitLength);
      // outputed all d0,d1, and d2
   }

   return Data(Data::Take, reinterpret_cast<char*>(dstData),
               dstIndex);
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the

 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

