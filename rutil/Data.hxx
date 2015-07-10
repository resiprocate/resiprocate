#ifndef RESIP_Data_hxx
#define RESIP_Data_hxx

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <string>
#include <bitset>
#include "rutil/ResipAssert.h"

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

/**
   @internal
   This template is here to help diagnose API/ABI mismatches. Say you build
   librutil. A single Data::init(DataLocalSize<RESIP_DATA_LOCAL_SIZE>)
   function is implemented in Data.cxx, with the default
   value. (Data::init(DataLocalSize<16>) ends up being defined,
   Data::init(DataLocalSize<15>) does not) If, later, another build using
   that librutil tries to tweak the local alloc size to 24, it will end
   up attempting to call Data::init(DataLocalSize<24>); this will result
   in a link-time error, that while opaque, is less opaque than the stack
   corruption that would result otherwise.
**/
template <int S>
struct DataLocalSize
{
      explicit DataLocalSize(size_t) {}
};

// .bwc. Pack class Data; has to come before doxygen block though.
#pragma pack(4)

/**
  @brief An alternative to std::string, encapsulates an arbitrary buffer of 
  bytes.

  It has a variety of memory management styles that can be
  established at contruction time and changed later via setBuf().

  Three modes of allocation are currently available:

    @li 'Borrow' - The Data instance is borrowing the memory from the passed
                   in buffer. It will modify its contents as necessary,
                   but will not deallocate it.
            
    @li 'Share'  - The Data instance will use the buffer in a read-only mode.
                   If any attempt is made to modify the contents of
                   the Data, it will copy the buffer and modify it.
           
    @li 'Take'   - The Data instance takes complete ownership of the
                   buffer. The buffer is deallocated using delete[].

   Additionally, Data has a small locally-allocated buffer (member buffer) that
   it will use to hold small amounts of data. By default, this buffer can 
   contain 16 bytes, meaning that Data will not use the heap unless it
   needs more than 16 bytes of space. The tradeoff here, of course, is that
   instances of Data will be larger than instances of std::string. Generally
   speaking, if you expect to need more than 16 bytes of room, and you cannot
   make good use of the flexible memory management offered by Data, you may want
   to use a std::string instead.

  @see RESIP_HeapCount

  @todo It might be worthwhile examining the heap usage of this
        class in the context of using realloc everywhere appropriate.
        (realloc is defined in ANSI C, SVID, and the OpenGroup "Single
        Unix Specification").

   @ingroup text_proc
*/

class Data 
{
   public:
      RESIP_HeapCount(Data);

      typedef UInt32 size_type;

      inline Data()
         : mBuf(mPreBuffer),
           mSize(0),
           mCapacity(LocalAlloc),
           mShareEnum(Borrow)
      {
         mBuf[mSize] = 0;
      }

      /**
      @internal
      */
      class PreallocateType
      {
         friend class Data;
         explicit PreallocateType(int);
      };
      /**
      @brief used only to disambiguate constructors
      */
      static const PreallocateType Preallocate;

      /**
        Creates a data with a specified initial capacity.

        @param capacity  The initial capacity of the buffer

        @param foo       This parameter is ignored; it is merely
                         used to disambuguate this constructor
                         from the constructor that takes a single
                         int. Always pass Data::Preallocate.
      */
      Data(size_type capacity, const PreallocateType&);

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
      Data(size_type capacity, bool foo);
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
      Data(const char* buffer, size_type length);

      /**
        Creates a data with the contents of the buffer.

        @param length Number of bytes in the buffer
      */
      Data(const unsigned char* buffer, size_type length);

      Data(const Data& data);

#ifdef RESIP_HAS_RVALUE_REFS
      Data(Data &&data);
#endif
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
      explicit Data(Int32 value);

      /**
        Converts the passed in value into ascii-decimal
        representation, and then creates a "Data" containing
        that value. (E.g. "Data(75)" will create a Data
        with length=2, and contents of 0x37 0x35).
      */
      explicit Data(UInt32 value);

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
        Borrow=0,

        /** The Data instance will use the buffer in a read-only mode.
            If any attempt is made to modify the contents of
            the Data, it will copy the buffer and modify it.
        */
        Share=1,

        /** The Data instance takes complete ownership of the
            buffer. The buffer must have been allocate using
            "new char[]" so that it can be freed with "delete char[]".
        */
        Take=2
      };

      /**
        Creates a Data from the passed-in buffer.

        @see ShareEnum
      */

      Data(ShareEnum, const char* buffer, size_type length);

      /**
        Creates a Data from the passed-in buffer.

        @see ShareEnum
      */

      Data(ShareEnum, const char* buffer, size_type length, size_type capacity);

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
              way to use it with Take as long as you play with mShareEnum
              correctly?)
      */
      Data(ShareEnum, const Data& staticData); // Cannot call with 'Take'

      inline ~Data()
      {
         if (mShareEnum == Take)
         {
            delete[] mBuf;
         }
      }

      /**
        Set the Data to hold {buf} using share type {se}, which may be any
        of Share (read-only, no-free), Borrow (read-write, no-free)
        or Take (read-write, yes-free). Both the capacity
        and current length are set to {length}; you can call truncate2()
        afterwords to shorten.  The provided buffer (and its current
        contents) will be used going forward; any currently owned buffer
        will be released.
        NOTE: The {buf} param is declared const to support Share type; for
        Borrow and Take the buffer may be written (e.g., treated non-const).
      **/
      Data& setBuf(ShareEnum se, const char *buf, size_type length);

      /**
        Convience function to call setBuf() with a NULL-terminated string.
        This is in-lined for case where compiler knows strlen statically.
      **/
      Data& setBuf(ShareEnum se, const char *str)
      {
         return setBuf(se, str, (size_type)strlen(str));
      };


      /**
        Take the data from {other}. Any current buffer is released.
        {this} will have the same storage mode as {other} and steal
        its buffer. All storage modes of {other} (Share,Borrow,Take)
        are legal. When done, {other} will be empty (it will ref its
        internal buffer).
      **/
      Data& takeBuf(Data& other);

      /**
        Functional equivalent of: *this = Data(buf, length)
        and Data& copy(const char *buf, size_type length)
        but avoids an actual copy of the data if {other} is Shared
        or Borrowed.  Will have the same storage mode as {other}.
      **/
      Data& duplicate(const Data& other);

      /**
        Functional equivalent of: *this = Data(buf, length)
        but avoids the intermediate allocation and free. Also,
        will never decrease capacity. Safe to call even if {buf}
        is part of {this}.

        @note The result is always NULL terminated. Unfortunately,
        this requires a buffer allocation even if capacity exactly
        equals length.
      **/
      Data& copy(const char *buf, size_type length);

      /**
        Set size to be exactly {length}, extending buffer if needed.
        Also, reallocate buffer if needed so that it is writable.
        Buffer contents is NOT initialized, and existing contents
        may or may not be preserved.

        @note Purpose of this function is to provide a working buffer
        of fixed size that the application fills in after this call.

        @note If you want just the buffer without changing the size,
        use data() and cast-away the const-ness.

        @note The result may or may not be NULL terminated. The buffer
        is NULL terminated only when safe to do so without extra reallocation.
      **/
      char* getBuf(size_type length);

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

      friend bool operator==(const Data& lhs, const Data& rhs);
      friend bool operator==(const Data& lhs, const char* rhs);

      friend bool operator<(const Data& lhs, const Data& rhs);
      friend bool operator<(const Data& lhs, const char* rhs);
      friend bool operator<(const char* lhs, const Data& rhs);

      Data& operator=(const Data& data)
      {
         if (&data==this)
             return *this;
         return copy(data.mBuf,data.mSize);
      }

#ifdef RESIP_HAS_RVALUE_REFS
      Data& operator=(Data &&data);
#endif

      /**
        Assigns a null-terminated string to the buffer.

        @warning Passing a non-null-terminated string to this
                 method would be a Really Bad Thing.
        The strlen() inlined to take advantages of cases where
        the compiler knows the length statically.
      */
      Data& operator=(const char* str)
      {
         return copy(str, (size_type)strlen(str));
      }

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
      inline Data& operator+=(const Data& rhs)
      {
         return append(rhs.data(), rhs.size());
      }

      /**
        Appends a null-terminated string to the end of the Data
        object.

        @warning Passing a non-null-terminated string to this
                 method would be a Really Bad Thing.
      */
      inline Data& operator+=(const char* str)
      {
         resip_assert(str);
         return append(str, (size_type)strlen(str));
      }


      /**
        Appends a single byte to the Data object.
      */
      inline Data& operator+=(char c)
      {
         return append(&c, 1);
      }


      /**
        Performs an in-place exclusive-or of this buffer
        buffer with the specified buffer. If the specifed
        buffer is longer than this buffer, then this buffer
        will first be expanded and zero-padded.
      */
      Data& operator^=(const Data& rhs);

      /**
        Returns the character at the specified position. Ensures that ownership of
        the buffer is taken, since the character could be modified by the caller.
      */
      inline char& operator[](size_type p)
      {
         resip_assert(p < mSize);
         own();
         return mBuf[p];
      }

      /**
        Returns the character at the specified position.
      */
      inline char operator[](size_type p) const
      {
         resip_assert(p < mSize);
         return mBuf[p];
      }

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
        This owns() the buffer (undoes Share) so as to write
        terminating NULL. See truncate2() as alternative.

        @deprecated dlb says that no one uses this and
                    it should be removed.

        @todo Remove this at some point.
      */
      size_type truncate(size_type len);

      /**
        Shortens the size of this Data so length is at most of {len}.
        (If already shorter, doesn't increase length).
        Does not affect buffer allocation, and doesn't impact writing
        terminating NULL. Thus is safe to use with Share'd or external
        Take'n buffers.
      **/
      Data& truncate2(size_type len);

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
      inline const char* data() const
      {
         return mBuf;
      }

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
      inline const char* begin() const
      {
         return mBuf;
      }

      /**
        Returns a pointer to the end of the buffer used by the Data.
      */
      inline const char* end() const
      {
         return mBuf + mSize;
      }

      typedef enum
      {
         BINARY,
         BASE64,
         HEX
      } EncodingType;

      /**
        Computes the MD5 hash of the current data.
        @param type The encoding of the return (default is HEX)
        @return The MD5 hash, in the encoding specified by type.
      */      
      Data md5(EncodingType type=HEX) const;

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
        Converts this Data to lowercase, assuming this Data only consists of 
        scheme characters.

        @note Assuming scheme contents allows the use of a bitmask instead of
         tolower(), which is faster. Why, you ask? A bitmask is sufficient to 
         perform a lowercase operation on alphabetical data, since 'a' and 'A' 
         only differ on bit 6; it is set for 'a', but not for 'A'. Digits always 
         have bit 6 set, so setting it is a no-op. The last three characters in 
         the scheme character set are '+', '-', and '.'; all of these have bit 6 
         set as well. Note that there is no corresponding efficient uppercase 
         function; clearing bit 6 on either a digit or the the three remaining 
         characters (+=.) will change them.
      */
      Data& schemeLowercase();

      /**
        Returns a hexadecimal representation of the contents of
        this Data.
      */
      Data hex() const;

      /**
        Returns the binary form of the hexadecimal string in this Data
      */
      Data fromHex() const;

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
      EncodeStream& urlEncode(EncodeStream& s) const;

      /**
        Un-escapes a Data to a stream according to HTTP URL encoding rules.
      */
      EncodeStream& urlDecode(EncodeStream& s) const;

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
      EncodeStream& xmlCharDataEncode(EncodeStream& s) const;

      /**
        Un-escapes a Data to a stream according to XML Character Data encoding rules.
      */
      EncodeStream& xmlCharDataDecode(EncodeStream& s) const;

      /**
        Shortens the size of this Data. If the contents are truncated,
        this method appends two dot ('.') characters to the end.
        Presumably, this is used for output purposes.
      */
      Data trunc(size_type trunc) const;

      /**
        Clears the contents of this Data. This call does not modify
        the capacity of the Data. It does not write terminating NULL,
        and thus is safe to use with external buffers.
      */
      Data& clear() { return truncate2(0); };

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
          Replaces up to max occurrences of the bytes match with
          target. Returns the number of matches.
      */
      int replace(const Data& match, const Data& target, int max=INT_MAX);
      
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
      static size_t rawHash(const unsigned char* c, size_t size);

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
      static size_t rawCaseInsensitiveHash(const unsigned char* c, size_t size);

      /**
        A faster version of rawCaseInsensitiveHash that has the same collision 
        properties if this Data is made up of RFC 3261 token characters.

        @param c Pointer to the buffer to hash
        @param size Number of bytes to be hashed
        @note This is not guaranteed to return the same value as 
            rawCaseInsensitiveHash.
      */
      static size_t rawCaseInsensitiveTokenHash(const unsigned char* c, size_t size);

      /**
        Creates a 32-bit hash based on the contents of this Data, after
        normalizing any alphabetic characters to lowercase.
      */
      size_t caseInsensitivehash() const;

      /**
        A faster version of caseInsensitiveHash that has the same collision 
        properties if this Data is made up of RFC 3261 token characters.
        @note This is not guaranteed to return the same value as 
            rawCaseInsensitiveHash.
      */
      size_t caseInsensitiveTokenHash() const;

      inline bool caseInsensitiveTokenCompare(const Data& rhs) const
      {
         if(mSize==rhs.mSize)
         {
            return sizeEqualCaseInsensitiveTokenCompare(rhs);
         }
         return false;
      }

      bool sizeEqualCaseInsensitiveTokenCompare(const Data& rhs) const;

      /**
         Creates a bitset reflecting the contents of this data (as a set)
         ie. "15eo" would have the bits 49, 53, 101, and 111 set to true, and
         all others set to false
      */
      static std::bitset<256> toBitset(const resip::Data& chars);

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
      template<class Predicate> EncodeStream& 
          escapeToStream(EncodeStream& str, 
                         Predicate shouldEscape) const;

      /**
        Performs escaping of this Data according to a bitset.

        @param str          A stream to which the escaped representation
                            should be added.

        @param shouldEscape A bitset representing which chars should be escaped.
      */      
      std::ostream& escapeToStream(std::ostream& str, 
                                   const std::bitset<256>& shouldEscape) const;

      static Data fromFile(const Data& filename);

   private:
      /**
        @deprecated use Data(ShareEnum ...)
      */
      Data(const char* buffer, size_type length, bool);

      /**
        Used by string constructors
      */
      inline void initFromString(const char* str, size_type len);

      /**
        Copies the contents of this data to a new buffer if the
        Data does not own the current buffer.
      */
      void own() const;

      /**
        @note Always allocates a new buffer
      */
      void resize(size_type newSize, bool copy);

      static bool isHex(unsigned char c);      

      /** Trade off between in-object and heap allocation
          Larger LocalAlloc makes for larger objects that have Data members but
          bulk allocation/deallocation of Data  members. */
      enum {LocalAlloc = RESIP_DATA_LOCAL_SIZE };

      char* mBuf;
      size_type mSize;
      size_type mCapacity;
      char mPreBuffer[LocalAlloc];
      // Null terminator for mPreBuffer when mSize==LocalAlloc lands here; this
      // is ok, because Borrow==0.
      // Note: we could use a char here, and expand mPreBuffer by 3 bytes, but 
      // this imposes a performance penalty since it requires operating on a 
      // memory location smaller than a word (requires masking and such).
      size_type mShareEnum;

      friend std::ostream& operator<<(std::ostream& strm, const Data& d);
#ifndef RESIP_USE_STL_STREAMS
      friend EncodeStream& operator<<(EncodeStream& strm, const Data& d);
#endif
      friend class ParseBuffer;
      friend class DataBuffer;
      friend class DataStream;
      friend class oDataStream;
      friend class ::TestData;
      friend class MD5Buffer;
};
// reset alignment to default
#pragma pack()


class DataHelper {
   public:
      static const bool isCharHex[256];
};

static bool invokeDataInit = Data::init(DataLocalSize<RESIP_DATA_LOCAL_SIZE>(0));

inline bool Data::isHex(unsigned char c)
{
   return DataHelper::isCharHex[c];
}

inline bool isEqualNoCase(const Data& left, const Data& right)
{
   return ( (left.size() == right.size()) &&
            (strncasecmp(left.data(), right.data(), left.size()) == 0) );
}

inline bool isTokenEqualNoCase(const Data& left, const Data& right)
{
   return left.caseInsensitiveTokenCompare(right);
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

template<class Predicate> EncodeStream& 
Data::escapeToStream(EncodeStream& str, Predicate shouldEscape) const
{
   static char hex[] = "0123456789ABCDEF";

   if (empty())
   {
      return str;
   }
   
   const unsigned char* p = (unsigned char*)mBuf;
   const unsigned char* e = (unsigned char*)mBuf + mSize;

   while (p < e)
   {
      // ?abr? Why is this special cased? Removing this code
      // does not change the behavior of this method.
      if (*p == '%' 
          && e - p > 2 
          && isHex(*(p+1)) 
          && isHex(*(p+2)))
      {
         str.write((char*)p, 3);
         p+=3;
      }
      else if (shouldEscape[*p])
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

inline bool operator!=(const Data& lhs, const Data& rhs) { return !(lhs == rhs); }
inline bool operator>(const Data& lhs, const Data& rhs) { return rhs < lhs; }
inline bool operator<=(const Data& lhs, const Data& rhs) { return !(rhs < lhs); }
inline bool operator>=(const Data& lhs, const Data& rhs) { return !(lhs < rhs); }
inline bool operator!=(const Data& lhs, const char* rhs) { return !(lhs == rhs); }
inline bool operator>(const Data& lhs, const char* rhs) { return rhs < lhs; }
inline bool operator<=(const Data& lhs, const char* rhs) { return !(rhs < lhs); }
inline bool operator>=(const Data& lhs, const char* rhs) { return !(lhs < rhs); }
inline bool operator==(const char* lhs, const Data& rhs) { return rhs == lhs; }
inline bool operator!=(const char* lhs, const Data& rhs) { return !(rhs == lhs); }
inline bool operator>(const char* lhs, const Data& rhs) { return rhs < lhs; }
inline bool operator<=(const char* lhs, const Data& rhs) { return !(rhs < lhs); }
inline bool operator>=(const char* lhs, const Data& rhs) { return !(lhs < rhs); }
#ifndef  RESIP_USE_STL_STREAMS
EncodeStream& operator<<(EncodeStream& strm, const Data& d);
#endif
inline std::ostream& operator<<(std::ostream& strm, const Data& d)
{
   return strm.write(d.mBuf, d.mSize);
}


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
 * vi: set shiftwidth=3 expandtab:
 */
