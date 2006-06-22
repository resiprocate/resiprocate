#if !defined(RESIP_QVALUEPARAMETER_HXX)
#define RESIP_QVALUEPARAMETER_HXX 

#include "resip/stack/Parameter.hxx"
#include "resip/stack/ParameterTypeEnums.hxx"
#include <iosfwd>

namespace resip
{

   class ParseBuffer;

   class QValueParameter : public Parameter
   {
   public:
      class QVal 
      {
      public:
         explicit QVal(int val) : value(val) { }

         operator int() const { return value; }

         bool operator<(const QVal& rhs) const { return value < rhs.value; }
         bool operator>(const QVal& rhs) const { return value > rhs.value; }
         bool operator<=(const QVal& rhs) const { return value <= rhs.value; }
         bool operator>=(const QVal& rhs) const { return value >= rhs.value; }
         bool operator==(const QVal& rhs) const { return value == rhs.value; }
         bool operator!=(const QVal& rhs) const { return value != rhs.value; }			

         bool operator<(const int rhs) const { return value < rhs; }
         bool operator>(const int rhs) const { return value > rhs; }
         bool operator<=(const int rhs) const { return value <= rhs; }
         bool operator>=(const int rhs) const { return value >= rhs; }
         bool operator==(const int rhs) const { return value == rhs; }
         bool operator!=(const int rhs) const { return value != rhs; }			

         bool operator<(const long rhs) const { return value < rhs; }
         bool operator>(const long rhs) const { return value > rhs; }
         bool operator<=(const long rhs) const { return value <= rhs; }
         bool operator>=(const long rhs) const { return value >= rhs; }
         bool operator==(const long rhs) const { return value == rhs; }
         bool operator!=(const long rhs) const { return value != rhs; }			

         QVal& operator=(const QVal& rhs) { value = rhs.value;  return (*this); }
         QVal& operator=(const int rhs) { setValue(rhs); return (*this); }			
         QVal& operator=(const long rhs) { setValue((int)(rhs * 1000.0)); return (*this); }			

#ifndef RESIP_FIXED_POINT

         float floatVal() const { return value/float(1000.0); }

         operator float() const { return floatVal(); }
         operator double() const { return (double) floatVal(); }		

         bool operator<(const float rhs) const { return (value / 1000.0) < rhs; }
         bool operator>(const float rhs) const { return (value / 1000.0) > rhs; }
         bool operator<=(const float rhs) const { return (value / 1000.0) <= rhs; }
         bool operator>=(const float rhs) const { return (value / 1000.0) >= rhs; }
         bool operator==(const float rhs) const { return (value / 1000.0) == rhs; }
         bool operator!=(const float rhs) const { return (value / 1000.0) != rhs; }

         bool operator<(const double rhs) const { return (value / 1000.0) < rhs; }
         bool operator>(const double rhs) const { return (value / 1000.0) > rhs; }
         bool operator<=(const double rhs) const { return (value / 1000.0) <= rhs; }
         bool operator>=(const double rhs) const { return (value / 1000.0) >= rhs; }
         bool operator==(const double rhs) const { return (value / 1000.0) == rhs; }
         bool operator!=(const double rhs) const { return (value / 1000.0) != rhs; }			

         QVal& operator=(const float rhs) { value = (int)(rhs * 1000.0); return (*this); }			
         QVal& operator=(const double rhs) { value = (int)(rhs * 1000.0); return (*this); }			
#endif		
         void setValue(int val) { value = (val<0) || (val>1000) ? 1000 : val; }
         int getValue() const { return value; }
         std::ostream& encode(std::ostream& stream) const;

      private:
         int value;
      };

      typedef QVal Type;

      QValueParameter(ParameterTypes::Type, ParseBuffer& pb, const char* terminators);
      explicit QValueParameter(ParameterTypes::Type type);

      static Parameter* decode(ParameterTypes::Type type, ParseBuffer& pb, const char* terminators)
      {
         return new QValueParameter(type, pb, terminators);
      }

      virtual Parameter* clone() const;
      virtual std::ostream& encode(std::ostream& stream) const;

   private:
      friend class ParserCategory;
      Type& value() {return mValue;}
      int qval() const {return mValue.getValue();}

      Type mValue;
   };

   std::ostream& operator<<(std::ostream& stream, const QValueParameter::QVal& qvalue);

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
