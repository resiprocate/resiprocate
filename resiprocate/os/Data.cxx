static const char* const Data_cxx_Version =
"$Id: Data.cxx,v 1.17 2002/11/01 22:01:07 jason Exp $";

#include <algorithm>
#include <cassert>

#include <util/Data.hxx>
#include <util/vmd5.hxx>
#include <util/RandomHex.hxx>

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
     mBuf(new char[mSize + 1]),
     mCapacity(mSize),
     mMine(true)
{
   assert(str);
   memcpy(mBuf, str, mSize + 1);
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
   switch (strncmp(mBuf, rhs.mBuf, min(mSize, rhs.mSize)))
   {
      case 1:
      {
         return true;
      }
      case -1:
      {
         return false;
      }
      case 0:
      {
         return (mSize < rhs.mSize);
      }
      default:
         // strncmp returned a perculiar value-- use signum?
         assert(0);
		return false;
   }
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
Data::operator+(const Data& data)
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

Data& 
Data::operator=(const char* str)
{
   unsigned int l = strlen(str);

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
Data::operator+(const char* str)
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
Data::append(const char* str, unsigned int len)
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
Data::operator+(char c)
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
Data::resize(unsigned int newCapacity, 
             bool copy)
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

