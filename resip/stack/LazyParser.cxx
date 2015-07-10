#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif


#include "rutil/ResipAssert.h"

#include "resip/stack/Headers.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "resip/stack/LazyParser.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

LazyParser::LazyParser(const HeaderFieldValue& headerFieldValue)
   : mHeaderField(headerFieldValue, HeaderFieldValue::NoOwnership),
      mState(mHeaderField.getBuffer() == 0 ? DIRTY : NOT_PARSED)
{
}

LazyParser::LazyParser(const HeaderFieldValue& headerFieldValue,
                        HeaderFieldValue::CopyPaddingEnum e)
   : mHeaderField(headerFieldValue, e), // Causes ownership to be taken. Oh well
      mState(mHeaderField.getBuffer() == 0 ? DIRTY : NOT_PARSED)
{}

LazyParser::LazyParser(const char* buf, int length) :
   mHeaderField(buf, length),
   mState(buf == 0 ? DIRTY : NOT_PARSED)
{}

LazyParser::LazyParser()
   : mHeaderField(),
      mState(DIRTY)
{
}

LazyParser::LazyParser(const LazyParser& rhs)
   : mHeaderField((rhs.mState==DIRTY ? HeaderFieldValue::Empty : rhs.mHeaderField)), // Pretty cheap when rhs is DIRTY
      mState(rhs.mState)
{}

LazyParser::LazyParser(const LazyParser& rhs,HeaderFieldValue::CopyPaddingEnum e)
   :  mHeaderField((rhs.mState==DIRTY ? HeaderFieldValue::Empty : rhs.mHeaderField), e), // Pretty cheap when rhs is DIRTY
      mState(rhs.mState)
{}


LazyParser::~LazyParser()
{
   clear();
}

LazyParser&
LazyParser::operator=(const LazyParser& rhs)
{
   resip_assert( &rhs != 0 );
   
   if (this != &rhs)
   {
      clear();
      mState = rhs.mState;
      if (rhs.mState!=DIRTY)
      {
         mHeaderField=rhs.mHeaderField;
      }
   }
   return *this;
}

void
LazyParser::doParse() const
{
   LazyParser* ncThis = const_cast<LazyParser*>(this);
   // .bwc. We assume the worst, and if the parse succeeds, we update.
   ncThis->mState = MALFORMED;
   ParseBuffer pb(mHeaderField.getBuffer(), mHeaderField.getLength(), errorContext());
   ncThis->parse(pb);
   // .bwc. If we get this far without throwing, the parse has succeeded.
   ncThis->mState = WELL_FORMED;
}

bool
LazyParser::isWellFormed() const
{
   try
   {
      checkParsed();
   }
   catch(resip::ParseException&)
   {
   }
   
   return (mState!=MALFORMED);
}

void
LazyParser::clear()
{
   mHeaderField.clear();
}

EncodeStream&
LazyParser::encode(EncodeStream& str) const
{
   if (mState == DIRTY)
   {
      return encodeParsed(str);
   }
   else
   {
      mHeaderField.encode(str);
      return str;
   }
}

#ifndef  RESIP_USE_STL_STREAMS
EncodeStream&
resip::operator<<(EncodeStream&s, const LazyParser& lp)
{
   lp.encode(s);
   return s; 
}
#endif

std::ostream&
resip::operator<<(std::ostream &s, const LazyParser& lp)
{	
#ifdef  RESIP_USE_STL_STREAMS
	lp.encode(s);
#else
	//this should only be called for things like cout,cerr, or other streams not supporting
	//other stream encoders, aka MD5Stream
	Data data;
	DataStream stream(data);

	lp.encode(stream);
	stream.flush();
	s << data.c_str();
#endif
   return s; 
}
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
