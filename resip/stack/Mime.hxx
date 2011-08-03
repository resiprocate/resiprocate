#if !defined(RESIP_MIME_HXX)
#define RESIP_MIME_HXX 

#include <iosfwd>
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "resip/stack/ParserCategory.hxx"
#include "resip/stack/ParserContainer.hxx"

namespace resip
{

class HeaderFieldValue;

/**
   @ingroup sip_grammar
   @brief Represents the "media-type" element in the RFC 3261 grammar.
*/
class Mime : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputCommas};

      Mime();
      Mime(const Data& type, const Data& subType);
      Mime(const HeaderFieldValue& hfv, 
            Headers::Type type,
            PoolBase* pool=0);
      Mime(const Mime& orig,
            PoolBase* pool=0);
      Mime& operator=(const Mime&);
      bool operator<(const Mime& rhs) const;
      bool isEqual(const Mime& rhs) const;
      bool operator==(const Mime& rhs) const;
      bool operator!=(const Mime& rhs) const;
      
      const Data& type() const;
      const Data& subType() const;
      Data& type();
      Data& subType();
         
      virtual void parse(ParseBuffer& pb);
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
      bool exists(const Param<Mime>& paramType) const;
      void remove(const Param<Mime>& paramType);

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const;  \
      _enum##_Param::DType& param(const _enum##_Param& paramType); \
      friend class _enum##_Param

      defineParam(accessType, "access-type", DataParameter, "RFC 2046");
      defineParam(boundary, "boundary", DataParameter, "RFC 2046");
      defineParam(charset, "charset", DataParameter, "RFC 2045");
      defineParam(directory, "directory", DataParameter, "RFC 2046");
      defineParam(expiration, "expiration", QuotedDataParameter, "RFC 2046");
      defineParam(micalg, "micalg", DataParameter, "RFC 1847");
      defineParam(mode, "mode", DataParameter, "RFC 2046");
      defineParam(name, "name", DataParameter, "RFC 2046");
      defineParam(permission, "permission", DataParameter, "RFC 2046");
      defineParam(protocol, "protocol", QuotedDataParameter, "RFC 1847");
      defineParam(q, "q", QValueParameter, "RFC 3261");
      defineParam(server, "server", DataParameter, "RFC 2046");
      defineParam(site, "site", DataParameter, "RFC 2046");
      defineParam(size, "size", DataParameter, "RFC 2046");
      defineParam(smimeType, "smime-type", DataParameter, "RFC 2633");
      defineParam(url, "url", QuotedDataParameter, "RFC 4483");

#undef defineParam

   private:
      Data mType;
      Data mSubType;

      static ParameterTypes::Factory ParameterFactories[ParameterTypes::MAX_PARAMETER];
};
typedef ParserContainer<Mime> Mimes;
 
}


HashValue(resip::Mime);

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
