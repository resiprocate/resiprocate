#if !defined(RESIP_TOKEN_OR_QUOTED_STRING_CATEGORY_HXX)
#define RESIP_TOKEN_OR_QUOTED_STRING_CATEGORY_HXX

#include <iosfwd>
#include "rutil/Data.hxx"
#include "resip/stack/ParserCategory.hxx"
#include "resip/stack/ParserContainer.hxx"

namespace resip
{

/**
   @ingroup sip_grammar
   @brief Generically represents a token whose value can be
   wrapped and encoded using double quotes.
*/

class TokenOrQuotedStringCategory : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputCommas};

      TokenOrQuotedStringCategory();
      explicit TokenOrQuotedStringCategory(const Data& value, bool quoted);
      TokenOrQuotedStringCategory(const HeaderFieldValue& hfv,
                                  Headers::Type type,
                                  PoolBase* pool = 0);
      TokenOrQuotedStringCategory(const TokenOrQuotedStringCategory& rhs,
                                  PoolBase* pool = 0);
      TokenOrQuotedStringCategory& operator=(const TokenOrQuotedStringCategory& rhs);

      virtual ParserCategory* clone() const;
      virtual ParserCategory* clone(void* location) const;
      virtual ParserCategory* clone(PoolBase* pool) const;

      bool isEqual(const TokenOrQuotedStringCategory& rhs) const;
      bool operator==(const TokenOrQuotedStringCategory& rhs) const;
      bool operator!=(const TokenOrQuotedStringCategory& rhs) const;
      bool operator<(const TokenOrQuotedStringCategory& rhs) const;

      virtual void parse(ParseBuffer& pb);
      virtual EncodeStream& encodeParsed(EncodeStream& str) const;

      // Gets the value (ie; no parameters) of this Token as a Data&.
      const Data& value() const;
      Data& value();

      // mValue will be enclosed in quotes e.g. "foo"
      bool isQuoted() const { return mQuoted; }
      void setQuoted(bool b) { mQuoted = b; };

      Data quotedValue() const;

      // Inform the compiler that overloads of these may be found in ParserCategory, too.
      using ParserCategory::exists;
      using ParserCategory::remove;
      using ParserCategory::param;

      virtual Parameter* createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool);
      bool exists(const Param<TokenOrQuotedStringCategory>& paramType) const;
      void remove(const Param<TokenOrQuotedStringCategory>& paramType);

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const;  \
      _enum##_Param::DType& param(const _enum##_Param& paramType); \
      friend class _enum##_Param

      defineParam(purpose, "purpose", DataParameter, "draft-ietf-cuss-sip-uui-17"); // User-to-User
      defineParam(content, "content", DataParameter, "draft-ietf-cuss-sip-uui-17"); // User-to-User
      defineParam(encoding, "encoding", DataParameter, "draft-ietf-cuss-sip-uui-17"); // User-to-User

#undef defineParam

   private:
      Data mValue;
      bool mQuoted;

      static ParameterTypes::Factory ParameterFactories[ParameterTypes::MAX_PARAMETER];
};

typedef ParserContainer<TokenOrQuotedStringCategory> TokenOrQuotedStringCategories;

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
