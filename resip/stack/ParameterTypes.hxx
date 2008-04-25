#if !defined(RESIP_PARAMETERTYPES_HXX)
#define RESIP_PARAMETERTYPES_HXX 

#include "resip/stack/BooleanFeatureParameter.hxx"
#include "resip/stack/BranchParameter.hxx"
#include "resip/stack/DataParameter.hxx"
#include "resip/stack/NumericFeatureParameter.hxx"
#include "resip/stack/QuotedDataParameter.hxx"
#include "resip/stack/QuotedListParameter.hxx"
#include "resip/stack/QopParameter.hxx"
#include "resip/stack/IntegerParameter.hxx"
#include "resip/stack/UInt32Parameter.hxx"
#include "resip/stack/QValueParameter.hxx"
#include "resip/stack/ExistsParameter.hxx"
#include "resip/stack/ParameterTypeEnums.hxx"
#include "resip/stack/RportParameter.hxx"
#include "resip/stack/Symbols.hxx"

namespace resip
{

class ParamBase
{
   public:
      virtual ~ParamBase() {}
      virtual ParameterTypes::Type getTypeNum() const = 0;
      virtual const char* name() const = 0;
};

#include "resip/stack/ParameterTypes.hxx.ixx"

// .bwc. Alternate accessor/type for qop that uses a quoted string.
class qopOptions_Param : public ParamBase
{
  public:
   typedef DataParameter Type;
   typedef DataParameter::Type DType;
   virtual ParameterTypes::Type getTypeNum() const;
   virtual const char* name() const { return "qop"; }
   qopOptions_Param();
};
extern qopOptions_Param p_qopOptions;

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
