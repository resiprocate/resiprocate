#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Token.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


//====================
// Token
//===================
Token::Token() 
   : ParserCategory(), 
     mValue() 
{}
  
Token::Token(const Data& d) 
   : ParserCategory(),
     mValue(d) 
{}

Token::Token(const HeaderFieldValue& hfv, Headers::Type type, PoolBase* pool) 
   : ParserCategory(hfv, type, pool), 
     mValue() 
{}

Token::Token(const Token& rhs, PoolBase* pool)
   : ParserCategory(rhs, pool),
     mValue(rhs.mValue)
{}

Token&
Token::operator=(const Token& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
   }
   return *this;
}

bool
Token::isEqual(const Token& rhs) const
{
   return (value() == rhs.value());
}

bool
Token::operator==(const Token& rhs) const
{
   return (value() == rhs.value());
}

bool
Token::operator!=(const Token& rhs) const
{
   return (value() != rhs.value());
}

bool
Token::operator<(const Token& rhs) const
{
   return (value() < rhs.value());
}

const Data& 
Token::value() const 
{
   checkParsed(); 
   return mValue;
}

Data& 
Token::value()
{
   checkParsed(); 
   return mValue;
}

void
Token::parse(ParseBuffer& pb)
{
   const char* startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
   pb.data(mValue, startMark);
   pb.skipToChar(Symbols::SEMI_COLON[0]);
   parseParameters(pb);
}

ParserCategory* 
Token::clone() const
{
   return new Token(*this);
}

ParserCategory* 
Token::clone(void* location) const
{
   return new (location) Token(*this);
}

ParserCategory* 
Token::clone(PoolBase* pool) const
{
   return new (pool) Token(*this, pool);
}

EncodeStream& 
Token::encodeParsed(EncodeStream& str) const
{
   str << mValue;
   encodeParameters(str);
   return str;
}

ParameterTypes::Factory Token::ParameterFactories[ParameterTypes::MAX_PARAMETER]={0};

Parameter* 
Token::createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool)
{
   if(type > ParameterTypes::UNKNOWN && type < ParameterTypes::MAX_PARAMETER && ParameterFactories[type])
   {
      return ParameterFactories[type](type, pb, terminators, pool);
   }
   return 0;
}

bool 
Token::exists(const Param<Token>& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

void 
Token::remove(const Param<Token>& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                                                      \
_enum##_Param::DType&                                                                                           \
Token::param(const _enum##_Param& paramType)                                                           \
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
Token::param(const _enum##_Param& paramType) const                                                     \
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

defineParam(text, "text", ExistsOrDataParameter, "RFC 3840");
defineParam(cause, "cause", UInt32Parameter, "RFC 3326");
defineParam(dAlg, "d-alg", DataParameter, "RFC 3329");
defineParam(dQop, "d-qop", DataParameter, "RFC 3329");
defineParam(dVer, "d-ver", QuotedDataParameter, "RFC 3329");
defineParam(expires, "expires", UInt32Parameter, "RFC 3261");
defineParam(filename, "filename", DataParameter, "RFC 2183");
defineParam(fromTag, "from-tag", DataParameter, "RFC 4235");
defineParam(handling, "handling", DataParameter, "RFC 3261");
defineParam(id, "id", DataParameter, "RFC 3265");
defineParam(q, "q", QValueParameter, "RFC 3261");
defineParam(reason, "reason", DataParameter, "RFC 3265");
defineParam(retryAfter, "retry-after", UInt32Parameter, "RFC 3265");
defineParam(toTag, "to-tag", DataParameter, "RFC 4235");
defineParam(extension, "ext", DataParameter, "RFC 3966"); // Token is used when ext is a user-parameter
defineParam(profileType, "profile-type", DataParameter, "RFC 6080");
defineParam(vendor, "vendor", QuotedDataParameter, "RFC 6080");
defineParam(model, "model", QuotedDataParameter, "RFC 6080");
defineParam(version, "version", QuotedDataParameter, "RFC 6080");
defineParam(effectiveBy, "effective-by", UInt32Parameter, "RFC 6080");
defineParam(document, "document", DataParameter, "draft-ietf-sipping-config-framework-07 (removed in 08)");
defineParam(appId, "app-id", DataParameter, "draft-ietf-sipping-config-framework-05 (renamed to auid in 06, which was then removed in 08)");
defineParam(networkUser, "network-user", DataParameter, "draft-ietf-sipping-config-framework-11 (removed in 12)");
defineParam(require, "require", DataParameter, "RFC 5373");

defineParam(utranCellId3gpp, "utran-cell-id-3gpp", DataParameter, "RFC 3455"); // P-Access-Network-Info
defineParam(cgi3gpp, "cgi-3gpp", DataParameter, "RFC 3455"); // P-Access-Network-Info
defineParam(ccf, "ccf", DataParameter, "RFC 3455"); // P-Charging-Function-Addresses
defineParam(ecf, "ecf", DataParameter, "RFC 3455"); // P-Charging-Function-Addresses
defineParam(icidValue, "icid-value", DataParameter, "RFC 3455"); // P-Charging-Vector
defineParam(icidGeneratedAt, "icid-generated-at", DataParameter, "RFC 3455"); // P-Charging-Vector
defineParam(origIoi, "orig-ioi", DataParameter, "RFC 3455"); // P-Charging-Vector
defineParam(termIoi, "term-ioi", DataParameter, "RFC 3455"); // P-Charging-Vector

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
