#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/TokenOrQuotedStringCategory.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//============================
// TokenOrQuotedStringCategory
//============================
TokenOrQuotedStringCategory::TokenOrQuotedStringCategory()
   : ParserCategory(),
     mValue(),
     mQuoted(false)
{}

TokenOrQuotedStringCategory::TokenOrQuotedStringCategory(const Data& value,
                                                         bool quoted)
   : ParserCategory(),
     mValue(value),
     mQuoted(quoted)
{}

TokenOrQuotedStringCategory::TokenOrQuotedStringCategory(const HeaderFieldValue& hfv,
                                                         Headers::Type type,
                                                         PoolBase* pool)
   : ParserCategory(hfv, type, pool),
     mValue(),
     mQuoted(false)
{}

TokenOrQuotedStringCategory::TokenOrQuotedStringCategory(const TokenOrQuotedStringCategory& rhs,
                                                         PoolBase* pool)
   : ParserCategory(rhs, pool),
     mValue(rhs.mValue),
     mQuoted(rhs.mQuoted)
{}

TokenOrQuotedStringCategory&
TokenOrQuotedStringCategory::operator=(const TokenOrQuotedStringCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
      mQuoted = rhs.mQuoted;
   }
   return *this;
}

ParserCategory*
TokenOrQuotedStringCategory::clone() const
{
   return new TokenOrQuotedStringCategory(*this);
}

ParserCategory*
TokenOrQuotedStringCategory::clone(void* location) const
{
   return new (location) TokenOrQuotedStringCategory(*this);
}

ParserCategory*
TokenOrQuotedStringCategory::clone(PoolBase* pool) const
{
   return new (pool) TokenOrQuotedStringCategory(*this, pool);
}

bool
TokenOrQuotedStringCategory::isEqual(const TokenOrQuotedStringCategory& rhs) const
{
   return ((value() == rhs.value()) && (isQuoted() == rhs.isQuoted()));
}

bool
TokenOrQuotedStringCategory::operator==(const TokenOrQuotedStringCategory& rhs) const
{
   return ((value() == rhs.value()) && (isQuoted() == rhs.isQuoted()));
}

bool
TokenOrQuotedStringCategory::operator!=(const TokenOrQuotedStringCategory& rhs) const
{
   return ((value() != rhs.value()) || (isQuoted() != rhs.isQuoted()));
}

bool
TokenOrQuotedStringCategory::operator<(const TokenOrQuotedStringCategory& rhs) const
{
   // don't use mQuoted for operator <
   return (value() < rhs.value());
}

void
TokenOrQuotedStringCategory::parse(ParseBuffer& pb)
{
   const char* startMark = pb.skipWhitespace();
   if (*pb.position() == Symbols::DOUBLE_QUOTE[0])
   {
      setQuoted(true);
      pb.skipChar();
      startMark = pb.position();
      pb.skipToEndQuote();
   }
   else
   {
      setQuoted(false);
      pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
   }
   pb.data(mValue, startMark);
   pb.skipToChar(Symbols::SEMI_COLON[0]);
   parseParameters(pb);
}

EncodeStream&
TokenOrQuotedStringCategory::encodeParsed(EncodeStream& str) const
{
   str << quotedValue();
   encodeParameters(str);
   return str;
}

const Data& TokenOrQuotedStringCategory::value() const
{
   checkParsed();
   return mValue;
}

Data& TokenOrQuotedStringCategory::value()
{
   checkParsed();
   return mValue;
}

Data TokenOrQuotedStringCategory::quotedValue() const
{
   checkParsed();
   Data tokenValue;
   if (mQuoted)
   {
      tokenValue += Symbols::DOUBLE_QUOTE;
   }
   // mValue does not contain quoted string
   tokenValue += mValue;
   if (mQuoted)
   {
      tokenValue += Symbols::DOUBLE_QUOTE;
   }
   return tokenValue;
}

ParameterTypes::Factory TokenOrQuotedStringCategory::ParameterFactories[ParameterTypes::MAX_PARAMETER] = { 0 };

Parameter*
TokenOrQuotedStringCategory::createParam(ParameterTypes::Type type,
                                         ParseBuffer& pb,
                                         const std::bitset<256>& terminators,
                                         PoolBase* pool)
{
   if ((type > ParameterTypes::UNKNOWN) && (type < ParameterTypes::MAX_PARAMETER) && ParameterFactories[type])
   {
      return ParameterFactories[type](type, pb, terminators, pool);
   }
   return 0;
}

bool
TokenOrQuotedStringCategory::exists(const Param<TokenOrQuotedStringCategory>& paramType) const
{
    checkParsed();
    bool ret = (getParameterByEnum(paramType.getTypeNum()) != NULL);
    return ret;
}

void
TokenOrQuotedStringCategory::remove(const Param<TokenOrQuotedStringCategory>& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                                                      \
_enum##_Param::DType&                                                                                           \
TokenOrQuotedStringCategory::param(const _enum##_Param& paramType)                                              \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p =                                                                                     \
      static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));                            \
   if (!p)                                                                                                      \
   {                                                                                                            \
      p = new _enum##_Param::Type(paramType.getTypeNum());                                                      \
      mParameters.push_back(p);                                                                                 \
   }                                                                                                            \
   return p->value();                                                                                           \
}                                                                                                               \
                                                                                                                \
const _enum##_Param::DType&                                                                                     \
TokenOrQuotedStringCategory::param(const _enum##_Param& paramType) const                                        \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p =                                                                                     \
      static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));                            \
   if (!p)                                                                                                      \
   {                                                                                                            \
      InfoLog(<< "Missing parameter " _name " " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);     \
      DebugLog(<< *this);                                                                                       \
      throw Exception("Missing parameter " _name, __FILE__, __LINE__);                                          \
   }                                                                                                            \
   return p->value();                                                                                           \
}

defineParam(purpose, "purpose", DataParameter, "draft-ietf-cuss-sip-uui-17"); // User-to-User
defineParam(content, "content", DataParameter, "draft-ietf-cuss-sip-uui-17"); // User-to-User
defineParam(encoding, "encoding", DataParameter, "draft-ietf-cuss-sip-uui-17"); // User-to-User

#undef defineParam

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
