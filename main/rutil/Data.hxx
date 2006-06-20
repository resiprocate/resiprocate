#ifndef RESIP_Data_hxx
#define RESIP_Data_hxx

#include <iostream>
#include <string>

#include "rutil/compat.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "rutil/HashMap.hxx"

#ifndef RESIP_DATA_LOCAL_SIZE
#define RESIP_DATA_LOCAL_SIZE 16
#endif

class TestData;
namespace resip
{

template <int S>
struct DataLocalSize
{
      explicit DataLocalSize(size_t) {}
};

/**
  This class encapsulates an arbitrary buffer of bytes.
  It has a variety of memory management styles that can be
  established at contruction time.

  Three modes of allocation are currently available:

    @li 'Borrow' - The Data instance is borrowing the memory from the passed
                   in buffer. It will modify its contents as necessary,
                   but will not deallocate it.
            
    @li 'Share'  - The Data instance will use the buffer in a read-only mode.
                   If any attempt is made to modify the contents of
                   the Data, it will copy the buffer and modify it.
           
    @li 'Take'   - The Data instance takes complete ownership of the
                   buffer. The buffer is deallocated using delete[].

  @see RESIP_HeapCount

  @todo It might be worthwhile examining the heap usage of this
        class in the context of using realloc everywhere appropriate.
        (realloc is defined in ANSI C, SVID, and the OpenGroup "Single
        Unix Specification").
*/

class Data 
{
   public:
      RESIP_HeapCount(Data);

      typedef size_t size_type;

      Data();

      class PreallocateType
      {
	    friend class Data;
	    explicit PreallocateType(int);
      };
      static const PreallocateType Preallocate;

      /**
        Creates a data with a specified initial capacity.

        @param capacity  The initial capacity of the buffer

        @param foo       This parameter is ignored; it is merely
                         used to disambuguate this constructor
                         from the constructor that takes a single
                         int. Always pass Data::Preallocate.
      */
      Data(int capacity, const PreallocateType&);

//#define DEPRECATED_PREALLOC
#ifdef DEPRECATED_PREALLOC
      /**
        Creates a data with a specified initial capacity.

        @deprecated      This constructor shouldn't really exist;
                         it would be far better to add a value
                         to "ShareEnum" (e.g. "Allocate") which
                         indicates that the Data should allocated
                         its own buffer.

        @param capacity  The initial capacity of the buffer

        @param foo       This parameter is ignored; it is merely
                         used to disambuguate this constructor
                         from the constructor that takes a single
                         int. Yes, it's ugly -- that's why it's
                         deprecated.

        @todo Remove this constructor
      */
      Data(int capacity, bool foo);
#endif      

      /**
        Creates a data with a copy of the contents of the
        null-terminated string.

        @warning Passing a non-null-terminated string to this
                 method would be a Really Bad Thing.
      */
      Data(const char* str);

      /**
        Creates a data with the contents of the buffer.

        @param length Number of bytes in the buffer
      */
      Data(const char* buffer, int length);

      /**
        Creates a data with the contents of the buffer.

        @param length Number of bytes in the buffer
      */
      Data(const unsigned char* buffer, int length);

      Data(const Data& data);

      /**
        Creates a data with the contents of the string.
      */
      explicit Data(const std::string& str);

      /**
        Converts the passed in value into ascii-decimal
        representation, and then creates a "Data" containing
        that value. (E.g. "Data(75)" will create a Data
        with length=2, and contents of 0x37 0x35).
      */
      explicit Data(int value);

      /**
        Converts the passed in value into ascii-decimal
        representation, and then creates a "Data" containing
        that value. (E.g. "Data(75)" will create a Data
        with length=2, and contents of 0x37 0x35).
      */
      explicit Data(unsigned long value);

      /**
        Converts the passed in value into ascii-decimal
        representation, and then creates a "Data" containing
        that value. (E.g. "Data(75)" will create a Data
        with length=2, and contents of 0x37 0x35).
      */
      explicit Data(unsigned int value);

      /**
        Converts the passed in value into ascii-decimal
        representation, and then creates a "Data" containing
        that value. (E.g. "Data(75)" will create a Data
        with length=2, and contents of 0x37 0x35).
      */
      explicit Data(UInt64 value);

#ifndef RESIP_FIXED_POINT
      enum DoubleDigitPrecision 
      {
         ZeroDigitPrecision = 0, OneDigitPrecision, 
         TwoDigitPrecision, ThreeDigitPrecision, 
         FourDigitPrecision, FiveDigitPrecision,
         SixDigitPrecision, SevenDigitPrecision,
         EightDigitPrecision, NineDigitPrecision,
         TenDigitPrecision, MaxDigitPrecision
      };
      /**
        Converts the passed in value into ascii-decimal
        representation, and then creates a "Data" containing
        that value. (E.g. "Data(75.4,2)" will create a Data
        with length=4, and contents of 0x37 0x35 0x2E 0x34).

        @param precision  Number of digits after the decimal point.
                          Trailing zeros will be removed.
      */
      explicit Data(double value, 
		    Data::DoubleDigitPrecision precision = FourDigitPrecision);
#endif

      /**
        Creates a buffer containing "true" or "false", depending
        on the value of "value".
      */
      explicit Data(bool value);

      /**
        Creates a buffer containing a single character. Is this silly?
        Maybe. Perhaps it can be removed.
      */
      explicit Data(char c);

      /**
        The various memory management behaviors.
      */
      enum ShareEnum 
      {
        /** The Data instance is borrowing the memory from the passed
            in buffer. It will modify its contents as necessary,
            but will not deallocate it.
        */
        Share,

        /** The Data instance will use the buffer in a read-only mode.
            If any attempt is made to modify the contents of
            the Data, it will copy the buffer and modify it.
        */
        Borrow,

        /** The Data instance takes complete ownership of the
            buffer.
        */
        Take
      };

      /**
        Creates a Data from the passed-in buffer.

        @see ShareEnum
      */

      Data(ShareEnum, const char* buffer, int length);

      /**
        Takes a null-terminated string and creates a buffer.

        @see ShareEnum

        @warning Passing a non-null-terminated string to this
                 method would be a Really Bad Thing.
      */
      Data(ShareEnum, const char* buffer);

      /**
        Lazily creates a Data from the passed-in Data. 

        @see ShareEnum

        @warning Calling this with "Take" or "Borrow" is invalid and will
                 cause an assertion or crash.

        @todo This implementation has some confusing and conflicting
              comments. (e.g. is Borrow actually okay? Can there be some
              way to use it with Take as long as you play with mMine
              correctly?)
      */
      Data(ShareEnum, const Data& staticData); // Cannot call with 'Take'

      ~Data();

      /**
        Converts from arbitrary other type to Data. Requires the other
        type to have an operator<<.
      */
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

      bool operator!=(const Data& rhs) const { return !(*this == rhs); }
      bool operator!=(const char* rhs) const { return !(*this == rhs); }

      bool operator<(const Data& rhs) const;
      bool operator<=(const Data& rhs) const;
      bool operator<(const char* rhs) const;
      bool operator<=(const char* rhs) const;
      bool operator>(const Data& rhs) const;
      bool operator>=(const Data& rhs) const;
      bool operator>(const char* rhs) const;
      bool operator>=(const char* rhs) const;

      Data& operator=(const Data& data);

      /**
        Assigns a null-terminated string to the buffer.

        @warning Passing a non-null-terminated string to this
                 method would be a Really Bad Thing.
      */
      Data& operator=(const char* str);

      /**
        Concatenates two Data objects.
      */
      Data operator+(const Data& rhs) const;

      /**
        Concatenates a null-terminated string after the Data object.

        @warning Passing a non-null-terminated string to this
                 method would be a Really Bad Thing.
      */
      Data operator+(const char* str) const;

      /**
        Concatenates a single byte after Data object.
      */
      Data operator+(char c) const;

      /**
        Appends a data object to this one.
      */
      Data& operator+=(const Data& rhs);

      /**
        Appends a null-terminated string to the end of the Data
        object.

        @warning Passing a non-null-terminated string to this
                 method would be a Really Bad Thing.
      */
      Data& operator+=(const char* str);


      /**
        Appends a single byte to the Data object.
      */
      Data& operator+=(char c);


      /**
        Performs an in-place exclusive-or of this buffer
        buffer with the specified buffer. If the specifed
        buffer is longer than this buffer, then this buffer
        will first be expanded and zero-padded.
      */
      Data& operator^=(const Data& rhs);

      char& operator[](size_type p);
      char operator[](size_type p) const;

      /**
        Returns the character at the specified position.
      */
      char& at(size_type p);

      /**
        Guarantees that the underlying buffer used by the Data
        is at least the number of bytes specified. May cause
        reallocation of the buffer.
      */
      void reserve(size_type capacity);

      /**
        Appends the specified number of bytes to the end of
        this Data.
      */
      Data& append(const char* str, size_type len);

      /**
        Shortens the size of this Data. Does not
        impact the size of the allocated buffer.

        @deprecated dlb says that no one uses this and
                    it should be removed.

        @todo Remove this at some point.
      */
      size_type truncate(size_t len);

      /**
        Checks whether the Data is empty.
      */
      bool empty() const { return mSize == 0; }


      /**
        Returns the number of bytes in this Data.

        @note This does NOT indicate the capacity of the
              underlying buffer.
      */
      size_type size() const { return mSize; }

      /**
        Returns a pointer to the contents of this Data. This
        is the preferred mechanism for accessing the bytes inside
        the Data.

        @note The value returned is NOT necessarily null-terminated.
      */
      const char* data() const;

      /**
        Returns a null-terminated string representing 

        @note    Depending on the memory management scheme being used,
                 this method often copies the contents of the Data;
                 consequently, this method is rather expensive and should
                 be avoided when possible.

        @warning Calling this method is a pretty bad idea if the
                 contents of Data are binary (i.e. may contain
                 a null in the middle of the Data).
      */
      const char* c_str() const;

      /**
        Returns a pointer to the beginning of the buffer used by the Data.
      */
      const char* begin() const;

      /**
        Returns a pointer to the end of the buffer used by the Data.
      */
      const char* end() const;

      /**
        Computes the MD5 hash of the current data.

        @return ASCII hexadecimal representation of the MD5 hash
      */      
      Data md5() const;

      /**
        Converts this Data to lowercase.

        @note This is silly unless the contents are ASCII.
      */      
      Data& lowercase();

      /**
        Converts this Data to uppercase.

        @note This is silly unless the contents are ASCII.
      */      
      Data& uppercase();

      /**
        Returns a hexadecimal representation of the contents of
        this Data.
      */
      Data hex() const;

      /**	
        Returns a representation of the contents of the data
        with any non-printable characters escaped.

        @warning This is extremely slow, and should not be called
                 except for debugging purposes.
      */
      Data escaped() const;

      /**
        Performs RFC 3261 escaping of SIP URIs.

        @note This method is relatively inefficient

        @deprecated Use escapeToStream instead

        @todo This method should be removed

        @see escapeToStream
      */
      Data charEncoded() const;

      /**
        Performs RFC 3261 un-escaping of SIP URIs.

        @note This method is relatively inefficient

        @bug This method can assert if a "%00" comes
             in off the wire. That's really bad form.

        @deprecated Use something more in the spirit of escapeToStream instead

        @todo This method should be removed

        @see escapeToStream
      */
      Data charUnencoded() const;

      /**
        Performs in-place HTTP URL escaping of a Data.
      */
      Data urlEncoded() const;

      /**
        Performs in-place HTTP URL un-escaping of a Data.
      */
      Data urlDecoded() const;

      /**
        Escapes a Data to a stream according to HTTP URL encoding rules.
      */
      std::ostream& urlEncode(std::ostream& s) const;

      /**
        Un-escapes a Data to a stream according to HTTP URL encoding rules.
      */
      std::ostream& urlDecode(std::ostream& s) const;

      /**
        Performs in-place XML Character Data escaping of a Data.
      */
	  Data xmlCharDataEncode() const;

      /**
        Performs in-place XML Character Data un-escaping of a Data.
      */
	  Data xmlCharDataDecode() const;

      /**
        Escapes a Data to a stream according to XML Character Data encoding rules.
      */
	  std::ostream& xmlCharDataEncode(std::ostream& s) const;

      /**
        Un-escapes a Data to a stream according to XML Character Data encoding rules.
      */
	  std::ostream& xmlCharDataDecode(std::ostream& s) const;

      /**
        Shortens the size of this Data. If the contents are truncated,
        this method appends two dot ('.') characters to the end.
        Presumably, this is used for output purposes.
      */
      Data trunc(size_type trunc) const;

      /**
        Clears the contents of this Data. This call does not modify
        the capacity of the Data.
      */	
      Data& clear();

      /**
        Takes the contents of the Data and converts them to an 
        integer. Will strip leading whitespace. This method stops
        upon encountering the first non-decimal digit (with exceptions
        made for leading negative signs).
      */ 
      int convertInt() const;
      unsigned long convertUnsignedLong() const;

      /**
        Takes the contents of the Data and converts them to a 
        size_t. Will strip leading whitespace. This method stops
        upon encountering the first non-decimal digit.
      */ 
      size_t convertSize() const;

#ifndef RESIP_FIXED_POINT
      /**
        Takes the contents of the Data and converts them to a 
        double precision floating point value. Will strip leading
        whitespace. This method stops upon encountering the first
        non-decimal digit (with exceptions made for decimal points
        and leading negative signs).
      */ 
      double convertDouble() const;
#endif

      /**
        Takes the contents of the Data and converts them to an
        unsigned 64-bit integer. Will strip leading whitespace.
        This method stops upon encountering the first non-decimal digit.
      */ 
      UInt64 convertUInt64() const;

      /**
        Returns true if this Data starts with the bytes indicated by
        the passed-in Data. For example, if this Data is "abc", then
        prefix(Data("ab")) would be true; however, prefix(Data("abcd"))
        would be false.
      */
      bool prefix(const Data& pre) const;

      /**
        Returns true if this Data ends with the bytes indicated by
        the passed-in Data. For example, if this Data is "abc", then
        postfix(Data("bc")) would be true; however, postfix(Data("ab"))
        would be false.
      */
      bool postfix(const Data& post) const;

      /**
        Copies a portion of this Data into a new Data.
 
        @param first Index of the first byte to copy
        @param count Number of bytes to copy
      */
      Data substr(size_type first, size_type count = Data::npos) const;

      /**
        Finds a specified sequence of bytes in this Data.

        @param match The bytes to be found

        @param start Offset into this Data to start the search

        @returns An index to the start of the found bytes.
      */
      size_type find(const Data& match, size_type start = 0) const;

      /** 
          Replaces all occurrences of the bytes match with
          target. Returns the number of matches.
      */
      int replace(const Data& match, const Data& target);
      
      /**
        Constant that represents a zero-length data.
      */
      static const Data Empty;

      /**
	 Represents an impossible position; returned to indicate failure to find.
      */
      static const size_type npos;

      /**
        Initializes Data class.

        @note This method is a link time constraint. Don't remove it.
      */
      static bool init(DataLocalSize<RESIP_DATA_LOCAL_SIZE> arg);

      /**
        Performs RFC 3548 Base 64 decoding of the contents of this data.

        @returns A new buffer containing the unencoded representation
      */
      Data base64decode() const;

      /**
        Performs RFC 3548 Base 64 encoding of the contents of this data.

        @returns A new buffer containing the base64 representation
      */
      Data base64encode(bool useUrlSafe=false) const;

      /**
        Creates a 32-bit hash based on the contents of the indicated
        buffer.

        @param c Pointer to the buffer to hash
        @param size Number of bytes to be hashed
      */
      static size_t rawHash(const char* c, size_t size);

      /**
        Creates a 32-bit hash based on the contents of this Data.
      */
      size_t hash() const;

      /**
        Creates a 32-bit hash based on the contents of the indicated
        buffer, after normalizing any alphabetic characters to lowercase.

        @param c Pointer to the buffer to hash
        @param size Number of bytes to be hashed
      */
      static size_t rawCaseInsensitiveHash(const char* c, size_t size);

      /**
        Creates a 32-bit hash based on the contents of this Data, after
        normalizing any alphabetic characters to lowercase.
      */
      size_t caseInsensitivehash() const;

      /**
        Performs escaping of this Data according to the indicated
        Predicate.

        @param str          A stream to which the escaped representation
                            should be added.

        @param shouldEscape A functor which takes a single character
                            as a parameter, and returns true if the
                            character should be escaped, false if
                            it should not.

	@deprecated dlb -- pass a 256 array of bits rather than a function.
      */      
      template<class Predicate> std::ostream& 
          escapeToStream(std::ostream& str, 
                         Predicate shouldEscape) const;

   private:
      /**
        @deprecated use Data(ShareEnum ...)
      */
      Data(const char* buffer, int length, bool);

      /**
        Copies the contents of this data to a new buffer if the
        Data does not own the current buffer.
      */
      void own() const;

      /**
        @note Always allocates a new buffer
      */
      void resize(size_type newSize, bool copy);

      static bool isHex(char c);      

      /** Trade off between in-object and heap allocation
	  Larger LocalAlloc makes for larger objects that have Data members but
	  bulk allocation/deallocation of Data  members. */
      enum {LocalAlloc = RESIP_DATA_LOCAL_SIZE };
      char mPreBuffer[LocalAlloc+1];

      size_type mSize;
      char* mBuf;
      size_type mCapacity;
      ShareEnum mMine;
      // The invariant for a Data with !mMine is mSize == mCapacity

      static const bool isCharHex[256];

      friend bool operator==(const char* s, const Data& d);
      friend bool operator!=(const char* s, const Data& d);
      friend std::ostream& operator<<(std::ostream& strm, const Data& d);
      friend class ParseBuffer;
      friend class DataBuffer;
      friend class DataStream;
      friend class oDataStream;
      friend class ::TestData;
      friend class MD5Buffer;
      friend class Contents;
};

static bool invokeDataInit = Data::init(DataLocalSize<RESIP_DATA_LOCAL_SIZE>(0));

inline bool Data::isHex(char c)
{
   return isCharHex[(size_t) c];
}

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

template<class Predicate> std::ostream& 
Data::escapeToStream(std::ostream& str, Predicate shouldEscape) const
{
   static char hex[] = "0123456789ABCDEF";

   if (empty())
   {
      return str;
   }
   
   const char* p = mBuf;
   const char* e = mBuf + mSize;

   while (p < e)
   {
      // ?abr? Why is this special cased? Removing this code
      // does not change the behavior of this method.
      if (*p == '%' 
          && e - p > 2 
          && isHex(*(p+1)) 
          && isHex(*(p+2)))
      {
         str.write(p, 3);
         p+=3;
      }
      else if (shouldEscape(*p))
      {
         int hi = (*p & 0xF0)>>4;
         int low = (*p & 0x0F);
	   
         str << '%' << hex[hi] << hex[low];
         p++;
      }
      else
      {
         str.put(*p++);
      }
   }
   return str;
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

HashValue(resip::Data);

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
