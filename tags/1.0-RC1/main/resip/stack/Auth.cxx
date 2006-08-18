#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/Auth.hxx"
#include "resip/stack/UnknownParameter.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// Auth:
//====================
Auth::Auth() : 
   ParserCategory() 
{}

Auth::Auth(HeaderFieldValue* hfv, Headers::Type type) 
   : ParserCategory(hfv, type) 
{}

Auth::Auth(const Auth& rhs)
   : ParserCategory(rhs)
{
   if (isParsed())
   {
      scheme() = rhs.scheme();
   }
}

Auth&
Auth::operator=(const Auth& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      scheme() = rhs.scheme();
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
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::EQUALS);

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

std::ostream& 
Auth::encodeParsed(std::ostream& str) const
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

void
Auth::parseAuthParameters(ParseBuffer& pb)
{
   while (!pb.eof())
   {
      const char* keyStart = pb.position();
      const char* keyEnd = pb.skipToOneOf(" \t\r\n=");
      ParameterTypes::Type type = ParameterTypes::getType(keyStart, (keyEnd - keyStart));
      if (type == ParameterTypes::UNKNOWN)
      {
         mUnknownParameters.push_back(new UnknownParameter(keyStart, 
                                                           int((keyEnd - keyStart)), pb, 
                                                           " \t\r\n,"));
      }
      else
      {
         // invoke the particular factory
         mParameters.push_back(ParameterTypes::ParameterFactories[type](type, pb, " \t\r\n,"));
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

ostream&
Auth::encodeAuthParameters(ostream& str) const
{
   bool first = true;
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if (!first)
      {
         str << Symbols::COMMA;
      }
      first = false;
      (*it)->encode(str);
   }

   for (ParameterList::iterator it = mUnknownParameters.begin();
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

defineParam(algorithm, "algorithm", DataParameter, "RFC ????");
defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC ????");
defineParam(nonce, "nonce", QuotedDataParameter, "RFC ????");
defineParam(domain, "domain", QuotedDataParameter, "RFC ????");
defineParam(nc, "nc", DataParameter, "RFC ????");
defineParam(opaque, "opaque", QuotedDataParameter, "RFC ????");
defineParam(qop, "qop", <SPECIAL-CASE>, "RFC ????");
defineParam(realm, "realm", QuotedDataParameter, "RFC ????");
defineParam(response, "response", QuotedDataParameter, "RFC ????");
defineParam(stale, "stale", DataParameter, "RFC ????");
defineParam(uri, "uri", QuotedDataParameter, "RFC ????");
defineParam(username, "username", DataParameter, "RFC ????");


Qop_Options_Param::DType&
Auth::param(const Qop_Options_Param& paramType) const
{
   checkParsed();
   Qop_Options_Param::Type* p = static_cast<Qop_Options_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Qop_Options_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
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
