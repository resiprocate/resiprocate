#ifndef RESIP_Contents_hxx
#define RESIP_Contents_hxx

#include <iosfwd>
#include <vector>

#include "resip/stack/LazyParser.hxx"
#include "resip/stack/Mime.hxx"
#include "resip/stack/StringCategory.hxx"
#include "resip/stack/Headers.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/ContentsFactory.hxx"

namespace resip
{

class Token;
class Contents;
class HeaderFieldValue;
class ParseBuffer;

/** @brief MIME header identifier base class */
class MIME_Header
{
};

/** @brief Content-ID MIME header identifier class */
class H_ContentID : public MIME_Header
{
   public:
      typedef Token Type;
};
extern H_ContentID h_ContentID;

/** @brief Content-ID MIME header identifier class */
class H_ContentDescription : public MIME_Header
{
   public:
      typedef StringCategory Type;
};
extern H_ContentDescription h_ContentDescription;
      

/**
   @ingroup resip_crit
   @ingroup sip_payload
   @brief Base class for all SIP body types. Lazily-parsed.
*/
class Contents : public LazyParser
{
   public:
      /// pass Mime instance for parameters
      Contents(const HeaderFieldValue& headerFieldValue, const Mime& contentsType);
      /**
      @brief Create a contents with a given mime type
      @param contentsType the mime type of the contents
      */
      Contents(const Mime& contentsType);
      /**
      @brief Copy constructor
      @param rhs the Contents to be copied
      */
      Contents(const Contents& rhs);
      /**
      @internal
      @todo internal documentation
      */
      Contents(const Contents& rhs,HeaderFieldValue::CopyPaddingEnum e);
      /**
      @internal
      @todo internal documentation
      */
      Contents(const HeaderFieldValue& headerFieldValue,
               HeaderFieldValue::CopyPaddingEnum e,
               const Mime& contentsType);
      virtual ~Contents();
      /**
      @brief Assignment operator
      @param rhs Contents object to be copied
      @return a reference to a copy of rhs
      */
      Contents& operator=(const Contents& rhs);
      /**
      @brief Preforms preparsing on the headers stored in the ParseBuffer pb
      @param pb a ParseBuffer containing the headers to preparse
      */
      void preParseHeaders(ParseBuffer& pb);
      /**
      @brief encodes headers to an output stream
      @param str output stream destination for encoded headers
      @return a reference to str
      */
      EncodeStream& encodeHeaders(EncodeStream& str) const;

      /**
        @brief access to wrapped contents (signed, encrypted)
        @return a pointer to self
      */
      virtual Contents* getContents() {return this;}

      /**
        @internal
        @todo - unimplemented - decide fate
      */
      Contents* getContents(const Mime&);

      virtual Data getBodyData() const;

      /**
        @brief returns a copy of a Contents object
      */
      virtual Contents* clone() const = 0;
      /**
        @brief getter for mime type of message
        @return the mime type of the message
      */
      const Mime& getType() const {return mType;}
      /**
        @brief factory method to create a Contents object from a mime type and a payload
        @param contentType the mime type of the contents
        @param contents the contents
        @return a instance of a Contents subclass appropriate to the mime type
        @note If no registered Contents subclass has been registered with the factory
        an instance of OctetContents is returned.
        @sa ContentsFactory
        @sa ContentsFactoryBase
      */
      static Contents* createContents(const Mime& contentType, 
                                      const Data& contents);
      /**
        @brief checks to see if a header is present
        @param headerType the header to check for
        @return true if that header exists and false otherwise
      */
      bool exists(const HeaderBase& headerType) const;
      /**
        @brief removes a header if it is present
        @param headerType the header to remove
      */
      void remove(const HeaderBase& headerType);
      /**
        @brief checks to see if a MIME header exists
        @param headerType the MIME header to check for
        @return true if the header exists and false otherwise
      */
      bool exists(const MIME_Header& headerType) const;
      /**
        @brief removes a MIME header if it is present
        @param headerType the MIME header to remove
      */
      void remove(const MIME_Header& headerType);

      // !dlb! break into const and non-const -- throw on const if not exists
      // !dlb! requires a nested exception...

      // shared header types
      /**
        @brief returns the value of the Content-Type header
        Throws an Contents::Exception if the header doesn't exist.
		
		@code
		retval = contents.header(Headers::ContentType);
        @endcode
		
        @return the Content-Type header value
      */
      const H_ContentType::Type& header(const H_ContentType& headerType) const;
      /**
        @brief returns the value of the Content-Type header
		
		@code
		retval = contents.header(Headers::ContentType);
        @endcode
		
        @return the Content-Type header value
      */
      H_ContentType::Type& header(const H_ContentType& headerType);

      /**
        @brief returns the value of the Content-Disposition header
        Throws an Contents::Exception if the header doesn't exist.
        @code
        retval = contents.header(Headers::ContentDisposition);
        @endcode
        @return the Content-Disposition header value
      */      
      const H_ContentDisposition::Type& header(const H_ContentDisposition& headerType) const;
      /**
        @brief returns the value of the Content-Disposition header
        @code
        retval = contents.header(Headers::ContentDisposition);
        @endcode
        @return the Content-Disposition header value
      */      
      H_ContentDisposition::Type& header(const H_ContentDisposition& headerType);

      /**
        @brief returns the value of the Content-Transfer-Encoding header
        Throws an Contents::Exception if the header doesn't exist.
        @code
        retval = contents.header(Headers::ContentTransferEncoding);
        @endcode
        @return the Content-Transfer-Encoding header value
      */
      const H_ContentTransferEncoding::Type& header(const H_ContentTransferEncoding& headerType) const;
      /**
        @brief returns the value of the Content-Transfer-Encoding header
        @code
        retval = contents.header(Headers::ContentTransferEncoding);
        @endcode
        @return the Content-Transfer-Encoding header value
      */
      H_ContentTransferEncoding::Type& header(const H_ContentTransferEncoding& headerType);

      /**
        @brief returns the value of the Content-Languages header
        Throws an Contents::Exception if the header doesn't exist.
        @code
        retval = contents.header(Headers::ContentLanguages);
        @endcode
        @return the Content-Languages header value
      */
      const H_ContentLanguages::Type& header(const H_ContentLanguages& headerType) const;
      /**
        @brief returns the value of the Content-Languages header
        @code
        retval = contents.header(Headers::ContentLanguages);
        @endcode
        @return the Content-Languages header value
      */
      H_ContentLanguages::Type& header(const H_ContentLanguages& headerType);

      // MIME specific header types
      /**
        @brief returns the value of the Content-ID MIME header
        Throws an Contents::Exception if the header doesn't exist.
        @code
        retval = contents.header(Headers::ContentId);
        @endcode
        @return the Content-Id MIME header value
      */
      const H_ContentID::Type& header(const H_ContentID& headerType) const;
      /**
        @brief returns the value of the Content-ID MIME header
        @code
        retval = contents.header(Headers::ContentId);
        @endcode
        @return the Content-Id MIME header value
      */
      H_ContentID::Type& header(const H_ContentID& headerType);

      /**
        @brief returns the Content-Description MIME header
        Throws an Contents::Exception if the header doesn't exist.
        @code
        retval = contents.header(Headers::ContentDescription)
        @endcode
        @return the Content-Description MIME header
      */
      const H_ContentDescription::Type& header(const H_ContentDescription& headerType) const;
      /**
        @brief returns the Content-Description MIME header
        @code
        retval = contents.header(Headers::ContentDescription)
        @endcode
        @return the Content-Description MIME header
      */
      H_ContentDescription::Type& header(const H_ContentDescription& headerType);

      /**
        @brief returns the major version of MIME used by the contents
        @return MIME major version
      */
      int& version() {return mVersion;}
      /**
        @brief returns the minor version of MIME used by the contents
        @return MIME minor version
      */
      int& minorVersion() {return mMinorVersion;}
      /**
        @internal
        @todo - is this being used? -- is the buffer list being used as a list?
      */
      void addBuffer(char* buf);

   protected:

      /**
        @internal
        @todo !bwc! Calls freeMem(), then reverts members to a default state
         (including setting pointers to 0)
      */
      inline void clear()
      {
         freeMem();
         init();
      }
      
      /** @internal */
      inline void init()
      {
         mBufferList.clear();
         mDisposition = 0;
         mTransferEncoding = 0;
         mLanguages = 0;
         mId = 0;
         mDescription = 0;
         mLength = 0;
         mVersion = 1;
         mMinorVersion = 0;
      }
      /** @internal */
      void init(const Contents& orig);

      // !bwc! Just frees up heap-allocated stuff, doesn't set pointers to 0
      // This exists because it is pointless (and inefficient) to revert 
      // members to a default state while deleting (they're just going to go
      // out of scope anyway) The d'tor is the only thing that uses this by
      // itself. Everything else should use clear()
      /** @internal */
      inline void freeMem()
      {
         delete mDisposition;
         delete mTransferEncoding;
         delete mLanguages;
         delete mId;
         delete mDescription;
         delete mLength;

         for (std::vector<char*>::iterator i = mBufferList.begin();
              i != mBufferList.end(); i++)
         {
            delete [] *i;
         }

      }
      /** @internal */
      virtual const Data& errorContext() const;

      /** @internal */
      Mime mType;
      /** @internal */
      H_ContentDisposition::Type *mDisposition;
      /** @internal */
      H_ContentTransferEncoding::Type *mTransferEncoding;
      /** @internal */
      H_ContentLanguages::Type *mLanguages;
      /** @internal */
      Token *mId;
      /** @internal */
      H_ContentDescription::Type *mDescription;
      /** @internal */
      StringCategory *mLength;
      
      /** @internal */
      int mVersion;
      /** @internal */
      int mMinorVersion;

      std::vector<char*> mBufferList;
};

bool operator==(const Contents& lhs, const Contents& rhs);
bool operator!=(const Contents& lhs, const Contents& rhs);

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
