#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Auth.hxx"
#include "resip/stack/UnknownParameter.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// Auth:
//====================
Auth::Auth() : 
   ParserCategory() 
{}

Auth::Auth(const HeaderFieldValue& hfv, Headers::Type type, PoolBase* pool) 
   : ParserCategory(hfv, type, pool) 
{}

Auth::Auth(const Auth& rhs, PoolBase* pool)
   : ParserCategory(rhs, pool),
   mScheme(rhs.mScheme)
{}

Auth&
Auth::operator=(const Auth& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mScheme = rhs.mScheme;
   }
   return *this;
}

Data& 
Auth::scheme()
{
   checkParsed(); 
   return mScheme;
}

const Data& 
Auth::scheme() const 
{
   checkParsed(); 
   return mScheme;
}

void
Auth::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   static const std::bitset<256> schemeDelimiter(Data::toBitset("\r\n\t ="));
   pb.skipToOneOf(schemeDelimiter);

   if (!pb.eof() && *pb.position() == Symbols::EQUALS[0])
   {
      // Authentication-Info only
      // back up, and then parse
      pb.reset(start);
      parseAuthParameters(pb);
   }
   else
   {
      // everything else
      pb.data(mScheme, start);

      pb.skipWhitespace();
      parseAuthParameters(pb);
   }
}

EncodeStream& 
Auth::encodeParsed(EncodeStream& str) const
{
   if (!mScheme.empty())
   {
      str << mScheme << Symbols::SPACE;
   }

   encodeAuthParameters(str);

   return str;
}

ParserCategory* 
Auth::clone() const
{
   return new Auth(*this);
}

ParserCategory* 
Auth::clone(void* location) const
{
   return new (location) Auth(*this);
}

ParserCategory* 
Auth::clone(PoolBase* pool) const
{
   return new (pool) Auth(*this, pool);
}

void
Auth::parseAuthParameters(ParseBuffer& pb)
{
   while (!pb.eof())
   {
      const char* keyStart = pb.position();
      static std::bitset<256> paramBegin=Data::toBitset(" \t\r\n=");
      static std::bitset<256> terminators=Data::toBitset(" \t\r\n,");
      const char* keyEnd = pb.skipToOneOf(paramBegin);
      if((int)(keyEnd-keyStart) != 0)
      {
         ParameterTypes::Type type = ParameterTypes::getType(keyStart, (unsigned int)(keyEnd - keyStart));
         Parameter* p=createParam(type, pb, terminators, getPool());
         if (!p)
         {
            mUnknownParameters.push_back(new UnknownParameter(keyStart, 
                                                              int((keyEnd - keyStart)), pb, 
                                                              terminators));
         }
         else
         {
            // invoke the particular factory
            mParameters.push_back(p);
         }
      }
      else
      {
          // empty parameter name - skip and advance pb to next parameter
          pb.skipToOneOf(terminators);
      }

      pb.skipWhitespace();
      if (pb.eof() || *pb.position() != Symbols::COMMA[0])
      {
         break;
      }
      pb.skipChar();
      pb.skipWhitespace();
   }
}

EncodeStream&
Auth::encodeAuthParameters(EncodeStream& str) const
{
   bool first = true;
   for (ParameterList::const_iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if (!first)
      {
         str << Symbols::COMMA;
      }
      first = false;
      (*it)->encode(str);
   }

   for (ParameterList::const_iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if (!first)
      {
         str << Symbols::COMMA;
      }
      first = false;
      (*it)->encode(str);
   }
   return str;
}

ParameterTypes::Factory Auth::ParameterFactories[ParameterTypes::MAX_PARAMETER]={0};

Parameter* 
Auth::createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool)
{
   if(type==ParameterTypes::qop)
   {
      DataParameter* qop = 0;
      switch(mHeaderType)
      {
         case Headers::ProxyAuthenticate:
         case Headers::WWWAuthenticate:
            qop = new (pool) DataParameter(ParameterTypes::qopOptions,pb,terminators);
            qop->setQuoted(true);
            break;
         case Headers::ProxyAuthorization:
         case Headers::Authorization:
         case Headers::AuthenticationInfo:
         default:
            qop = new (pool) DataParameter(ParameterTypes::qop,pb,terminators);
            qop->setQuoted(false);
      }
      return qop;
   }

   if(type > ParameterTypes::UNKNOWN && type < ParameterTypes::MAX_PARAMETER && ParameterFactories[type])
   {
      return ParameterFactories[type](type, pb, terminators, pool);
   }
   return 0;
}

bool 
Auth::exists(const Param<Auth>& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

void 
Auth::remove(const Param<Auth>& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                                                      \
_enum##_Param::DType&                                                                                           \
Auth::param(const _enum##_Param& paramType)                                                                     \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p = static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));      \
   if (!p)                                                                                                      \
   {                                                                                                            \
      p = new _enum##_Param::Type(paramType.getTypeNum());                                                      \
      mParameters.push_back(p);                                                                                 \
   }                                                                                                            \
   return p->value();                                                                                           \
}                                                                                                               \
const _enum##_Param::DType&                                                                                     \
Auth::param(const _enum##_Param& paramType) const                                                               \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p = static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));      \
   if (!p)                                                                                                      \
   {                                                                                                            \
      InfoLog(<< "Missing parameter " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);               \
      DebugLog(<< *this);                                                                                       \
      throw Exception("Missing parameter", __FILE__, __LINE__);                                                 \
   }                                                                                                            \
   return p->value();                                                                                           \
}

defineParam(algorithm, "algorithm", DataParameter, "RFC 2617");
defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC 2617");
defineParam(domain, "domain", QuotedDataParameter, "RFC 3261");
defineParam(nc, "nc", DataParameter, "RFC 2617");
defineParam(nonce, "nonce", QuotedDataParameter, "RFC 2617");
defineParam(opaque, "opaque", QuotedDataParameter, "RFC 2617");
defineParam(realm, "realm", QuotedDataParameter, "RFC 2617");
defineParam(response, "response", QuotedDataParameter, "RFC 3261");
defineParam(stale, "stale", DataParameter, "RFC 2617");
defineParam(uri, "uri", QuotedDataParameter, "RFC 3261");
defineParam(username, "username", QuotedDataParameter, "RFC 3261");

DataParameter::Type&
Auth::param(const qop_Param& paramType)
{
   checkParsed();
   DataParameter* p = static_cast<DataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new DataParameter(ParameterTypes::qop);
      p->setQuoted(false);
      mParameters.push_back(p);
   }
   return p->value();
}
const DataParameter::Type&
Auth::param(const qop_Param& paramType) const
{
   checkParsed();
   DataParameter* p = static_cast<DataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter", __FILE__, __LINE__);
   }
   return p->value();
}

DataParameter::Type&
Auth::param(const qopOptions_Param& paramType)
{
   checkParsed();
   DataParameter* p = static_cast<DataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new DataParameter(ParameterTypes::qopOptions);
      p->setQuoted(true);
      mParameters.push_back(p);
   }
   return p->value();
}
const DataParameter::Type&
Auth::param(const qopOptions_Param& paramType) const
{
   checkParsed();
   DataParameter* p = static_cast<DataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter", __FILE__, __LINE__);
   }
   return p->value();
}


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
