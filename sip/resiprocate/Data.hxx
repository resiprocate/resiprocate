#ifndef STRINGDATA_H_
#define STRINGDATA_H_

static const char* const DataHeaderVersion =
"$Id: Data.hxx,v 1.5 2002/09/22 01:30:28 dabryan Exp $";

//Authors: Sunitha Kumar, Cullen Jennings

#include <cstring>
#include <string>

namespace Vocal2
{

#define NOT_FOUND -1
#define FIRST -2
#define FOUND 0
static const char SPACE[] = " ";

class Data : public std::string
{
      
   public:
      Data( );
      Data( const char* str );
      Data( const char* buffer, int length );
      Data( const Data& data );
      Data( const std::string& str);
      Data( const int value);

      //Data& operator=(const Data& data);

      // getData returns a NUL terminated (e.g. a C string) buffer
      const char* logData() const { return c_str(); }
      //const char* getData(char* buf, int len) const;
      //const char* getData(LocalScopeAllocator& lo) const;
      //const char* getDataBuf() const; // not null terminated
        
      //char getChar( int i ) const;  //return the i'th char of string.
      //void setchar( int i, char c );  //write to the i'th char of string.

      int length() const { return size(); }

      //string convertString() const;
      int convertInt() const;

      // match
      int match(const Data& match,
                Data* data,
                bool replace = false,
                const Data& replaceWith = "");
        
      // removes spaces before and after a string.
      void removeSpaces();

      //expand requird for expandin headers
      void expand(const Data& startFrom, 
                  const Data& findstr, 
                  const Data& replstr, 
                  const Data& delimiter);

      /** returns a substring of this object
          @param first      the first character to be part of the substring
          @param last       the last character of the substring, or -1 to 
          mean the last character overall
          thus, x.substring(0, -1) == x.
      */
      Data substring(int first, int last) const;


      // do a case-insensitive match
      friend bool isEqualNoCase( const Data& left, const Data& right ) ;

      //
      void deepCopy (const Data& src, char ** bufPtr = 0, int *bufLenPtr = 0);

      //
      int find( const Data& match, int start = 0 );

      // convert this Data to lower case
      void lowercase();

      // convert this Data to upper case
      void uppercase();

      /* 
         match (and eat) the first contiguous block composed of the
         characters in match, which is outside of double quotes <">
         and angle brackets "<" and ">". Returned is the data
         before the matched characters.  If no characters match,
         return the empty Data.  If matchFail is set to a bool ptr,
         the bool *matchFail will be set to true if the match
         fails, and false otherwise.

         This is designed for use in separating a list of
         parameters at the commas (e.g. Contact:)
      */
      Data parseOutsideQuotes(const char* match, 
                              bool useQuote,
                              bool useAngle,
                              bool* matchFail = 0 );
        
      /* 
         match (and eat) the first contiguous block composed of the
         characters in match. Returned is the data before the
         matched characters.  If no characters match, return the
         empty Data.  If matchFail is set to a bool ptr, the bool
         *matchFail will be set to true if the match fails, and
         false otherwise.
      */
      Data parse(const char* match, bool* matchFail = 0 );

      /* match (and eat) any one of the characters in match.  If
         matchedChar points to a char, it will be set to the
         matching character, or \0 if not matched to anything.
         Returns characters before the match, or the empty string
         if no characters match.
      */
      Data matchChar(const char* match, char* matchedChar = 0);

      /* get the next line in the text, delimited by \r\n or \n .
         Differs from parse("\r\n", matchFail) in that if there is
         a blank line (which has the contiguous text \r\n\r\n),
         parse will merely skip the empty line, while getLine will
         return the empty line as an empty Data.
      */
      Data getLine(bool* matchFail = 0 );

      // remove leading white space.
      void removeLWS();
};

 
}


#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#include <ext/hash_map>
namespace __gnu_cxx
{

struct hash<std::string>
{
      size_t operator()(const std::string& __s) const
      {
         return __gnu_cxx::__stl_hash_string(__s.c_str());
      }
};


struct hash<Vocal2::Data>
{
      size_t operator()(const Vocal2::Data& __s) const
      {
         return __gnu_cxx::__stl_hash_string(__s.c_str());
      }
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
