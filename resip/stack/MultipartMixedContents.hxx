#if !defined(RESIP_MULTIPARTMIXEDCONTENTS_HXX)
#define RESIP_MULTIPARTMIXEDCONTENTS_HXX 

#include <vector>

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"

namespace resip
{

class Mime;
class ParseBuffer;

/**
   @ingroup sip_payload
   @brief SIP body type for holding Multipart-Mixed body contents (MIME content-type multipart/mixed).
*/
class MultipartMixedContents : public Contents
{
   public:
      MultipartMixedContents();
      explicit MultipartMixedContents(const Mime& contentType);
      MultipartMixedContents(const HeaderFieldValue& hfv, const Mime& contentType);
      MultipartMixedContents(const MultipartMixedContents& rhs);
      virtual ~MultipartMixedContents();
      MultipartMixedContents& operator=(const MultipartMixedContents& rhs);

      /** @brief duplicate an MultipartMixedContents object
          @return pointer to a new MultipartMixedContents object  
        **/
      virtual Contents* clone() const;

      static const Mime& getStaticType() ;

      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      virtual void parse(ParseBuffer& pb);

      typedef std::vector<Contents*> Parts;
      // Mutable access drops what getRawFirstPart() returns.  A caller that
      // can change the parts must not be handed bytes that claim to be the
      // ones that arrived.
      //
      // Assigning a fresh Data rather than calling clear() on the old one:
      // Data::clear() only sets the size to zero and deliberately keeps the
      // shared pointer, so "empty" and "points nowhere" would otherwise be two
      // different states.
      Parts& parts() {checkParsed(); mRawFirstPart = Data(); return mContents;}
      const Parts& parts() const {checkParsed(); return mContents;}

      /**
         The first body part exactly as it arrived: header block, the empty
         line, and the body.  The CRLF in front of the following boundary is
         not included, since RFC 2046 section 5.1.1 counts it as part of the
         delimiter.

         RFC 1847 requires the signature of a multipart/signed body to cover
         these bytes rather than a re-encoding of the parsed part, so
         BaseSecurity::checkSignature() needs them.  Only the first part is
         kept, because that is the part the signature covers.

         What is kept internally is a view into the buffer this object was
         parsed from, so parsing an ordinary multipart body costs nothing
         extra.  What this function hands back is a self contained copy of it,
         returned by value so that it stays usable after a later call to
         parts(); a reference into the object would go empty under the caller.

         Empty when the bytes are not available: for an object that was built
         locally rather than parsed, and after mutable access through parts().

         A copy is not empty by construction.  The copy constructor does not
         carry the view over, since it points into the buffer of the original,
         but it also does not force the original to parse.  Copying a body that
         is still unparsed therefore leaves a copy that is unparsed as well,
         holding a deep copy of the buffer, and the first access parses that
         buffer and establishes a view into it.  Copying a body that was
         already parsed leaves a copy with the parts and no view, because
         nothing parses a second time.
      */
      Data getRawFirstPart() const;

      void setBoundary(const Data& boundary);

      /**
      @brief Thrown when there is an error parsing a multi-part mixed contents envelope.
      @sa resip::BaseException
      */
      class Exception final : public BaseException
      {
        public:
         Exception(const Data& msg, const Data& file, const int line)
            : BaseException(msg, file, line) {}

         const char* name() const noexcept override { return "MultipartMixedContents::Exception"; }
      };
      
      static bool init();
      
   protected:
      void clear();
      
   private:
      void setBoundary();
      std::vector<Contents*> mContents;
      // A read-only view of the first part inside the buffer this object was
      // parsed from, not a copy of it.  Empty unless this object was parsed,
      // see getRawFirstPart().
      Data mRawFirstPart;
};

static bool invokeMultipartMixedContentsInit = MultipartMixedContents::init();

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
