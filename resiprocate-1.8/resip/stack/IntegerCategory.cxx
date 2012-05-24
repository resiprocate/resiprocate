#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/IntegerCategory.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// Integer:
//====================
IntegerCategory::IntegerCategory()
   : ParserCategory(), 
     mValue(0), 
     mComment() 
{}

IntegerCategory::IntegerCategory(const HeaderFieldValue& hfv, 
                                 Headers::Type type,
                                 PoolBase* pool)
   : ParserCategory(hfv, type, pool), 
     mValue(0), 
     mComment() 
{}

IntegerCategory::IntegerCategory(const IntegerCategory& rhs,
                                 PoolBase* pool)
   : ParserCategory(rhs, pool),
     mValue(rhs.mValue),
     mComment(rhs.mComment)
{}

IntegerCategory&
IntegerCategory::operator=(const IntegerCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
      mComment = rhs.mComment;
   }
   return *this;
}

ParserCategory* IntegerCategory::clone() const
{
   return new IntegerCategory(*this);
}

ParserCategory* IntegerCategory::clone(void* location) const
{
   return new (location) IntegerCategory(*this);
}

ParserCategory* 
IntegerCategory::clone(PoolBase* pool) const
{
   return new (pool) IntegerCategory(*this, pool);
}

int IntegerCategory::value() const 
{
   checkParsed(); 
   return mValue;
}

const Data& 
IntegerCategory::comment() const 
{
   checkParsed(); 
   return mComment;
}

int& IntegerCategory::value()
{
   checkParsed(); 
   return mValue;
}

Data& 
IntegerCategory::comment()
{
   checkParsed(); 
   return mComment;
}


void
IntegerCategory::parse(ParseBuffer& pb)
{
   const char* start = pb.skipWhitespace();
   mValue = pb.integer();
   pb.skipToChar('(');
   if (!pb.eof())
   {
      start = pb.skipChar();
      pb.skipToEndQuote(')');
      pb.data(mComment, start);
      pb.skipChar();
   }
   else
   {
      pb.reset(start);
      start = pb.skipNonWhitespace();
   }
   
   parseParameters(pb);
}

EncodeStream& 
IntegerCategory::encodeParsed(EncodeStream& str) const
{
  str << mValue;

  if (!mComment.empty())
  {
     str << "(" << mComment << ")";
  }
  
  encodeParameters(str);
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
