#if !defined(RESIP_QVALUE_HXX)
#define RESIP_QVALUE_HXX 

#include "rutil/Data.hxx"
#include "rutil/ParseBuffer.hxx"

#ifndef RESIP_FIXED_POINT
#include <math.h>
// Required due to float point inaccuracies and platform dependent issues (ie. rounding)
static int doubleToInt(const double d) { double f = floor(d); double c = ceil(d); return (((c-d) >= (d-f)) ? (int)f :(int)c); }
#endif

namespace resip
{
   class QValue 
   {
   public:
      explicit QValue() : mValue(0) { }
      explicit QValue(int val) : mValue(val) { }
      explicit QValue(const Data& data) { setValue(data); }

      operator int() const { return mValue; }

      bool operator<(const QValue& rhs) const { return mValue < rhs.mValue; }
      bool operator>(const QValue& rhs) const { return mValue > rhs.mValue; }
      bool operator<=(const QValue& rhs) const { return mValue <= rhs.mValue; }
      bool operator>=(const QValue& rhs) const { return mValue >= rhs.mValue; }
      bool operator==(const QValue& rhs) const { return mValue == rhs.mValue; }
      bool operator!=(const QValue& rhs) const { return mValue != rhs.mValue; }			

      bool operator<(const int rhs) const { return mValue < rhs; }
      bool operator>(const int rhs) const { return mValue > rhs; }
      bool operator<=(const int rhs) const { return mValue <= rhs; }
      bool operator>=(const int rhs) const { return mValue >= rhs; }
      bool operator==(const int rhs) const { return mValue == rhs; }
      bool operator!=(const int rhs) const { return mValue != rhs; }			

      bool operator<(const long rhs) const { return mValue < rhs; }
      bool operator>(const long rhs) const { return mValue > rhs; }
      bool operator<=(const long rhs) const { return mValue <= rhs; }
      bool operator>=(const long rhs) const { return mValue >= rhs; }
      bool operator==(const long rhs) const { return mValue == rhs; }
      bool operator!=(const long rhs) const { return mValue != rhs; }			

      QValue& operator=(const QValue& rhs) { mValue = rhs.mValue;  return (*this); }
      QValue& operator=(const int rhs) { setValue(rhs); return (*this); }			
      QValue& operator=(const long rhs) { setValue(rhs); return (*this); }			

#ifndef RESIP_FIXED_POINT

      float floatVal() const { return mValue/float(1000.0); }

      operator float() const { return floatVal(); }
      operator double() const { return (double) floatVal(); }		

      bool operator<(const float rhs) const { return mValue < doubleToInt(rhs*1000.0); }
      bool operator>(const float rhs) const { return mValue > doubleToInt(rhs*1000.0); }
      bool operator<=(const float rhs) const { return mValue <= doubleToInt(rhs*1000.0); }
      bool operator>=(const float rhs) const { return mValue >= doubleToInt(rhs*1000.0); }
      bool operator==(const float rhs) const { return mValue == doubleToInt(rhs*1000.0); }
      bool operator!=(const float rhs) const { return mValue != doubleToInt(rhs*1000.0); }

      bool operator<(const double rhs) const { return mValue < doubleToInt(rhs*1000.0); }
      bool operator>(const double rhs) const { return mValue > doubleToInt(rhs*1000.0); }
      bool operator<=(const double rhs) const { return mValue <= doubleToInt(rhs*1000.0); }
      bool operator>=(const double rhs) const { return mValue >= doubleToInt(rhs*1000.0); }
      bool operator==(const double rhs) const { return mValue == doubleToInt(rhs*1000.0); }
      bool operator!=(const double rhs) const { return mValue != doubleToInt(rhs*1000.0); }

      QValue& operator=(const float rhs) { setValue(doubleToInt(rhs*1000.0)); return (*this); }			
      QValue& operator=(const double rhs) { setValue(doubleToInt(rhs*1000.0)); return (*this); }			
#endif		
      void setValue(int val) { mValue = (val<0) || (val>1000) ? 1000 : val; }
      void setValue(const Data& data) { ParseBuffer pb(data); mValue = pb.qVal(); }
      int getValue() const { return mValue; }
      const Data& getData() const;
      EncodeStream& encode(EncodeStream& stream) const;

   private:
      int mValue;
      mutable Data mDataValue;
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
