#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/MediaControlContents.hxx"
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

//const MediaControlContents MediaControlContents::Empty;

bool
MediaControlContents::init()
{
   static ContentsFactory<MediaControlContents> factory;
   (void)factory;
   return true;
}

MediaControlContents::MediaControlContents() : Contents(getStaticType())
{
}

MediaControlContents::MediaControlContents(const HeaderFieldValue& hfv, const Mime& contentTypes)
   : Contents(hfv, contentTypes)
{
}

MediaControlContents::~MediaControlContents()
{
}

MediaControlContents&
MediaControlContents::operator=(const MediaControlContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mMediaControl = rhs.mMediaControl;
   }
   return *this;
}

Contents*
MediaControlContents::clone() const
{
   return new MediaControlContents(*this);
}

void
MediaControlContents::parse(ParseBuffer& pb)
{
   mMediaControl.parse(pb);
}

EncodeStream&
MediaControlContents::encodeParsed(EncodeStream& s) const
{
   mMediaControl.encode(s);
   return s;
}

const Mime&
MediaControlContents::getStaticType()
{
   static Mime type("application", "media_control+xml");
   return type;
}

MediaControlContents::MediaControl::VCPrimitive::VCPrimitive(const StreamIDList& streamIDs, bool pictureFastUpdate)
   : mStreamIDs(streamIDs),
     mPictureFastUpdate(pictureFastUpdate)
{
}

MediaControlContents::MediaControl::VCPrimitive::VCPrimitive(const VCPrimitive& rhs)
{
   *this = rhs;
}

MediaControlContents::MediaControl::VCPrimitive&
MediaControlContents::MediaControl::VCPrimitive::operator=(const VCPrimitive& rhs)
{
   if (this != &rhs)
   {
      mStreamIDs = rhs.mStreamIDs;
      mPictureFastUpdate = rhs.mPictureFastUpdate;
   }
   return *this;
}

MediaControlContents::MediaControl::MediaControl(const VCPrimitive::StreamIDList& streamIDs, bool pictureFastUpdate)
{
   VCPrimitive p(streamIDs, pictureFastUpdate);
   mVCPrimitives.push_back(p);
}

MediaControlContents::MediaControl::MediaControl(const GeneralErrorList& generalErrors)
   : mGeneralErrors(generalErrors)
{
}

MediaControlContents::MediaControl::MediaControl()
{
}

MediaControlContents::MediaControl::MediaControl(const MediaControl& rhs)
{
   *this = rhs;
}

MediaControlContents::MediaControl&
MediaControlContents::MediaControl::operator=(const MediaControl& rhs)
{
   if (this != &rhs)
   {
      mGeneralErrors = rhs.mGeneralErrors;
      mVCPrimitives = rhs.mVCPrimitives;
   }
   return *this;
}

void
MediaControlContents::MediaControl::VCPrimitive::parseVCPrimitive(XMLCursor& xml)
{
   if(xml.firstChild())
   {
      do
      {
         if (xml.getTag().caseInsensitiveTokenCompare(Data("to_encoder")))
         {
            if(xml.firstChild())
            {
               do
               {
                  if (xml.getTag().caseInsensitiveTokenCompare(Data("picture_fast_update")))
                  {
                     mPictureFastUpdate = true;
                  }
                  else
                  {
                     DebugLog(<< "Unknown element: " << xml.getTag());
                  }
               }
               while(xml.nextSibling());
               xml.parent();
            }
         }
         else if (xml.getTag().caseInsensitiveTokenCompare(Data("stream_id")))
         {
            xml.firstChild();
            mStreamIDs.insert(xml.getValue());
            xml.parent();
         }
         else
         {
            DebugLog(<< "Unknown element: " << xml.getTag());
         }
      } while (xml.nextSibling());
      xml.parent();
   }
}

void
MediaControlContents::MediaControl::parseMediaControl(XMLCursor& xml)
{
   if(xml.firstChild())
   {
      do
      {
         if (xml.getTag().caseInsensitiveTokenCompare(Data("vc_primitive")))
         {
            VCPrimitive p;
            p.parseVCPrimitive(xml);
            mVCPrimitives.push_back(p);
         }
         else if (xml.getTag().caseInsensitiveTokenCompare(Data("general_error")))
         {
            xml.firstChild();
            mGeneralErrors.push_back(xml.getValue());
            xml.parent();
         }
         else
         {
            DebugLog(<< "Unknown element: " << xml.getTag());
         }
      } while (xml.nextSibling());
   }
}

void
MediaControlContents::MediaControl::parse(ParseBuffer& pb)
{
   XMLCursor xml(pb);
   if(!xml.atLeaf())
   {
      do
      {
         if (xml.getTag().caseInsensitiveTokenCompare(Data("media_control")))
         {
            parseMediaControl(xml);
         }
         else
         {
            DebugLog(<< "Unknown root element: " << xml.getTag());
         }
      } while (xml.nextSibling());
   }
}

EncodeStream&
MediaControlContents::MediaControl::VCPrimitive::encode(EncodeStream& s) const
{
   s << "<vc_primitive>";
   if(mPictureFastUpdate)
   {
      s << "<to_encoder><picture_fast_update></picture_fast_update></to_encoder>";
   }
   StreamIDList::const_iterator it = mStreamIDs.cbegin();
   for (; it != mStreamIDs.cend(); it++)
   {
      s << "<stream_id>" << *it << "</stream_id>";
   }
   s << "</vc_primitive>";
   return s;
}

EncodeStream&
MediaControlContents::MediaControl::encode(EncodeStream& s) const
{
   s << "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
   s << "<media_control>";
   if(isError())
   {
      GeneralErrorList::const_iterator it = mGeneralErrors.cbegin();
      for( ; it != mGeneralErrors.cend(); it++)
      {
         s << "<general_error>" << *it << "</general_error>";
      }
   }
   else
   {
      VCPrimitiveList::const_iterator it = mVCPrimitives.cbegin();
      for( ; it != mVCPrimitives.cend(); it++)
      {
         it->encode(s);
      }
   }
   s << "</media_control>";
   return s;
}

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

