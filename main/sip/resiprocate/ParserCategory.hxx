#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <iostream>
#include <list>
#include "resiprocate/HeaderTypes.hxx"
#include "resiprocate/LazyParser.hxx"
#include "resiprocate/ParameterTypes.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/ParseBuffer.hxx"

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)  \
      _enum##_Param::DType& param(const _enum##_Param& paramType) const

namespace resip
{
class UnknownParameter;
class UnknownParameterType;
class Parameter;

class ParserCategory : public LazyParser
{
    public:
      enum {UnknownParserCategory = -1};

      ParserCategory(HeaderFieldValue* headerFieldValue, Headers::Type type);
      ParserCategory(const ParserCategory& rhs);
      ParserCategory& operator=(const ParserCategory& rhs);

      virtual ~ParserCategory();

      virtual ParserCategory* clone() const = 0;

      bool exists(const ParamBase& paramType) const;
      void remove(const ParamBase& paramType);

      // !dlb! causes compiler error in windows -- change template to const T*
      Data& param(const UnknownParameterType& param) const;

      void remove(const UnknownParameterType& param); 
      bool exists(const UnknownParameterType& param) const;

//#define TEMPLATE_METHODS
#ifdef TEMPLATE_METHODS
      template <class T> 
      typename T::DType& param(const T& paramType) const
      {
         checkParsed();
         typename T::Type* p = static_cast<typename T::Type*>(getParameterByEnum(paramType.getTypeNum()));
         if (!p)
         {
            p = new typename T::Type(paramType.getTypeNum());
            mParameters.push_back(p);
         }
         return p->value();
      }

#else
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
      
      void parseParameters(ParseBuffer& pb);
      std::ostream& encodeParameters(std::ostream& str) const;
      
      // used to compare 2 parameter lists for equality in an order independent way
      Data commutativeParameterHash() const;
      
      // typeless parameter interface
      Parameter* getParameterByEnum(ParameterTypes::Type type) const;
      void removeParameterByEnum(ParameterTypes::Type type);
      void setParameter(const Parameter* parameter);

   protected:
      ParserCategory();

      Parameter* getParameterByData(const Data& data) const;
      void removeParameterByData(const Data& data);

      virtual const Data& errorContext() const;

      typedef std::list<Parameter*> ParameterList; 
      mutable ParameterList mParameters;
      mutable ParameterList mUnknownParameters;
   private:
      Headers::Type mHeaderType;
      void clear();
      void copyParametersFrom(const ParserCategory& other);
      friend std::ostream& operator<<(std::ostream&, const ParserCategory&);
      friend class NameAddr;
};

std::ostream&
operator<<(std::ostream&, const ParserCategory& category);

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

#endif
