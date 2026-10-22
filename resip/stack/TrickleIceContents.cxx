#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/TrickleIceContents.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/SdpContents.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SDP

using namespace resip;
using namespace std;

//const TrickleIceContents TrickleIceContents::Empty;

bool
TrickleIceContents::init()
{
   static ContentsFactory<TrickleIceContents> factory;
   (void)factory;
   return true;
}

TrickleIceContents::TrickleIceContents() : Contents(getStaticType())
{
}

TrickleIceContents::TrickleIceContents(const HeaderFieldValue& hfv, const Mime& contentTypes)
   : Contents(hfv, contentTypes)
{
}

TrickleIceContents::TrickleIceContents(const TrickleIceContents& rhs)
   : Contents(rhs)
{
   *this = rhs;
}

TrickleIceContents::~TrickleIceContents()
{
}

TrickleIceContents&
TrickleIceContents::operator=(const TrickleIceContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mMedia = rhs.mMedia;
      mAttributeHelper = rhs.mAttributeHelper;

      /*for (MediumContainer::iterator i=mMedia.begin(); i != mMedia.end(); ++i)
      {
         i->setSession(this);
      }*/
   }
   return *this;
}

Contents*
TrickleIceContents::clone() const
{
   return new TrickleIceContents(*this);
}

EncodeStream&
TrickleIceContents::encodeParsed(EncodeStream& s) const
{
   mAttributeHelper.encode(s);
   for (SdpContents::Session::MediumContainer::const_iterator i = mMedia.begin();
           i != mMedia.end(); ++i)
   {
      i->encode(s);
   }
   return s;
}

void
TrickleIceContents::addMedium(const SdpContents::Session::Medium& medium)
{
   mMedia.push_back(medium);
   // mMedia.back().setSession(this);
}

void
TrickleIceContents::addAttribute(const Data& key, const Data& value)
{
   mAttributeHelper.addAttribute(key, value);
}

const list<Data>&
TrickleIceContents::getValues(const Data& key) const
{
   checkParsed();
   if (mAttributeHelper.exists(key))
   {
      return mAttributeHelper.getValues(key);
   }
   //if (!mSession)
   {
      resip_assert(false);
      static list<Data> error;
      return error;
   }
   //return mSession->getValues(key);
}

void
TrickleIceContents::parse(ParseBuffer& pb)
{
   mAttributeHelper.parse(pb);

   while (!pb.eof() && *pb.position() == 'm')
   {
      addMedium(SdpContents::Session::Medium());
      mMedia.back().parse(pb);
   }
}

const Mime&
TrickleIceContents::getStaticType()
{
   static Mime type("application", "trickle-ice-sdpfrag");
   return type;
}

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

