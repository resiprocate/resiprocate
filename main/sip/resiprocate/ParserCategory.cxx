#include "resiprocate/sipstack/HeaderFieldValue.hxx"
#include "resiprocate/sipstack/ParserCategory.hxx"

#include "resiprocate/util/DataStream.hxx"
#include "resiprocate/util/ParseBuffer.hxx"
#include "resiprocate/util/compat.hxx"

#include "resiprocate/sipstack/UnknownParameter.hxx"
#include "resiprocate/sipstack/UnknownParameterType.hxx"

#include <iostream>
#include <cassert>

#include "resiprocate/util/Logger.hxx"
#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace Vocal2;
using namespace std;

ParserCategory::ParserCategory(HeaderFieldValue* headerFieldValue,
                               Headers::Type headerType)
    : LazyParser(headerFieldValue),
      mParameters(),
      mUnknownParameters(),
      mHeaderType(headerType)
{
}

ParserCategory::ParserCategory()
   : LazyParser()
{
}

ParserCategory::ParserCategory(const ParserCategory& rhs)
   : LazyParser(rhs)
{
   if (isParsed())
   {
      copyParametersFrom(rhs);
   }
}

ParserCategory&
ParserCategory::operator=(const ParserCategory& rhs)
{
   if (this != &rhs)
   {
      clear();
      LazyParser::operator=(rhs);
      if (rhs.isParsed())
      {
         copyParametersFrom(rhs);
      }
   }
   return *this;
}

void
ParserCategory::clear()
{
   //DebugLog(<<"ParserCategory::clear");
   LazyParser::clear();

   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      delete *it;
   }
   mParameters.clear();

   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      delete *it;
   }   
   mUnknownParameters.clear();
}

void 
ParserCategory::copyParametersFrom(const ParserCategory& other)
{
   for (ParameterList::iterator it = other.mParameters.begin();
        it != other.mParameters.end(); it++)
   {
      mParameters.push_back((*it)->clone());
   }
   for (ParameterList::iterator it = other.mUnknownParameters.begin();
        it != other.mUnknownParameters.end(); it++)
   {
      mUnknownParameters.push_back((*it)->clone());
   }
}

ParserCategory::~ParserCategory()
{
   clear();
}

Data&
ParserCategory::param(const UnknownParameterType& param) const
{
   checkParsed();
   Parameter* p = getParameterByData(param.getName());
   if(!p)
   {
      p = new UnknownParameter(param.getName());
      mUnknownParameters.push_back(p);
   } 
   return static_cast<UnknownParameter*>(p)->value();
}

bool
ParserCategory::exists(const ParamBase& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

// removing non-present parameter is allowed      
void
ParserCategory::remove(const ParamBase& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

void 
ParserCategory::remove(const UnknownParameterType& param)
{
   checkParsed();
   removeParameterByData(param.getName());
}

bool 
ParserCategory::exists(const UnknownParameterType& param) const
{
   checkParsed();
   return getParameterByData(param.getName()) != NULL;
}

void
ParserCategory::parseParameters(ParseBuffer& pb)
{
   while (!pb.eof() )
   {
      const char* start = pb.position();
      pb.skipWhitespace();

      if (  (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0]) )
      {
         // extract the key
         pb.skipChar();
         const char* keyStart = pb.skipWhitespace();
         const char* keyEnd = pb.skipToOneOf(" \t\r\n;=?>");  //!dlb! @ here?
         ParameterTypes::Type type = ParameterTypes::getType(keyStart, (keyEnd - keyStart));
         if (type == ParameterTypes::UNKNOWN)
         {
            mUnknownParameters.push_back(new UnknownParameter(keyStart, int((keyEnd - keyStart)), pb));
         }
         else
         {
            // invoke the particular factory
            mParameters.push_back(ParameterTypes::ParameterFactories[type](type, pb, " \t\r\n;?>"));
         }
      }
      else
      {
         pb.reset(start);
         return;
      }
   }
}      

static Data up_Msgr("msgr");

ostream&
ParserCategory::encodeParameters(ostream& str) const
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      str << Symbols::SEMI_COLON;
      // !ah! this is a TOTAL hack to work around an MSN bug that
      // !ah! requires a SPACE after the SEMI following the MIME type.
      if (it == mParameters.begin() && getParameterByData(up_Msgr))
      {
         str << Symbols::SPACE;
      }
      (*it)->encode(str);
   }
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      str << Symbols::SEMI_COLON;
      (*it)->encode(str);
   }
   return str;
}

ostream&
Vocal2::operator<<(ostream& stream, const ParserCategory& category)
{
   category.checkParsed();
   return category.encode(stream);
}

Parameter* 
ParserCategory::getParameterByEnum(ParameterTypes::Type type) const
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if ((*it)->getType() == type)
      {
         return *it;
      }
   }
   return 0;
}

void
ParserCategory::setParameter(const Parameter* parameter)
{
   if (parameter != 0)
   {
      for (ParameterList::iterator it = mParameters.begin();
           it != mParameters.end(); it++)
      {
         if ((*it)->getType() == parameter->getType())
         {
            delete *it;
            mParameters.erase(it);
            break;
         }
      }
      mParameters.push_back(parameter->clone());
   }
}

void 
ParserCategory::removeParameterByEnum(ParameterTypes::Type type)
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if ((*it)->getType() == type)
      {
         delete *it;
         mParameters.erase(it);
         return;
      }
   }
}

Parameter* 
ParserCategory::getParameterByData(const Data& data) const
{
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if (isEqualNoCase((*it)->getName(), data))
      {
         return *it;
      }
   }
   return 0;
}

void 
ParserCategory::removeParameterByData(const Data& data)
{
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if ((*it)->getName() == data)
      {
         delete *it;
         mParameters.erase(it);
         return;
      }
   }
}

#ifndef TEMPLATE_METHODS

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                              \
_enum##_Param::DType&                                                                   \
ParserCategory::param(const _enum##_Param& paramType) const                             \
{                                                                                       \
   checkParsed();                                                                       \
   _enum##_Param::Type* p =                                                             \
      static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));    \
   if (!p)                                                                              \
   {                                                                                    \
      p = new _enum##_Param::Type(paramType.getTypeNum());                              \
      mParameters.push_back(p);                                                         \
   }                                                                                    \
   return p->value();                                                                   \
}

defineParam(accessType, "access-type", DataParameter, "RFC 2046");
defineParam(algorithm, "algorithm", DataParameter, "RFC ????");
defineParam(boundary, "boundary", DataParameter, "RFC 2046");
defineParam(branch, "branch", BranchParameter, "RFC ????");
defineParam(charset, "charset", DataParameter, "RFC 2045");
defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC ????");
defineParam(comp, "comp", DataParameter, "RFC ????");
defineParam(dAlg, "d-alg", DataParameter, "RFC 3329");
defineParam(dQop, "d-qop", DataParameter, "RFC ????");
defineParam(dVer, "d-ver", QuotedDataParameter, "RFC ????");
defineParam(directory, "directory", DataParameter, "RFC 2046");
defineParam(domain, "domain", QuotedDataParameter, "RFC ????");
defineParam(duration, "duration", IntegerParameter, "RFC ????");
defineParam(expiration, "expiration", IntegerParameter, "RFC 2046");
defineParam(expires, "expires", IntegerParameter, "RFC ????");
defineParam(filename, "filename", DataParameter, "RFC ????");
defineParam(fromTag, "from-tag", DataParameter, "RFC ????");
defineParam(handling, "handling", DataParameter, "RFC ????");
defineParam(id, "id", DataParameter, "RFC ????");
defineParam(lr, "lr", ExistsParameter, "RFC ????");
defineParam(maddr, "maddr", DataParameter, "RFC ????");
defineParam(method, "method", DataParameter, "RFC ????");
defineParam(micalg, "micalg", DataParameter, "RFC 1847");
defineParam(mobility, "mobility", DataParameter, "RFC ????");
defineParam(mode, "mode", DataParameter, "RFC 2046");
defineParam(name, "name", DataParameter, "RFC 2046");
defineParam(nc, "nc", DataParameter, "RFC ????");
defineParam(nonce, "nonce", QuotedDataParameter, "RFC ????");
defineParam(opaque, "opaque", QuotedDataParameter, "RFC ????");
defineParam(permission, "permission", DataParameter, "RFC 2046");
defineParam(protocol, "protocol", DataParameter, "RFC 1847");
defineParam(purpose, "purpose", DataParameter, "RFC ????");
defineParam(q, "q", FloatParameter, "RFC ????");
defineParam(realm, "realm", QuotedDataParameter, "RFC ????");
defineParam(reason, "reason", DataParameter, "RFC ????");
defineParam(received, "received", DataParameter, "RFC ????");
defineParam(response, "response", QuotedDataParameter, "RFC ????");
defineParam(retryAfter, "retry-after", IntegerParameter, "RFC ????");
defineParam(rport, "rport", RportParameter, "RFC ????");
defineParam(server, "server", DataParameter, "RFC 2046");
defineParam(site, "site", DataParameter, "RFC 2046");
defineParam(size, "size", DataParameter, "RFC 2046");
defineParam(smimeType, "smime-type", DataParameter, "RFC 2633");
defineParam(stale, "stale", DataParameter, "RFC ????");
defineParam(tag, "tag", DataParameter, "RFC ????");
defineParam(toTag, "to-tag", DataParameter, "RFC ????");
defineParam(transport, "transport", DataParameter, "RFC ????");
defineParam(ttl, "ttl", IntegerParameter, "RFC ????");
defineParam(uri, "uri", QuotedDataParameter, "RFC ????");
defineParam(user, "user", DataParameter, "RFC ????");
defineParam(username, "username", DataParameter, "RFC ????");

defineParam(qop, "qop", <SPECIAL-CASE>, "RFC ????");

#endif

Data
ParserCategory::commutativeParameterHash() const
{
   Data buffer;
   Data working;

   for (ParameterList::const_iterator i=mParameters.begin(); i!=mParameters.end(); i++)
   {
      if ((*i)->getType() != ParameterTypes::lr)
      {
         buffer.clear();
         DataStream strm(buffer);

         (*i)->encode(strm);
         strm.flush();
         working ^= buffer;
      }
   }

   buffer.clear();
   for (ParameterList::const_iterator i=mUnknownParameters.begin(); i!=mUnknownParameters.end(); i++)
   {
      UnknownParameter* p = static_cast<UnknownParameter*>(*i);
      buffer = p->getName();
      buffer += p->value();
      working ^= buffer;
   }
   
   return working;
}

const Data&
ParserCategory::errorContext() const
{
   if (mHeaderType == Headers::NONE)
   {
      static const Data reqLine("Request/Status line");
      return reqLine;
   }
   else
   {
      return Headers::getHeaderName(mHeaderType);
   }
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

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
