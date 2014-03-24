#if !defined(RESIP_DTMFPAYLOADCONTENTS_HXX)
#define RESIP_DTMFPAYLOADCONTENTS_HXX

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/ParseBuffer.hxx"

namespace resip
{

/* Provides a way to handle the application/dtmf-relay
   content-type from SIP INFO messages.
   Attempts to adhere to the IETF draft
     draft-kaplan-dispatch-info-dtmf-package-00
*/

class DtmfPayloadContents : public Contents
{

   public:

      class DtmfPayload
      {
         public:
            DtmfPayload(char button, int duration);

            DtmfPayload() : mButton(0), mDuration(0) {}
            DtmfPayload(const DtmfPayload& rhs);
            DtmfPayload& operator=(const DtmfPayload& rhs);

            void parse(ParseBuffer& pb);
            EncodeStream& encode(EncodeStream&) const;

            /** @brief obtain representation of the button as a character
              *
              * @return the ASCII symbol for the button pressed
              **/
            char getButton() const { return mButton; }

            /** @brief obtain representation of the button as event code
              *
              *   RFC 4733 provides a list of integer event codes for DTMF
              *   symbols.
              *
              * @return the event code corresponding to the button pressed
              **/
            unsigned short getEventCode() const;

            /** @brief obtain duration in milliseconds
              *
              * @return the number of milliseconds the button was pressed
              **/
            int getDuration() const { return mDuration; }

         private:
            char mButton;    // ASCII representation of the DTMF button
            int mDuration;   // milliseconds

            static bool isValidButton(const char c);

         friend class DtmfPayloadContents;
      };

      DtmfPayloadContents();
      DtmfPayloadContents(const HeaderFieldValue& hfv, const Mime& contentTypes);
      virtual ~DtmfPayloadContents();

      DtmfPayloadContents& operator=(const DtmfPayloadContents& rhs);

      /** @brief duplicate an DtmfPayloadContents object
        *
        * @return pointer to a new DtmfPayloadContents object
        **/
      virtual Contents* clone() const;

      /** @brief get the parsed payload
        *
        * @return parsed payload object
        **/
      DtmfPayload& dtmfPayload() {checkParsed(); return mDtmfPayload;}
      const DtmfPayload& dtmfPayload() const {checkParsed(); return mDtmfPayload;}
      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      virtual void parse(ParseBuffer& pb);
      static const Mime& getStaticType() ;

      static bool init();

   private:
      DtmfPayloadContents(const Data& data, const Mime& contentTypes);
      DtmfPayload mDtmfPayload;
};

static bool invokeDtmfPayloadContentsInit = DtmfPayloadContents::init();

}


#endif

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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

