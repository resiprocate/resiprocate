#if !defined(RESIP_NAME_ADDR_HXX)
#define RESIP_NAME_ADDR_HXX

#include <iosfwd>
#include "rutil/Data.hxx"
#include "resip/stack/ParserCategory.hxx"
#include "resip/stack/ParserContainer.hxx"
#include "resip/stack/Uri.hxx"

namespace resip
{

/**
   @ingroup sip_grammar
   @brief Represents the "name-addr" and the "addr-spec" elements in the RFC 
      3261 grammar.
*/
class NameAddr : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputMulti};

      NameAddr();
      NameAddr(const HeaderFieldValue& hfv, 
               Headers::Type type,
               PoolBase* pool=0);
      explicit NameAddr(const Uri& orig);
      explicit NameAddr(const Data& unparsed, bool preCacheAor=false);

      NameAddr(const NameAddr& orig,
               PoolBase* pool=0);
      NameAddr& operator=(const NameAddr&);
      bool operator==(const NameAddr& other) const;

      virtual ~NameAddr();
      
      Uri& uri();
      const Uri& uri() const;
      Data& displayName();
      const Data& displayName() const;
      bool isAllContacts() const;
      void setAllContacts();
      
      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual ParserCategory* clone(void* location) const;
      virtual ParserCategory* clone(PoolBase* pool) const;
      virtual EncodeStream& encodeParsed(EncodeStream& str) const;

      bool operator<(const NameAddr& other) const;

      bool mustQuoteDisplayName() const;      

      // Inform the compiler that overloads of these may be found in
      // ParserCategory, too.
      using ParserCategory::exists;
      using ParserCategory::remove;
      using ParserCategory::param;

      virtual Parameter* createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool);
      bool exists(const Param<NameAddr>& paramType) const;
      void remove(const Param<NameAddr>& paramType);

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const;  \
      _enum##_Param::DType& param(const _enum##_Param& paramType); \
      friend class _enum##_Param

      defineParam(data, "data", ExistsParameter, "RFC 3840");
      defineParam(control, "control", ExistsParameter, "RFC 3840");
      defineParam(mobility, "mobility", QuotedDataParameter, "RFC 3840"); // mobile|fixed
      defineParam(description, "description", QuotedDataParameter, "RFC 3840"); // <> quoted
      defineParam(events, "events", QuotedDataParameter, "RFC 3840"); // list
      defineParam(priority, "priority", QuotedDataParameter, "RFC 3840"); // non-urgent|normal|urgent|emergency
      defineParam(methods, "methods", QuotedDataParameter, "RFC 3840"); // list
      defineParam(schemes, "schemes", QuotedDataParameter, "RFC 3840"); // list
      defineParam(application, "application", ExistsParameter, "RFC 3840");
      defineParam(video, "video", ExistsParameter, "RFC 3840");
      defineParam(language, "language", QuotedDataParameter, "RFC 3840"); // list
      defineParam(type, "type", QuotedDataParameter, "RFC 3840"); // list
      defineParam(isFocus, "isfocus", ExistsParameter, "RFC 3840");
      defineParam(actor, "actor", QuotedDataParameter, "RFC 3840"); // principal|msg-taker|attendant|information
      defineParam(text, "text", ExistsOrDataParameter, "RFC 3840");
      defineParam(extensions, "extensions", QuotedDataParameter, "RFC 3840"); //list
      defineParam(Instance, "+sip.instance", QuotedDataParameter, "RFC 5626");  // <> quoted
      defineParam(regid, "reg-id", UInt32Parameter, "RFC 5626");
      defineParam(pubGruu, "pub-gruu", QuotedDataParameter, "RFC 5627");
      defineParam(tempGruu, "temp-gruu", QuotedDataParameter, "RFC 5627");
      defineParam(expires, "expires", UInt32Parameter, "RFC 3261");
      defineParam(q, "q", QValueParameter, "RFC 3261");
      defineParam(tag, "tag", DataParameter, "RFC 3261");
      defineParam(index, "index", DataParameter, "RFC 4244");
      defineParam(rc, "rc", DataParameter, "RFC 4244-bis");
      defineParam(mp, "mp", DataParameter, "RFC 4244-bis");
      defineParam(np, "np", DataParameter, "RFC 4244-bis");

#undef defineParam

   protected:
      bool mAllContacts;
      Uri mUri;
      Data mDisplayName;
      Data* mUnknownUriParametersBuffer;

   private:

      static ParameterTypes::Factory ParameterFactories[ParameterTypes::MAX_PARAMETER];
};
typedef ParserContainer<NameAddr> NameAddrs;
 
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
