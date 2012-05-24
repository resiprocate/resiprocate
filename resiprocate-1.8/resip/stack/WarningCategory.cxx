#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/WarningCategory.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// WarningCategory
//====================
WarningCategory::WarningCategory()
   : ParserCategory() 
{}

WarningCategory::WarningCategory(const HeaderFieldValue& hfv, 
                                 Headers::Type type,
                                 PoolBase* pool)
   : ParserCategory(hfv, type, pool) 
{}

WarningCategory::WarningCategory(const WarningCategory& rhs,
                                 PoolBase* pool)
   : ParserCategory(rhs, pool),
     mCode(rhs.mCode),
     mHostname(rhs.mHostname),
     mText(rhs.mText)
{}

WarningCategory&
WarningCategory::operator=(const WarningCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mCode = rhs.mCode;
      mHostname = rhs.mHostname;
      mText = rhs.mText;
   }
   return *this;
}
void
WarningCategory::parse(ParseBuffer& pb)
{
   pb.skipWhitespace();
   mCode = pb.uInt32();

   if(mCode >= 1000 || mCode < 100)
   {
      pb.fail(__FILE__,__LINE__,"Warning code does not have exactly three digits.");
   }

   const char* anchor = pb.skipWhitespace();
   pb.skipNonWhitespace();
   pb.data(mHostname, anchor);

   pb.skipWhitespace();
   anchor = pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
   pb.skipToEndQuote(Symbols::DOUBLE_QUOTE[0]);
   pb.data(mText, anchor);
   anchor = pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
   pb.skipWhitespace();
   pb.assertEof();
}

ParserCategory* 
WarningCategory::clone() const
{
   return new WarningCategory(*this);
}

ParserCategory* 
WarningCategory::clone(void* location) const
{
   return new (location) WarningCategory(*this);
}

ParserCategory* 
WarningCategory::clone(PoolBase* pool) const
{
   return new (pool) WarningCategory(*this, pool);
}

int&
WarningCategory::code()
{
   checkParsed();
   return mCode;
}

int
WarningCategory::code() const
{
   checkParsed();
   return mCode;
}

Data&
WarningCategory::hostname()
{
   checkParsed();
   return mHostname;
}

const Data&
WarningCategory::hostname() const
{
   checkParsed();
   return mHostname;
}

Data&
WarningCategory::text()
{
   checkParsed();
   return mText;
}

const Data&
WarningCategory::text() const
{
   checkParsed();
   return mText;
}

EncodeStream& 
WarningCategory::encodeParsed(EncodeStream& str) const
{
   str << mCode << Symbols::SPACE[0];
   str << mHostname << Symbols::SPACE[0];
   str << Symbols::DOUBLE_QUOTE[0] << mText << Symbols::DOUBLE_QUOTE[0];

   return str;
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
