static const char* const Data_cxx_Version =
"$Id: Data.cxx,v 1.30 2002/11/12 05:16:39 jason Exp $";

#include <algorithm>
#include <cassert>
#include <ctype.h>

#include "sip2/util/Data.hxx"
#include "sip2/util/vmd5.hxx"
#include "sip2/util/RandomHex.hxx"

using namespace Vocal2;
using namespace std;


const Data Data::Empty("", 0);

Data::Data() 
   : mSize(0),
     mBuf(Data::Empty.mBuf),
     mCapacity(mSize),
     mMine(false)
{
}

Data::Data(const char* str, int length) 
   : mSize(length),
     mBuf(new char[mSize+1]),
     mCapacity(mSize),
     mMine(true)
{
   assert(str);
   memcpy(mBuf, str, mSize);
}

// share memory KNOWN to be in a surrounding scope
// wears off on modify, copy, c_str, assign
Data::Data(const char* str, int length, bool) 
   : mSize(length),
     mBuf(const_cast<char*>(str)),
     mCapacity(mSize),
     mMine(false)
{
   assert(str);
}

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

// new functions

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
     mCapacity(0)
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
      mBuf[c--] = '0' + v%10;
      v /= 10;
   }
}

Data::Data(char c)
   : mSize(1), 
     mBuf(0),
     mCapacity(mSize)
{
   mBuf = new char[2];
   mBuf[0] = c;
   mBuf[1] = 0;
}

Data::Data(bool value)
   : mSize(0), 
     mBuf(0),
     mCapacity(0)
{
   static char* truec = "true";
   static char* falsec = "false";

   if (value)
   {
      mBuf = new char[5];
      mSize = 4;
      mCapacity = 4;
      memcpy(mBuf, truec, 5);
   }
   else
   {
      mBuf = new char[6];
      mSize = 5;
      mCapacity = 5;
      memcpy(mBuf, falsec, 6);
   }
}


// end new functions






Data::Data(const Data& data) 
   : mSize(data.mSize),
     mBuf(new char[mSize+1]),
     mCapacity(mSize),
     mMine(true)
{
   memcpy(mBuf, data.mBuf, mSize);
   mBuf[mSize] = 0;
}

Data::~Data()
{
   if (mMine)
   {
      delete[] mBuf;
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
Data::operator==(const std::string& rhs) const
{
   if (mSize != rhs.size())
   {
      return false;
   }
   return strncmp(mBuf, rhs.c_str(), mSize) == 0;
}

bool
Data::operator<(const Data& rhs) const
{
   int res = strncmp(mBuf, rhs.mBuf, min(mSize, rhs.mSize));

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
Data::operator>(const Data& rhs) const
{
   return rhs < *this;
}

Data& 
Data::operator=(const Data& data)
{
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
      memmove(mBuf, data.mBuf, mSize);
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
   if (mCapacity < mSize + data.mSize)
   {
      // .dlb. pad for future growth?
      resize(mSize + data.mSize, true);
   }
   else
   {
      if (!mMine)
      {
         char *oldBuf = mBuf;
         mBuf = new char[mSize + data.mSize];
         memcpy(mBuf, oldBuf, mSize);
         mMine = true;
      }
   }
   memmove(mBuf + mSize, data.mBuf, data.mSize);
   mSize += data.mSize;
   mCapacity = mSize;
   mBuf[mSize] = 0;

   return *this;
}

Data& 
Data::operator+=(const char* str)
{
   return append(str, strlen(str));
}

Data&
Data::operator+=(char c)
{
   if (mCapacity < mSize + c)
   {
      // .dlb. pad for future growth?
      resize(mSize + 1, true);
   }
   else
   {
      if (!mMine)
      {
         char *oldBuf = mBuf;
         mBuf = new char[mSize + 1];
         memcpy(mBuf, oldBuf, mSize);
         mMine = true;
      }
   }
   mBuf[mSize] = c;
   mSize += 1;
   mBuf[mSize] = 0;
   mCapacity = mSize;

   return *this;
}

char& 
Data::operator[](size_type p)
{
   assert(p > 0 && p < mSize);
   if (!mMine)
   {
      resize(mSize, true);
   }
   return mBuf[p];
}

char 
Data::operator[](size_type p) const
{
   assert(p > 0 && p < mSize);
   return mBuf[p];
}


Data& 
Data::operator=(const char* str)
{
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
   unsigned int l = strlen(str);
   Data tmp(mSize + l, true);
   tmp.mSize = mSize + l;
   tmp.mCapacity = tmp.mSize;
   memcpy(tmp.mBuf, mBuf, mSize);
   memcpy(tmp.mBuf + mSize, str, l+1);

   return tmp;
}

Data&
Data::append(const char* str, size_type len)
{
   if (mCapacity < mSize + len)
   {
      // .dlb. pad for future growth?
      resize(mSize + len, true);
   }
   else
   {
      if (!mMine)
      {
         char *oldBuf = mBuf;
         mBuf = new char[mSize + len];
         memcpy(mBuf, oldBuf, mSize);
         mMine = true;
      }
   }
   // could conceivably overlap
   memmove(mBuf + mSize, str, len);
   mSize += len;
   mBuf[mSize] = 0;
   mCapacity = mSize;

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
   if (!mMine)
   {
      const_cast<Data*>(this)->resize(mSize, true);
   }
   mBuf[mSize] = 0;
   return mBuf;
}

const char* 
Data::data() const
{
   return mBuf;
}

// pre-allocate capacity
Data::Data(int capacity, bool) 
   : mSize(0),
     mBuf(new char[capacity + 1]),
     mCapacity(capacity),
     mMine(true)
{
   mBuf[0] = 0;
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

   unsigned char digest[16];
   MD5Final(digest, &context);
   return RandomHex::convertToHex(digest, 16);
}


Data&
Data::lowercase()
{
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
   char* p = mBuf;
   for (size_type i=0; i < mSize; i++)
   {
      *p = toupper(*p);
      p++;
   }
   return *this;
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
Vocal2::operator==(const char* s, const Data& d)
{
   return d == s;
}

bool
Vocal2::operator!=(const char* s, const Data& d)
{
   return d != s;
}

ostream& 
Vocal2::operator<<(ostream& strm, const Data& d)
{
   return strm.write(d.mBuf, d.mSize);
}

#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
size_t 
__gnu_cxx::hash<Vocal2::Data>::operator()(const Vocal2::Data& data) const
{
   unsigned long __h = 0; 
   const char* start = data.data(); // non-copying
   const char* end = start + data.size();
   for ( ; start != end; ++start)
   {
      __h = 5*__h + *start; // .dlb. weird hash
   }
   return size_t(__h);
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

