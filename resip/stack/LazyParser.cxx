#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif


#include <cassert>

#include "resip/stack/Headers.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "resip/stack/LazyParser.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

LazyParser::LazyParser(HeaderFieldValue* headerFieldValue)
   : mHeaderField(headerFieldValue),
      mState(mHeaderField->mField == 0 ? EMPTY : NOT_PARSED),
      mIsMine(false)
{
}
  
LazyParser::LazyParser()
   : mHeaderField(0),
      mState(EMPTY),
     mIsMine(true)
{
}

LazyParser::LazyParser(const LazyParser& rhs)
   : mHeaderField(0),
      mState(rhs.mState),
     mIsMine(true)
{
   if (rhs.mState==NOT_PARSED && rhs.mHeaderField)
   {
      mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
   }
}

LazyParser::LazyParser(const LazyParser& rhs,HeaderFieldValue::CopyPaddingEnum e)
   :  mHeaderField(0),
      mState(rhs.mState),
      mIsMine(true)
{
   if (rhs.mState==NOT_PARSED && rhs.mHeaderField)
   {
      mHeaderField = new HeaderFieldValue(*rhs.mHeaderField,e);
   }

}


LazyParser::~LazyParser()
{
   clear();
}

LazyParser&
LazyParser::operator=(const LazyParser& rhs)
{
   assert( &rhs != 0 );
   
   if (this != &rhs)
   {
      clear();
      mState = rhs.mState;
      if (rhs.mState!=NOT_PARSED)
      {
         mHeaderField = 0;
         mIsMine = false;
      }
      else
      {
         mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
         mIsMine = true;
      }
   }
   return *this;
}

void
LazyParser::checkParsed() const
{
   if (mState==NOT_PARSED)
   {
      LazyParser* ncThis = const_cast<LazyParser*>(this);
      // !bwc! We assume the worst, and if the parse succeeds, we update.
      ncThis->mState = MALFORMED;
      assert(mHeaderField);
      ParseBuffer pb(mHeaderField->mField, mHeaderField->mFieldLength, errorContext());
      ncThis->parse(pb);
      // !bwc! If we get this far without throwing, the parse has succeeded.
      ncThis->mState = WELL_FORMED;
   }
}

bool
LazyParser::isWellFormed() const
{
   try
   {
      checkParsed();
   }
   catch(resip::ParseBuffer::Exception&)
   {
   }
   
   return (mState!=MALFORMED);
}

void
LazyParser::clear()
{
   if (mIsMine)
   {
      delete mHeaderField;
      mHeaderField = 0;
   }
}

std::ostream&
LazyParser::encode(std::ostream& str) const
{
   if (isParsed())
   {
      return encodeParsed(str);
   }
   else
   {
      assert(mHeaderField);
      mHeaderField->encode(str);
      return str;
   }
}

std::ostream&
resip::operator<<(std::ostream& s, const LazyParser& lp)
{
   lp.encode(s);
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
