#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/DtmfPayloadContents.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/DataStream.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SDP

using namespace resip;
using namespace std;

//const DtmfPayloadContents DtmfPayloadContents::Empty;

// RFC2327 6. page 9
// "parsers should be tolerant and accept records terminated with a single
// newline character"
inline void skipEol(ParseBuffer& pb)
{
   while(!pb.eof() && (*pb.position() == Symbols::SPACE[0] ||
                       *pb.position() == Symbols::TAB[0]))
   {
      pb.skipChar();
   }

   if (*pb.position() == Symbols::LF[0])
   {
      pb.skipChar();
   }
   else
   {
      // allow extra 0x0d bytes.
      while(*pb.position() == Symbols::CR[0])
      {
         pb.skipChar();
      }
      pb.skipChar(Symbols::LF[0]);
   }

}

bool
DtmfPayloadContents::init()
{
   static ContentsFactory<DtmfPayloadContents> factory;
   (void)factory;
   return true;
}

DtmfPayloadContents::DtmfPayloadContents() : Contents(getStaticType())
{
}

DtmfPayloadContents::DtmfPayloadContents(const HeaderFieldValue& hfv, const Mime& contentTypes)
   : Contents(hfv, contentTypes)
{
}

DtmfPayloadContents::~DtmfPayloadContents()
{
}

DtmfPayloadContents&
DtmfPayloadContents::operator=(const DtmfPayloadContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mDtmfPayload = rhs.mDtmfPayload;
   }
   return *this;
}

Contents*
DtmfPayloadContents::clone() const
{
   return new DtmfPayloadContents(*this);
}

void
DtmfPayloadContents::parse(ParseBuffer& pb)
{
   mDtmfPayload.parse(pb);
}

EncodeStream&
DtmfPayloadContents::encodeParsed(EncodeStream& s) const
{
   mDtmfPayload.encode(s);
   return s;
}

const Mime&
DtmfPayloadContents::getStaticType()
{
   static Mime type("application", "dtmf-relay");
   return type;
}

DtmfPayloadContents::DtmfPayload::DtmfPayload(char button, int duration)
   : mButton(button),
     mDuration(duration)
{}

DtmfPayloadContents::DtmfPayload::DtmfPayload(const DtmfPayload& rhs)
{
   *this = rhs;
}

DtmfPayloadContents::DtmfPayload&
DtmfPayloadContents::DtmfPayload::operator=(const DtmfPayload& rhs)
{
   if (this != &rhs)
   {
      mButton = rhs.mButton;
      mDuration = rhs.mDuration;
   }
   return *this;
}

bool
DtmfPayloadContents::DtmfPayload::isValidButton(const char c)
{
   static const char* permittedChars = "ABCD*#";
   if(isdigit(c))
   {
      return true;
   }
   if(strchr(permittedChars, c) != NULL)
   {
      return true;
   }
   WarningLog(<<"Not a valid DTMF button: " << c);
   return false;
}

void
DtmfPayloadContents::DtmfPayload::parse(ParseBuffer& pb)
{
   pb.skipToChars(Symbols::EQUALS);
   pb.skipChar();
   const char* anchor = pb.skipWhitespace();
   pb.skipToOneOf(Symbols::CRLF);
   Data val;
   pb.data(val, anchor);
   if(val.size() != 1)
   {
      ErrLog(<<"signal string [" << val << "], size = " << val.size());
      throw ParseException("Exactly one button character expected in SIP INFO", pb.getContext(), __FILE__, __LINE__);
   }
   const char& _button = val[0];
   if(!isValidButton(_button))
   {
      throw ParseException("Invalid DTMF button character found", pb.getContext(), __FILE__, __LINE__);
   }
   StackLog(<< "Button=" << _button);

   skipEol(pb);

   pb.skipToChars(Symbols::EQUALS);
   pb.skipChar();
   pb.skipWhitespace();
   mDuration = pb.integer();

   StackLog(<< "Duration = " << mDuration);

   mButton = _button;
}

unsigned short
DtmfPayloadContents::DtmfPayload::getEventCode() const
{
   assert(mButton);
   unsigned short eventCode;
   if(isdigit(mButton))
   {
      eventCode = mButton - '0';
   }
   else if(mButton == '*')
   {      
      eventCode = 10;
   }      
   else if(mButton == '#')
   {      
      eventCode = 11;
   }
   else if(mButton >= 'A' && mButton <= 'D')
   {
      eventCode = 12 + mButton - 'A';
   }
   else
   {
      assert(0);  // unexpected button, should have been caught by the parser
   }

   return eventCode;
}

EncodeStream&
DtmfPayloadContents::DtmfPayload::encode(EncodeStream& s) const
{
   s << "Signal=" << mButton << Symbols::CRLF;
   s << "Duration=" << mDuration << Symbols::CRLF;
   return s;
}

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

