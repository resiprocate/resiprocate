#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <iostream>
#include <list>
#include <util/Data.hxx>
#include <util/ParseBuffer.hxx>

#include <sipstack/ParameterTypes.hxx>
#include <sipstack/HeaderFieldValue.hxx>


namespace Vocal2
{
class UnknownParameter;
class Parameter;

class ParserCategory
{
   public:
      ParserCategory(HeaderFieldValue* headerFieldValue)
         : mHeaderField(headerFieldValue),
           mParameters(),
           mUnknownParameters(),
           mIsParsed(false),
           mMine(false)
      {}

      ParserCategory(const ParserCategory& rhs);
      ParserCategory& operator=(const ParserCategory& rhs);

      virtual ~ParserCategory();

      virtual ParserCategory* clone() const = 0;

      virtual std::ostream& encode(std::ostream& str) const = 0;
      std::ostream& encodeFromHeaderFieldValue(std::ostream& str) const;

      bool exists(const ParamBase& paramType) const
      {
         checkParsed();
         return getParameterByEnum(paramType.getTypeNum());
      }

      // removing non-present parameter is allowed      
      void remove(const ParamBase& paramType)
      {
         checkParsed();
         removeParameterByEnum(paramType.getTypeNum());
      }

      Transport_Param::Type::Type& param(const Transport_Param& paramType) const;
      User_Param::Type::Type& param(const User_Param& paramType) const;
      Method_Param::Type::Type& param(const Method_Param& paramType) const;
      Ttl_Param::Type::Type& param(const Ttl_Param& paramType) const;
      Maddr_Param::Type::Type& param(const Maddr_Param& paramType) const;
      Lr_Param::Type::Type& param(const Lr_Param& paramType) const;
      Q_Param::Type::Type& param(const Q_Param& paramType) const;
      Purpose_Param::Type::Type& param(const Purpose_Param& paramType) const;
      Expires_Param::Type::Type& param(const Expires_Param& paramType) const;
      Handling_Param::Type::Type& param(const Handling_Param& paramType) const;
      Tag_Param::Type::Type& param(const Tag_Param& paramType) const;
      ToTag_Param::Type::Type& param(const ToTag_Param& paramType) const;
      FromTag_Param::Type::Type& param(const FromTag_Param& paramType) const;
      Duration_Param::Type::Type& param(const Duration_Param& paramType) const;
      Branch_Param::Type::Type& param(const Branch_Param& paramType) const;
      Received_Param::Type::Type& param(const Received_Param& paramType) const;
      Mobility_Param::Type::Type& param(const Mobility_Param& paramType) const;
      Comp_Param::Type::Type& param(const Comp_Param& paramType) const;
      Rport_Param::Type::Type& param(const Rport_Param& paramType) const;
      
      UnknownParameter& param(const Data& param) const;
      void remove(const Data& param); 
      bool exists(const Data& param) const;
      
      void parseParameters(ParseBuffer& pb);
      std::ostream& encodeParameters(std::ostream& str) const;

      bool isParsed() const {return mIsParsed;}
      
      virtual void parse(ParseBuffer& pb) = 0;

      HeaderFieldValue& getHeaderField() { return *mHeaderField; }
   protected:
      ParserCategory();

      // call before every access 
      void checkParsed() const;

      Parameter* getParameterByEnum(int type) const;
      void removeParameterByEnum(int type);

      Parameter* getParameterByData(const Data& data) const;
      void removeParameterByData(const Data& data);

      HeaderFieldValue* mHeaderField;
      typedef std::list<Parameter*> ParameterList; 
      mutable ParameterList mParameters;
      mutable ParameterList mUnknownParameters;

   private:
      friend std::ostream& operator<<(std::ostream&, const ParserCategory&);
      friend class NameAddr;
      bool mIsParsed;
      bool mMine;
};

std::ostream&
operator<<(std::ostream&, const ParserCategory& category);

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
