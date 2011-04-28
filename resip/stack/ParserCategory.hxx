#if !defined(RESIP_PARSERCATEGORY_HXX)
#define RESIP_PARSERCATEGORY_HXX 

#include <iosfwd>
#include <vector>
#include <set>
#include "resip/stack/HeaderTypes.hxx"
#include "resip/stack/LazyParser.hxx"
#include "resip/stack/ParameterTypes.hxx"
#include "rutil/Data.hxx"
#include "rutil/BaseException.hxx"

#include "rutil/resipfaststreams.hxx"

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                      \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const;  \
      _enum##_Param::DType& param(const _enum##_Param& paramType)

namespace resip
{
class UnknownParameter;
class ExtensionParameter;
class Parameter;
class ParseBuffer;

/**
   @ingroup resip_crit
   @brief Base class for all SIP grammar elements that can have parameters.
   @todo Maybe a better name? IHaveParams? ElemWithParams?
*/
class ParserCategory : public LazyParser
{
    public:
      enum {UnknownParserCategory = -1};

      // NoCommaTokenizing:        commas do not indicate a new header value
      // CommasAllowedOutputMulti: multi headers can be received with commas but
      //                           output them on separate lines.
      // CommasAllowedOutputCommas: multi headers can be received with commas
      //                            and will always output with commas when
      //                            parsed.  
      enum {NoCommaTokenizing = 0, CommasAllowedOutputMulti = 1, CommasAllowedOutputCommas = 3};
      
      ParserCategory(HeaderFieldValue* headerFieldValue, Headers::Type type);
      ParserCategory(const ParserCategory& rhs);
      ParserCategory& operator=(const ParserCategory& rhs);

      virtual ~ParserCategory();

      virtual ParserCategory* clone() const = 0;
      virtual Parameter* createParam(ParameterTypes::Type type, ParseBuffer& pb, const char* terminators);

      // !dlb! causes compiler error in windows -- change template to const T*
      const Data& param(const ExtensionParameter& param) const;
      Data& param(const ExtensionParameter& param);

      void remove(const ExtensionParameter& param); 
      bool exists(const ExtensionParameter& param) const;

      typedef std::set<ParameterTypes::Type> ParameterTypeSet;      
      
      static const ParameterTypeSet EmptyParameterTypeSet;      

      //doesn't remove unknown parameters
      void removeParametersExcept(const ParameterTypeSet& set = EmptyParameterTypeSet);
      void clearUnknownParameters();

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "ParserCategory::Exception"; }
      };

      void parseParameters(ParseBuffer& pb);
      EncodeStream& encodeParameters(EncodeStream& str) const;
      
      // used to compare 2 parameter lists for equality in an order independent way
      Data commutativeParameterHash() const;
      
      // typeless parameter interface
      Parameter* getParameterByEnum(ParameterTypes::Type type) const;
      void removeParameterByEnum(ParameterTypes::Type type);
      void setParameter(const Parameter* parameter);

      int numKnownParams() const {return (int)mParameters.size();};
      int numUnknownParams() const {return (int)mUnknownParameters.size();};

   protected:
      ParserCategory();

      Parameter* getParameterByData(const Data& data) const;
      void removeParameterByData(const Data& data);

      virtual const Data& errorContext() const;

      typedef std::vector<Parameter*> ParameterList; 
      mutable ParameterList mParameters;
      mutable ParameterList mUnknownParameters;
      Headers::Type mHeaderType;
   private:
      void clear();
      void copyParametersFrom(const ParserCategory& other);
      friend EncodeStream& operator<<(EncodeStream&, const ParserCategory&);
      friend class NameAddr;
};

EncodeStream&
operator<<(EncodeStream&, const ParserCategory& category);

}

#undef defineParam

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
