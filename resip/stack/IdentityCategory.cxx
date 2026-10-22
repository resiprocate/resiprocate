#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/IdentityCategory.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


//====================
// IdentityCategory
//===================
IdentityCategory::IdentityCategory() 
   : ParserCategory(), 
     mValue() 
{}
  
IdentityCategory::IdentityCategory(const Data& d) 
   : ParserCategory(),
     mValue(d) 
{}

IdentityCategory::IdentityCategory(const HeaderFieldValue& hfv, Headers::Type type, PoolBase* pool) 
   : ParserCategory(hfv, type, pool), 
     mValue() 
{}

IdentityCategory::IdentityCategory(const IdentityCategory& rhs, PoolBase* pool)
   : ParserCategory(rhs, pool),
     mValue(rhs.mValue)
{}

IdentityCategory&
IdentityCategory::operator=(const IdentityCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
   }
   return *this;
}

const Data& 
IdentityCategory::value() const 
{
   checkParsed(); 
   return mValue;
}

Data& 
IdentityCategory::value()
{
   checkParsed(); 
   return mValue;
}

void
IdentityCategory::parse(ParseBuffer& pb)
{
   const char* startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
   pb.data(mValue, startMark);
   pb.skipToChar(Symbols::SEMI_COLON[0]);
   parseParameters(pb);
}

ParserCategory* 
IdentityCategory::clone() const
{
   return new IdentityCategory(*this);
}

ParserCategory* 
IdentityCategory::clone(void* location) const
{
   return new (location) IdentityCategory(*this);
}

ParserCategory* 
IdentityCategory::clone(PoolBase* pool) const
{
   return new (pool) IdentityCategory(*this, pool);
}

EncodeStream& 
IdentityCategory::encodeParsed(EncodeStream& str) const
{
   str << mValue;
   encodeParameters(str);
   return str;
}

ParameterTypes::Factory IdentityCategory::ParameterFactories[ParameterTypes::MAX_PARAMETER]={0};

Parameter* 
IdentityCategory::createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool)
{
   if(type > ParameterTypes::UNKNOWN && type < ParameterTypes::MAX_PARAMETER && ParameterFactories[type])
   {
      return ParameterFactories[type](type, pb, terminators, pool);
   }
   return 0;
}

bool 
IdentityCategory::exists(const Param<IdentityCategory>& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

void 
IdentityCategory::remove(const Param<IdentityCategory>& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                                                      \
_enum##_Param::DType&                                                                                           \
IdentityCategory::param(const _enum##_Param& paramType)                                                         \
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
IdentityCategory::param(const _enum##_Param& paramType) const                                                     \
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

defineParam(info, "info", UriParameter, "RFC 8224");

#undef defineParam

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
