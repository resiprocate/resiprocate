#if !defined(RESIP_LAZYPARSER_HXX)
#define RESIP_LAZYPARSER_HXX 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iosfwd>
#include "resip/stack/HeaderFieldValue.hxx"
#include "rutil/resipfaststreams.hxx"

namespace resip
{

class ParseBuffer;
class Data;

/**
   @brief The base-class for all lazily-parsed SIP grammar elements.

   Subclasses of this are parse-on-access; the parse will be carried out (if
   it hasn't already) the first time one of the members is accessed. Right now,
   all header field values and SIP bodies are lazily parsed.

   Application writers are expected to make consistent use of isWellFormed() in
   order to determine whether an element is parseable. Using a try/catch block
   around accesses will not work as expected, since an exception will only be
   thrown the _first_ time an access is made, and it is determined that the 
   element isn't parseable.

   @ingroup resip_crit
   @ingroup sip_parse
*/
class LazyParser
{
   public:
      /**
         @internal
      */
      explicit LazyParser(const HeaderFieldValue& headerFieldValue);

      /**
         @internal
      */
      LazyParser(const char* buf, int length);

      /**
         @internal
      */
      LazyParser(const HeaderFieldValue& headerFieldValue,
                  HeaderFieldValue::CopyPaddingEnum e);

      /**
         @internal
      */
      LazyParser(const LazyParser& rhs);

      /**
         @internal
      */
      LazyParser(const LazyParser& rhs,HeaderFieldValue::CopyPaddingEnum e);

      /**
         @internal
      */
      LazyParser& operator=(const LazyParser& rhs);
      virtual ~LazyParser();

      /**
         @internal
      */
      virtual EncodeStream& encodeParsed(EncodeStream& str) const = 0;

      /**
         @internal
      */
      virtual void parse(ParseBuffer& pb) = 0;

      /**
         @brief Encodes this element to a stream, in the fashion it should be 
            represented on the wire.
         @param str The ostream to encode to.
         @return A reference to str.
      */
      EncodeStream& encode(EncodeStream& str) const;

      /**
         @brief Returns true iff a parse has been attempted.
         @note This means that this will return true if a parse failed earlier.
      */
      bool isParsed() const {return (mState!=NOT_PARSED);}

      /**
         @internal
      */
      HeaderFieldValue& getHeaderField() { return mHeaderField; }

      // call (internally) before every access 
      /**
         @internal
      */
      void checkParsed() const
      {
         if (mState==NOT_PARSED)
         {
            doParse();
         }
      }
      void checkParsed()
      {
         const LazyParser* constThis = const_cast<const LazyParser*>(this);
         constThis->checkParsed();
         mState=DIRTY;
      }
      void doParse() const;
      inline void markDirty() const {mState=DIRTY;}
      
      /**
         @brief Returns true iff this element was parsed successfully, according
            to the implementation of parse().

         App writers are expected to use this extensively, since a failure in
         the parse-on-access will throw an exception the _first_ time access is
         made, but nothing will happen on subsequent accesses. (Writing a 
         try/catch block when you try to inspect the members of a subclass will 
         only work as intended if the subclass hasn't been accessed before.)
      */
      bool isWellFormed() const;
   protected:
      LazyParser();

      // called in destructor and on assignment 
      void clear();

      // context for error messages
      virtual const Data& errorContext() const = 0;
      
   private:
      // !dlb! bit of a hack until the dust settles
      friend class Contents;

      HeaderFieldValue mHeaderField;

      typedef enum
      {
         NOT_PARSED,
         WELL_FORMED, // Parsed, well-formed, but underlying buffer is still valid
         MALFORMED, // Parsed, malformed, underlying buffer is still valid
         DIRTY // Well-formed, and underlying buffer is invalid
      } ParseState;
      mutable ParseState mState;
};

#ifndef  RESIP_USE_STL_STREAMS
EncodeStream&
operator<<(EncodeStream&, const LazyParser& lp);
#endif

//need this for MD5Stream
std::ostream&
operator<<(std::ostream&, const LazyParser& lp);

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
