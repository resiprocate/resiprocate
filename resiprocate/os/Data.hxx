#ifndef RESIP_Data_hxx
#define RESIP_Data_hxx

static const char* const resipDataHeaderVersion =
   "$Id: Data.hxx,v 1.78 2004/05/19 02:11:36 davidb Exp $";

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/DataStream.hxx"
#include <iostream>
#include <string>
#include "resiprocate/os/HeapInstanceCounter.hxx"

#include "HashMap.hxx"
#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if !defined(RESIP_DATA_LOCAL_SIZE)
#define RESIP_DATA_LOCAL_SIZE 128
#endif

class TestData;
namespace resip
{

class Data 
{
   public:
      RESIP_HeapCount(Data);

      typedef size_t size_type;

      Data();
      Data(int capacity, bool);
      Data(const char* str);// Uses share mode
      Data(const char* buffer, int length); // Uses share mode
      Data(const unsigned char* buffer, int length);// Uses share mode
      Data(const Data& data);
      explicit Data(const std::string& str);
      explicit Data(int value);
      explicit Data(unsigned long value);
      explicit Data(unsigned int value);
      explicit Data(double value, int precision = 4);
      explicit Data(bool value);
      explicit Data(char c);

      // construct a Data that shares memory; the passed characters MUST be
      // immutable and in a longer lasting scope -- or take the buffer
      // as thine own.
      /** 
          'Borrow' means that the Data instance MAY write to the buffer and MUST
          NOT delete the buffer.
           
          'Share' means that the Data instance MUST NOT write to the buffer and
          MUST NOT delete the buffer.
          
          'Take' means that the Data instance MAY write to the buffer and MUST
          delete the buffer.
    */
      enum  ShareEnum {Share, Borrow, Take};

      Data(ShareEnum, const char* buffer, int length);
      Data(ShareEnum, const char* buffer);
      Data(ShareEnum, const Data& staticData); // Cannot call with 'Take'

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
      bool operator<=(const Data& rhs) const;
      bool operator<(const char* rhs) const;
      bool operator<=(const char* rhs) const;
      bool operator>(const Data& rhs) const;
      bool operator>=(const Data& rhs) const;
      bool operator>(const char* rhs) const;
      bool operator>=(const char* rhs) const;

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
      char& at(size_type p);

      void reserve(size_type capacity);
      Data& append(const char* str, size_type len);
      size_type truncate(size_t len);

      bool empty() const { return mSize == 0; }
      size_type size() const { return mSize; }

      // preferred -- not necessarily NULL terminated
      const char* data() const;

      // necessarily NULL terminated -- often copies
      const char* c_str() const;

      const char* begin() const;
      const char* end() const;
      
      // compute an md5 hash (return in asciihex)
      Data md5() const;
      
      // convert this data in place to lower/upper case
      Data& lowercase();
      Data& uppercase();

      // return a HEX representation of binary data
      Data hex() const;
	
      // return a representation with any non printable characters escaped - very
      // slow only use for debug stuff 
      Data escaped() const;

      /// encodes with %hex for special characters
      Data charEncoded() const;
      Data charUnencoded() const;
      Data trunc(size_type trunc) const;
	
      // resize to zero without changing capacity
      void clear();
      int convertInt() const;
      size_t convertSize() const;
      double convertDouble() const;
      UInt64 convertUInt64() const;


      bool prefix(const Data& pre) const;
      bool postfix(const Data& post) const;
      Data substr(size_type first, size_type count = Data::npos) const;
      size_type find(const Data& match, size_type start = 0) const;
      size_type find(const char* match, size_type start = 0) const;

      static const Data Empty;
      static const size_type npos;

      static bool init();

      /// return base64 decode as specified in RFC 3548
      Data base64decode() const;

      /// return base64 encode as specified in RFC 3548
      Data base64encode(bool useUrlSafe=false) const;

      static size_t rawHash(const char* c, size_t size);
      size_t hash() const;

      static size_t rawCaseInsensitiveHash(const char* c, size_t size);
      size_t caseInsensitivehash() const;
      
   private:
      Data(const char* buffer, int length, bool); // deprecated: use // Data(ShareEnum ...)

      // copy if not mine
      void own() const;
      void resize(size_type newSize, bool copy);

      // Trade off between in-object and heap allocation
      // Larger LocalAlloc makes for larger objects that have Data members but
      // bulk allocation/deallocation of Data  members.

      enum {LocalAlloc = RESIP_DATA_LOCAL_SIZE};
      char mPreBuffer[LocalAlloc+1];

      size_type mSize;
      char* mBuf;
      size_type mCapacity;
      ShareEnum mMine;
      // The invariant for a Data with !mMine is mSize == mCapacity


      friend bool operator==(const char* s, const Data& d);
      friend bool operator!=(const char* s, const Data& d);
      friend std::ostream& operator<<(std::ostream& strm, const Data& d);
      friend class ParseBuffer;
      friend class DataBuffer;
      friend class oDataBuffer;
      friend class ::TestData;
      friend class MD5Buffer;
      friend class Contents;
      friend class oDataStream;
      friend class DataStream;
};

static bool invokeDataInit = Data::init();


inline bool isEqualNoCase(const Data& left, const Data& right)
{
   return ( (left.size() == right.size()) &&
            (strncasecmp(left.data(), right.data(), left.size()) == 0) );
}

inline bool isLessThanNoCase(const Data& left, const Data& right)
{
   size_t minsize = resipMin( left.size(), right.size() );
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

template<>
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
