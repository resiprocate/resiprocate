// "$Id: Data.cxx,v 1.75 2003/10/03 01:52:42 jason Exp $";

#include <algorithm>
#include <cassert>
#include <ctype.h>
#include <math.h>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/vmd5.hxx"


using namespace resip;
using namespace std;

const Data Data::Empty("", 0);
const int Data::npos = INT_MAX;

char* 
Data::initializeHack()
{
   assert(Data::Empty.mBuf == 0);
   static char buffer[1];

   return buffer;
}

Data::Data() 
   : mSize(0),
     mBuf(Data::Empty.mBuf ? Data::Empty.mBuf : initializeHack()),
     mCapacity(mSize),
     mMine(false)
{
}

// pre-allocate capacity
Data::Data(int capacity, bool) 
   : mSize(0),
     mBuf(new char[capacity + 1]),
     mCapacity(capacity),
     mMine(true)
{
   assert( capacity >= 0 );
   mBuf[0] = 0;
}

Data::Data(const char* str, int length) 
   : mSize(length),
     mBuf(new char[length+1]),
     mCapacity(length),
     mMine(true)
{
   assert( length >= 0 );
   if ( length > 0 )
   {
      assert(str);
      memcpy(mBuf, str, mSize);
      mBuf[mSize]=0;
   }
}

Data::Data(const unsigned char* str, int length) 
   : mSize(length),
     mBuf(new char[mSize+1]),
     mCapacity(mSize),
     mMine(true)
{
   assert(str);
   memcpy(mBuf, str, mSize);
     mBuf[mSize]=0;
}

// share memory KNOWN to be in a surrounding scope
// wears off on, c_str, operator=, operator+=, non-const
// operator[], append, reserve
Data::Data(const char* str, int length, bool) 
   : mSize(length),
     mBuf(const_cast<char*>(str)),
     mCapacity(mSize),
     mMine(false)
{
   assert(str);
}

Data::Data(ShareEnum, const char* buffer, int length)
   : mSize(length),
     mBuf(const_cast<char*>(buffer)),
     mCapacity(mSize),
     mMine(false)
{
   assert(buffer);
}

Data::Data(ShareEnum, const char* buffer)
   : mSize(strlen(buffer)),
     mBuf(const_cast<char*>(buffer)),
     mCapacity(mSize),
     mMine(false)
{
   assert(buffer);
}

Data::Data(ShareEnum, const Data& staticData)
   : mSize(staticData.mSize),
     mBuf(staticData.mBuf),
     mCapacity(mSize),
     mMine(false)
{}
//=============================================================================

Data::Data(const char* str) 
   : mSize(str ? strlen(str) : 0),
     mBuf(new char[mSize + 1]),
     mCapacity(mSize),
     mMine(true)
{
   assert(str);
   memcpy(mBuf, str, mSize+1);
}

Data::Data(const string& str) : 
   mSize(str.size()),
   mBuf(new char[mSize + 1]),
   mCapacity(mSize),
   mMine(true)
{
   memcpy(mBuf, str.c_str(), mSize + 1);
}

Data::Data(const Data& data) 
   : mSize(data.mSize),
     mBuf(mSize ? new char[mSize+1] : Empty.mBuf),
     mCapacity(mSize),
     mMine(mSize != 0)
{
   if (mSize)
   {
      memcpy(mBuf, data.mBuf, mSize);
      mBuf[mSize] = 0;
   }
}

Data::~Data()
{
   if (mMine)
   {
      delete[] mBuf;
   }
}

Data::Data(int val)
   : mSize(0),
     mBuf(0),
     mCapacity(0),
     mMine(true)
{
   if (val == 0)
   {
      mBuf = new char[2];
      mBuf[0] = '0';
      mBuf[1] = 0;
      mSize = 1;
      mCapacity = mSize;
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
      c++;
   }

   if (neg)
   {
      c++;
   }

   mSize = c+1;
   mCapacity = mSize;
   mBuf = new char[c+2];
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

Data::Data(double value, int precision)
   : mSize(0), 
     mBuf(0),
     mCapacity(0), 
     mMine(true)
{
   assert(precision < 10);

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

   Data d;

   if (dec == 0)
   {
      d = "0";
   }
   else
   {
      d.resize(precision, false);
      d.mBuf[precision] = 0;
      p = precision;
      // neglect trailing zeros
      bool significant = false;
      while (p--)
      {
         if (dec % 10 || significant)
         {
            significant = true;
            d.mSize++;
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
      resize(m.size() + d.size() + 2, false);
      mBuf[0] = '-';
      memcpy(mBuf+1, m.mBuf, m.size());
      mBuf[1+m.size()] = '.';
      memcpy(mBuf+1+m.size()+1, d.mBuf, d.size()+1);
      mSize = m.size() + d.size() + 2;
   }
   else
   {
      resize(m.size() + d.size() + 1, false);
      memcpy(mBuf, m.mBuf, m.size());
      mBuf[m.size()] = '.';
      memcpy(mBuf+m.size()+1, d.mBuf, d.size()+1);
      mSize = m.size() + d.size() + 1;
   }
}

Data::Data(unsigned long value)
   : mSize(0), 
     mBuf(0),
     mCapacity(0),
     mMine(true)
{
   if (value == 0)
   {
      mBuf = new char[2];
      mBuf[0] = '0';
      mBuf[1] = 0;
      mSize = 1;
      return;
   }

   int c = 0;
   unsigned long v = value;
   while (v /= 10)
   {
      c++;
   }

   mSize = c+1;
   mCapacity = c+1;
   mBuf = new char[c+2];
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

Data::Data(unsigned int value)
   : mSize(0), 
     mBuf(0),
     mCapacity(0),
     mMine(true)
{
   if (value == 0)
   {
      mBuf = new char[2];
      mBuf[0] = '0';
      mBuf[1] = 0;
      mSize = 1;
      return;
   }

   int c = 0;
   unsigned long v = value;
   while (v /= 10)
   {
      c++;
   }

   mSize = c+1;
   mCapacity = c+1;
   mBuf = new char[c+2];
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


Data::Data(char c)
   : mSize(1), 
     mBuf(0),
     mCapacity(mSize),
     mMine(true)
{
   mBuf = new char[2];
   mBuf[0] = c;
   mBuf[1] = 0;
}

Data::Data(bool value)
   : mSize(0), 
     mBuf(0),
     mCapacity(0),
     mMine(false)
{
   static char truec[] = "true";
   static char falsec[] = "false";

   if (value)
   {
      mBuf = truec;
      mSize = 4;
      mCapacity = 4;
   }
   else
   {
      mBuf = falsec;
      mSize = 5;
      mCapacity = 5;
   }
}

bool 
Data::operator==(const Data& rhs) const
{
   if (mSize != rhs.mSize)
   {
      return false;
   }
   return strncmp(mBuf, rhs.mBuf, mSize) == 0;
}

bool 
Data::operator==(const char* rhs) const
{
   assert(rhs);
   if (strncmp(mBuf, rhs, mSize) != 0)
   {
      return false;
   }
   else
   {
      // make sure the string terminates at size
      return rhs[mSize] == 0;
   }
}

bool
Data::operator<(const Data& rhs) const
{
   int res = strncmp(mBuf, rhs.mBuf, resipMin(mSize, rhs.mSize));

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
      return (mSize < rhs.mSize);
   }
}

bool
Data::operator<=(const Data& rhs) const
{
   return !(*this > rhs);
}

bool
Data::operator<(const char* rhs) const
{
   assert(rhs);
   size_type l = strlen(rhs);
   int res = strncmp(mBuf, rhs, resipMin(mSize, l));

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
      return (mSize < l);
   }
}

bool
Data::operator<=(const char* rhs) const
{
   return !(*this > rhs);
}

bool
Data::operator>(const Data& rhs) const
{
   return rhs < *this;
}

bool
Data::operator>=(const Data& rhs) const
{
   return !(*this < rhs);
}

bool
Data::operator>(const char* rhs) const
{
   return rhs < *this;
}

bool
Data::operator>=(const char* rhs) const
{
   return !(*this < rhs);
}

Data& 
Data::operator=(const Data& data)
{
   assert(mBuf);
   
   if (&data != this)
   {
      if (!mMine)
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
      if ( mSize > 0 )
      {
         memmove(mBuf, data.mBuf, mSize);
      }
      mBuf[mSize] = 0;
   }
   return *this;
}

Data 
Data::operator+(const Data& data) const
{
   Data tmp(mSize + data.mSize, true);
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
   assert(p < mCapacity);
   own();
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

   if (!mMine)
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
   unsigned int l = strlen(str);
   Data tmp(mSize + l, true);
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
      resize(mSize + len, true);
   }
   else
   {
      if (!mMine)
      {
         // !dlb! violates invariant
         // if !mMine, then mCapacity == mSize, so should have resized
         assert(false);

         char *oldBuf = mBuf;
         mCapacity = mSize + len;
         mBuf = new char[mSize + len];
         memcpy(mBuf, oldBuf, mSize);
         mMine = true;
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
   Data tmp(mSize + 1, true);
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
   own();
   mBuf[mSize] = 0;
   return mBuf;
}

const char* 
Data::data() const
{
   return mBuf;
}

// generate additional capacity
void
Data::resize(size_type newCapacity, bool copy)
{
   char *oldBuf = mBuf;
   mBuf = new char[newCapacity+1];
   if (copy)
   {
      memcpy(mBuf, oldBuf, mSize);
      mBuf[mSize] = 0;
   }
   if (mMine)
   {
      delete[] oldBuf;
   }
   mMine = true;
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

//must be lowercase for MD5
static char hexmap[] = "0123456789abcdef";

Data 
Data::escaped() const
{ 
   Data ret(2*size(), true );  

   const char* p = data();
   for (size_type i=0; i < size(); i++)
   {
      unsigned char c = *p++;

      switch (c)
      {
         case 0x0A: // LF
         case 0x0D: // CR
         {
            ret += c;
            continue;
         }
      }
      
      if ( iscntrl(c) || (c>=0x7F) )
      {
         ret +='%';
         
         int hi = (c & 0xF0)>>4;
         int low = (c & 0x0F);
	   
         ret += hexmap[hi];
         ret += hexmap[low];
         continue;
      }

      ret += c;
   }

   return ret;
}

Data
Data::hex() const
{
   Data ret( 2*mSize, true );

   char* p = mBuf;
   char* r = ret.mBuf;
   for (size_type i=0; i < mSize; i++)
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
   for (size_type i=0; i < mSize; i++)
   {
      *p = tolower(*p);
      p++;
   }
   return *this;
}

Data&
Data::uppercase()
{
   own();
   char* p = mBuf;
   for (size_type i=0; i < mSize; i++)
   {
      *p = toupper(*p);
      p++;
   }
   return *this;
}

void
Data::clear()
{
   mSize = 0;
}

int 
Data::convertInt() const
{
   int val = 0;
   char* p = mBuf;
   int l = mSize;
   int s = 1;

   while (isspace(*p++))
   {
      l--;
   }
   p--;
   
   if (*p == '-')
   {
      s = -1;
      p++;
      l--;
   }
   
   while (l--)
   {
      char c = *p++;
      if ((c >= '0') && (c <= '9'))
      {
         val *= 10;
         val += c - '0';
      }
      else
      {
         return s*val;
      }
   }

   return s*val;
}

double 
Data::convertDouble() const
{
   long val = 0;
   char* p = mBuf;
   int s = 1;

   while (isspace(*p++));
   p--;
   
   if (*p == '-')
   {
      s = -1;
      p++;
   }
   
   while (isdigit(*p))
   {
      val *= 10;
      val += *p - '0';
      p++;
   }

   if (*p == '.')
   {
      p++;
      long d = 0;
      double div = 1.0;
      while (isdigit(*p))
      {
         d *= 10;
         d += *p - '0';
         div *= 10;
         p++;
      }
      return s*(val + d/div);
   }

   return s*val;
}

bool
Data::prefix(const Data& pre) const
{
   return strncmp(data(), pre.data(), pre.size()) == 0;
}

Data 
Data::substr(size_type first, size_type count) const
{
   assert(first <= mSize);
   if ( (int)count == Data::npos)
   {
      return Data(mBuf+first, mSize-first);
   }
   else
   {
      assert(first + count <= mSize);
      return Data(mBuf+first, count);
   }
}

int 
Data::find(const Data& match, size_type start) const
{
   return find(match.data(), start);
}

int 
Data::find(const char* match, size_type start) const
{
   if (start > mSize) 
   {
      return Data::npos;
   }
   else
   {
      ParseBuffer pb(mBuf+start, mSize);
      pb.skipToChars(match);
      if (pb.eof()) 
      {
         return Data::npos;
      }
      else
      {
         return pb.position() - pb.start();
      }
   }
}


bool
resip::operator==(const char* s, const Data& d)
{
   assert(s);
   return ((strncmp(s, d.data(), d.size()) == 0) &&
           strlen(s) == d.size() );
}

bool
resip::operator!=(const char* s, const Data& d)
{
   return !(s == d);
}

bool
resip::operator<(const char* s, const Data& d)
{
   assert(s);
   Data::size_type l = strlen(s);
   int res = strncmp(s, d.data(), resipMin(d.size(), l));

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
      return (l < d.size());
   }
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


#if defined(HASH_MAP_NAMESPACE)
size_t HASH_MAP_NAMESPACE::hash<resip::Data>::operator()(const resip::Data& data) const
{
   // 4 byte Pearson's hash
   // essentially random hashing

   // .dlb. better if layed out in host byte order

   // network order is big endian:
   unsigned char byte0(randomPermutation[0]);
   unsigned char byte1(randomPermutation[1]);
   unsigned char byte2(randomPermutation[2]);
   unsigned char byte3(randomPermutation[3]);

   const char* c = data.data();
   const char* end = c + data.size();
   for ( ; c != end; ++c)
   {
      byte0 = randomPermutation[*c ^ byte0];
      byte1 = randomPermutation[*c ^ byte1];
      byte2 = randomPermutation[*c ^ byte2];
      byte3 = randomPermutation[*c ^ byte3];
   }

   // convert from network to host byte order
   return ntohl((size_t)(byte0));
}
#endif

#if defined(__INTEL_COMPILER)
size_t std::hash_value(const resip::Data& data)
{
   // 4 byte Pearson's hash
   // essentially random hashing

   // .dlb. better if layed out in host byte order

   // network order is big endian:
   unsigned char byte0(randomPermutation[0]);
   unsigned char byte1(randomPermutation[1]);
   unsigned char byte2(randomPermutation[2]);
   unsigned char byte3(randomPermutation[3]);

   const char* c = data.data();
   const char* end = c + data.size();
   for ( ; c != end; ++c)
   {
      byte0 = randomPermutation[*c ^ byte0];
      byte1 = randomPermutation[*c ^ byte1];
      byte2 = randomPermutation[*c ^ byte2];
      byte3 = randomPermutation[*c ^ byte3];
   }

   // convert from network to host byte order
   return ntohl((size_t)(byte0));
}
#endif




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

