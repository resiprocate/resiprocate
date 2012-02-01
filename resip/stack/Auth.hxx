#if !defined(RESIP_AUTH_HXX)
#define RESIP_AUTH_HXX 

#include <iosfwd>
#include "rutil/Data.hxx"
#include "resip/stack/ParserCategory.hxx"

namespace resip
{


/**
   @ingroup sip_grammar
   @brief Represents the "credentials" and "challenge" elements in the SIP 
      grammar.
*/
class Auth : public ParserCategory
{
   public:
      enum {commaHandling = NoCommaTokenizing};

      Auth();
      Auth(const HeaderFieldValue& hfv, Headers::Type type, PoolBase* pool=0);
      Auth(const Auth& orig, PoolBase* pool=0);
      Auth& operator=(const Auth&);

      virtual void parse(ParseBuffer& pb);
      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      virtual ParserCategory* clone() const;
      virtual ParserCategory* clone(void* location) const;
      virtual ParserCategory* clone(PoolBase* pool) const;

      void parseAuthParameters(ParseBuffer& pb);
      EncodeStream& encodeAuthParameters(EncodeStream& str) const;

      Data& scheme();
      const Data& scheme() const;

      // Inform the compiler that overloads of these may be found in
      // ParserCategory, too.
      using ParserCategory::exists;
      using ParserCategory::remove;
      using ParserCategory::param;

      virtual Parameter* createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool);
      bool exists(const Param<Auth>& paramType) const;
      void remove(const Param<Auth>& paramType);

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)  \
      _enum##_Param::DType& param(const _enum##_Param& paramType); \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const; \
      friend class _enum##_Param

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
      defineParam(qop,"qop",DataParameter, "RFC 3261");
      // Internal use only
      defineParam(qopOptions,"qop",DataParameter, "RFC 3261");

#undef defineParam

   private:
      Data mScheme;

      static ParameterTypes::Factory ParameterFactories[ParameterTypes::MAX_PARAMETER];
};
 
}


#endif


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
