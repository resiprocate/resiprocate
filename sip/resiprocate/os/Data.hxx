#if !defined(resip_Data_hxx)
#define resip_Data_hxx

static const char* const resipDataHeaderVersion = "$Id: Data.hxx,v 1.62 2003/03/25 06:33:48 jason Exp $";

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/DataStream.hxx"
#include <iostream>
#include <string>
#include "HashMap.hxx"

class TestData;
namespace resip
{

class Data 
{
      
   public:
      typedef size_t size_type;

      Data();
      Data(int capacity, bool);
      Data(const char* str);
      Data(const char* buffer, int length);
      Data(const unsigned char* buffer, int length);
      Data(const Data& data);
      explicit Data(const std::string& str);
      explicit Data(int value);
      explicit Data(unsigned long value);
      explicit Data(unsigned int value);
      explicit Data(double value, int precision = 4);
      explicit Data(bool value);
      explicit Data(char c);
      
      ~Data();

      // convert from something to a Data -- requires 'something' has operator<<
      template<class T>
      static Data from(const T& x)
      {
         Data d;
         {
            DataStream s(d);
            s << x;
         }
         return d;
      }

      bool operator==(const Data& rhs) const;
      bool operator==(const char* rhs) const;
      //bool operator==(const std::string& rhs) const;

      bool operator!=(const Data& rhs) const { return !(*this == rhs); }
      bool operator!=(const char* rhs) const { return !(*this == rhs); }
      //bool operator!=(const std::string& rhs) const { return !(*this == rhs); }

      bool operator<(const Data& rhs) const;
      bool operator<(const char* rhs) const;
      bool operator>(const Data& rhs) const;
      bool operator>(const char* rhs) const;

      Data& operator=(const Data& data);
      Data& operator=(const char* str);

      Data operator+(const Data& rhs) const;
      Data operator+(const char* str) const;
      Data operator+(char c) const;

      Data& operator+=(const char* str);
      Data& operator+=(const Data& rhs);
      Data& operator+=(char c);

      Data& operator^=(const Data& rhs);

      char& operator[](size_type p);
      char operator[](size_type p) const;

      void reserve(size_type capacity);
      Data& append(const char* str, size_type len);

      bool empty() const { return mSize == 0; }
      size_type size() const { return mSize; }

      // preferred -- not necessarily NULL terminated
      const char* data() const;

      // necessarily NULL terminated -- often copies
      const char* c_str() const;

      // compute an md5 hash (return in asciihex)
      Data md5() const;
      
      // convert this data in place to lower/upper case
      Data& lowercase();
      Data& uppercase();

      // return a HEX representation of binary data
      Data hex() const;
	
      // return a representation with any non printable chracters escaped - very
      // slow only use for debug stuff 
      Data escaped() const;
	
      // resize to zero without changing capacity
      void clear();
      int convertInt() const;
      double convertDouble() const;

      bool prefix(const Data& pre);
      Data substr(size_type first, size_type count = Data::npos) const;
      int find(const Data& match, size_type start = 0) const;
      int find(const char* match, size_type start = 0) const;

      static const Data Empty;
      static const int npos;

   private:
      char* initializeHack();
      
      Data(const char* buffer, int length, bool);

      // copy if not mine
      void own() const
      {
         if (!mMine) 
         {
            const_cast<Data*>(this)->resize(mSize, true);
         }
      }
      void resize(size_type newSize, bool copy);
      size_type mSize;
      char* mBuf;
      size_type mCapacity;
      bool mMine;

      friend bool operator==(const char* s, const Data& d);
      friend bool operator!=(const char* s, const Data& d);
      friend std::ostream& operator<<(std::ostream& strm, const Data& d);
      friend class ParseBuffer;
      friend class DataBuffer;
      friend class ::TestData;
      friend class MD5Buffer;
      friend class Contents;
};

inline bool isEqualNoCase(const Data& left, const Data& right)
{
   return ( (left.size() == right.size()) &&
            (strncasecmp(left.data(), right.data(), left.size()) == 0) );
}

inline bool isLessThanNoCase(const Data& left, const Data& right)
{
   size_t minsize = vocal2Min( left.size(), right.size() );
   int res = strncasecmp(left.data(), right.data(), minsize);

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
       return left.size() < right.size();
   }
}

bool operator==(const char* s, const Data& d);
bool operator!=(const char* s, const Data& d);
bool operator<(const char* s, const Data& d);

std::ostream& operator<<(std::ostream& strm, const Data& d);

inline Data
operator+(const char* c, const Data& d)
{
   return Data(c) + d;
}

 
}

#if  defined(__INTEL_COMPILER )
namespace std
{
size_t hash_value(const resip::Data& data);
}

#elif defined(HASH_MAP_NAMESPACE)  //#elif ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
namespace HASH_MAP_NAMESPACE
{
struct hash<resip::Data>
{
      size_t operator()(const resip::Data& data) const;
};
}

#endif // HASHMAP

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

