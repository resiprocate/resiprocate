#if !defined(RESIP_MEDIACONTROLCONTENTS_HXX)
#define RESIP_MEDIACONTROLCONTENTS_HXX

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/XMLCursor.hxx"

namespace resip
{

/* Provides a way to handle the application/media_control+xml
   content-type from SIP INFO messages.
   Attempts to adhere to the IETF RFC 5168
     https://datatracker.ietf.org/doc/html/rfc5168
*/

class MediaControlContents : public Contents
{

   public:

      class MediaControl
      {
         public:

            class VCPrimitive
            {
               public:
                  typedef std::set<Data> StreamIDList;
                  VCPrimitive(const StreamIDList& streamIDs, bool pictureFastUpdate);
                  VCPrimitive() : mPictureFastUpdate(false) {};

                  VCPrimitive(const VCPrimitive& rhs);
                  VCPrimitive& operator=(const VCPrimitive& rhs);

                  void parseVCPrimitive(XMLCursor& xml);
                  EncodeStream& encode(EncodeStream&) const;

                  const StreamIDList& streamIDs() const { return mStreamIDs; };
                  StreamIDList& streamIDs() { return mStreamIDs; };

                  const bool& pictureFastUpdate() const { return mPictureFastUpdate; };
                  bool& pictureFastUpdate() { return mPictureFastUpdate; };

               private:
                  StreamIDList mStreamIDs;
                  bool mPictureFastUpdate;
            };

            typedef std::vector<Data> GeneralErrorList;

            typedef std::vector<VCPrimitive> VCPrimitiveList;

            MediaControl(const VCPrimitive::StreamIDList& streamIDs, bool pictureFastUpdate);
            MediaControl(const GeneralErrorList& generalErrors);
            MediaControl();

            MediaControl(const MediaControl& rhs);
            MediaControl& operator=(const MediaControl& rhs);

            void parseMediaControl(XMLCursor& xml);
            void parse(ParseBuffer& pb);
            EncodeStream& encode(EncodeStream&) const;

            bool isError() const { return !mGeneralErrors.empty(); };
            const GeneralErrorList& generalErrors() const { return mGeneralErrors; };
            
            const VCPrimitiveList& vCPrimitives() const { return mVCPrimitives; };
            VCPrimitiveList& vCPrimitives() { return mVCPrimitives; };

         private:
            GeneralErrorList mGeneralErrors;
            VCPrimitiveList mVCPrimitives;

         friend class MediaControlContents;
      };

      MediaControlContents();
      MediaControlContents(const HeaderFieldValue& hfv, const Mime& contentTypes);
      virtual ~MediaControlContents();

      MediaControlContents& operator=(const MediaControlContents& rhs);

      /** @brief duplicate an MediaControlContents object
        *
        * @return pointer to a new MediaControlContents object
        **/
      virtual Contents* clone() const;

      /** @brief get the parsed payload
        *
        * @return parsed payload object
        **/
      MediaControl& mediaControl() {checkParsed(); return mMediaControl;}
      const MediaControl& mediaControl() const {checkParsed(); return mMediaControl;}
      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      virtual void parse(ParseBuffer& pb);
      static const Mime& getStaticType() ;

      static bool init();

   private:
      MediaControlContents(const Data& data, const Mime& contentTypes);
      MediaControl mMediaControl;
};

static bool invokeMediaControlContentsInit = MediaControlContents::init();

}


#endif

/* ====================================================================
 *
 * Copyright (c) 2021, Daniel Pocock, https://danielpocock.com
 * Copyright (c) 2021, Software Freedom Institute SA, https://softwarefreedom.institute
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

