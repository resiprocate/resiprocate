#ifndef Data_hxx
#define Data_hxx

static const char* const DataHeaderVersion = "$Id: Data.hxx,v 1.24 2002/11/05 02:56:36 derekm Exp $";

#include <iostream>
#include <string>
#include <util/compat.hxx>

class TestData;
namespace Vocal2
{

class Data 
{
      
   public:
      Data();
      Data(int capacity, bool);
      Data(const char* str);
      Data(const char* buffer, int length);
      Data(const Data& data);
      Data(const std::string& str);
      explicit Data(int value);
      
      ~Data();

      bool operator==(const Data& rhs) const;
      bool operator!=(const Data& rhs) const { return !(*this == rhs); }
      bool operator==(const char* rhs) const;
      bool operator==(const std::string& rhs) const;
      bool operator<(const Data& rhs) const;

      Data& operator=(const Data& data);
      Data& operator=(const char* str);

      Data operator+(const Data& rhs) const;
      Data operator+(const char* str) const;
      Data operator+(char c) const;

      Data& operator+=(const char* str);
      Data& operator+=(const Data& rhs);
      Data& operator+=(char c);

      Data& append(const char* str, unsigned int len);

      bool empty() const { return mSize == 0; }
      int size() const { return mSize; }
      const char* c_str() const;
      // not necessarily NULL terminated
      const char* data() const;

      // compute an md5 hash (return in asciihex)
      Data md5() const;
      
      //covert this data(in place) to lower/upper case
      Data& lowercase();
      Data& uppercase();
      
   private:
      friend class TestData;
      Data(const char* buffer, int length, bool);
      void resize(unsigned int newSize, bool copy);
      unsigned int mSize;
      char* mBuf;
      unsigned int mCapacity;
      bool mMine;

      static const Data Empty;
      
      friend bool operator==(const char* s, const Data& d);
      friend bool operator!=(const char* s, const Data& d);
      friend std::ostream& operator<<(std::ostream& strm, const Data& d);
      friend class ParseBuffer;
      friend class DataBuffer;
};

inline bool isEqualNoCase(const Data& left, const Data& right)
{
   return ( (left.size() == right.size()) &&
            (strncasecmp(left.data(), right.data(), left.size()) == 0) );
}

bool operator==(const char* s, const Data& d);
bool operator!=(const char* s, const Data& d);
std::ostream& operator<<(std::ostream& strm, const Data& d);
 
}

#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#include <ext/hash_map>

namespace __gnu_cxx
{

struct hash<Vocal2::Data>
{
      size_t operator()(const Vocal2::Data& data) const;
};

}


#endif // gcc >= 3.1

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

