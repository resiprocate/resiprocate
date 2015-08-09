#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include "resip/stack/DataParameter.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ParseException.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

DataParameter::DataParameter(ParameterTypes::Type type,
                             ParseBuffer& pb,
                             const std::bitset<256>& terminators)
   : Parameter(type), 
     mValue(),
     mQuoted(false)
{
   pb.skipWhitespace();
   pb.skipChar(Symbols::EQUALS[0]);
   pb.skipWhitespace();
   if(terminators[(unsigned char)(*pb.position())]) // handle cases such as ;tag=
   {
      throw ParseException("Empty value in string-type parameter.",
                              "DataParameter",
                              __FILE__,__LINE__);
   }

   if (*pb.position() == Symbols::DOUBLE_QUOTE[0])
   {
      setQuoted(true);
      pb.skipChar();
      const char* pos = pb.position();
      pb.skipToEndQuote();
      pb.data(mValue, pos);
      pb.skipChar();
   }
   else
   {
      const char* pos = pb.position();
      pb.skipToOneOf(terminators);
      pb.data(mValue, pos);
   }

   if(!mQuoted && mValue.empty())
   {
      // .bwc. We can't let this happen, because we throw if we try to encode
      // when we have an empty value. If that behavior stops, this can be 
      // removed.
     /* throw ParseException("DataParameter c'tor parsed empty param!", 
                           "DataParameter",
                           __FILE__,
                           __LINE__); */
   }
}

DataParameter::DataParameter(ParameterTypes::Type type)
   : Parameter(type),
     mValue(),
     mQuoted(false)
{
}

Parameter* 
DataParameter::clone() const
{
   return new DataParameter(*this);
}

EncodeStream& 
DataParameter::encode(EncodeStream& stream) const
{
   if (mQuoted)
   {
      return stream << getName() << Symbols::EQUALS 
                    << Symbols::DOUBLE_QUOTE << mValue << Symbols::DOUBLE_QUOTE;
   }
   else
   {
      // this will assert if you've accessed a parameter that doesn't exist and
      // then the stack has created an empty parameter with no value. Try
      // calling exists(p_foo) before calling param(p_foo)
      if (mValue.empty())
      {
         ErrLog(<< "Accessing defaulted DataParameter: '" << getName() << "'");
      }
      resip_assert(!mValue.empty()); // !jf!  probably should throw here
      return stream << getName() << Symbols::EQUALS << mValue;
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
