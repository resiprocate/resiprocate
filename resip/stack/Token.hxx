#if !defined(RESIP_TOKEN_HXX)
#define RESIP_TOKEN_HXX 

#include <iosfwd>
#include "rutil/Data.hxx"
#include "resip/stack/ParserCategory.hxx"
#include "resip/stack/ParserContainer.hxx"

namespace resip
{


/**
   @ingroup sip_grammar
   @brief Represents the "token" element in the RFC 3261 grammar.
*/
class Token : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputCommas};

      Token();
      explicit Token(const Data& d);
      Token(const HeaderFieldValue& hfv, 
            Headers::Type type,
            PoolBase* pool=0);
      Token(const Token& orig,
            PoolBase* pool=0);
      Token& operator=(const Token&);
      bool isEqual(const Token& rhs) const;
      bool operator==(const Token& rhs) const;
      bool operator!=(const Token& rhs) const;
      bool operator<(const Token& rhs) const;

      /**
         Gets the value (ie; no parameters) of this Token as a Data&.
      */
      const Data& value() const;
      Data& value();

      virtual void parse(ParseBuffer& pb); // remember to call parseParameters()
      virtual ParserCategory* clone() const;
      virtual ParserCategory* clone(void* location) const;
      virtual ParserCategory* clone(PoolBase* pool) const;

      virtual EncodeStream& encodeParsed(EncodeStream& str) const;

      // Inform the compiler that overloads of these may be found in
      // ParserCategory, too.
      using ParserCategory::exists;
      using ParserCategory::remove;
      using ParserCategory::param;

      virtual Parameter* createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool);
      bool exists(const Param<Token>& paramType) const;
      void remove(const Param<Token>& paramType);

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const;  \
      _enum##_Param::DType& param(const _enum##_Param& paramType); \
      friend class _enum##_Param

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

   private:
      Data mValue;

      static ParameterTypes::Factory ParameterFactories[ParameterTypes::MAX_PARAMETER];
};
typedef ParserContainer<Token> Tokens;
 
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
