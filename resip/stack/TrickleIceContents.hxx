#if !defined(RESIP_TRICKLEICECONTENTS_HXX)
#define RESIP_TRICKLEICECONTENTS_HXX

#include "resip/stack/Contents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/XMLCursor.hxx"

namespace resip
{

/* Provides a way to handle the application/trickle-ice-sdpfrag
   content-type from SIP INFO messages.
   Attempts to adhere to the IETF RFC 8840
     https://datatracker.ietf.org/doc/html/rfc8840
*/

class TrickleIceContents : public Contents
{

   public:

      TrickleIceContents();
      TrickleIceContents(const HeaderFieldValue& hfv, const Mime& contentTypes);
      TrickleIceContents(const TrickleIceContents& rhs);
      virtual ~TrickleIceContents();

      TrickleIceContents& operator=(const TrickleIceContents& rhs);

      /** @brief duplicate an TrickleIceContents object
        *
        * @return pointer to a new TrickleIceContents object
        **/
      virtual Contents* clone() const override;

      virtual EncodeStream& encodeParsed(EncodeStream& str) const override;
      virtual void parse(ParseBuffer& pb) override;
      static const Mime& getStaticType();

      void addAttribute(const Data& key, const Data& value = Data::Empty);
      void addMedium(const SdpContents::Session::Medium& medium);

      static bool init();

      const SdpContents::Session::MediumContainer& media() const {checkParsed(); return mMedia;}

      const std::list<Data>& getValues(const Data& key) const;

   private:
      AttributeHelper mAttributeHelper;
      SdpContents::Session::MediumContainer mMedia;
};

static bool invokeTrickleIceContentsInit = TrickleIceContents::init();

}


#endif

/* ====================================================================
 *
 * Copyright (c) 2022, Daniel Pocock, https://danielpocock.com
 * Copyright (c) 2022, Software Freedom Institute LLC, https://softwarefreedom.institute
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

