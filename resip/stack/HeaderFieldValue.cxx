#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>

#include "resip/stack/UnknownParameter.hxx"
#include "resip/stack/ExistsParameter.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "resip/stack/MsgHeaderScanner.hxx"
#include "resip/stack/ParserCategory.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace std;
using namespace resip;


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const HeaderFieldValue HeaderFieldValue::Empty;

HeaderFieldValue::HeaderFieldValue(const char* field, unsigned int fieldLength)
   : mField(field),
     mFieldLength(fieldLength),
     mMine(false)
{}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv)
   : mField(0),
     mFieldLength(hfv.mFieldLength),
     mMine(true)
{
   if(mFieldLength)
   {
      char* newField = new char[mFieldLength];
      memcpy(newField, hfv.mField, mFieldLength);
      mField=newField;
   }
}

HeaderFieldValue&
HeaderFieldValue::operator=(const HeaderFieldValue& rhs)
{
   if(this!=&rhs)
   {
      mFieldLength=rhs.mFieldLength;
      if(mMine) delete [] mField;
      mMine=true;
      if(mFieldLength)
      {
         char* newField = new char[mFieldLength];
         memcpy(newField, rhs.mField, mFieldLength);
         mField=newField;
      }
      else
      {
         mField=0;
      }
   }
   
   return *this;
}

HeaderFieldValue& 
HeaderFieldValue::copyWithPadding(const HeaderFieldValue& rhs)
{
   if(this!=&rhs)
   {
      mFieldLength=rhs.mFieldLength;
      if(mMine) delete [] mField;
      mMine=true;
      if(mFieldLength)
      {
         char* newField = MsgHeaderScanner::allocateBuffer(mFieldLength);
         memcpy(newField, rhs.mField, mFieldLength);
         mField=newField;
      }
      else
      {
         mField=0;
      }
   }
   
   return *this;
}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv, CopyPaddingEnum e)
   : mField(0),
     mFieldLength(hfv.mFieldLength),
     mMine(true)
{
   char* newField = MsgHeaderScanner::allocateBuffer(mFieldLength);
   memcpy(newField, hfv.mField, mFieldLength);
   mField=newField;
}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv, NoOwnershipEnum n)
   : mField(hfv.mField),
     mFieldLength(hfv.mFieldLength),
     mMine(false)
{
   // ?bwc? assert(!hfv.mMine); ?
}

HeaderFieldValue&
HeaderFieldValue::swap(HeaderFieldValue& orig)
{
   if (this != &orig)
   {
      std::swap(mField, orig.mField);
      std::swap(mFieldLength, orig.mFieldLength);
      std::swap(mMine, orig.mMine);
   }
   return *this;
}

HeaderFieldValue::~HeaderFieldValue()
{
  if (mMine)
  {
     delete[] mField;
  }
}

EncodeStream& 
HeaderFieldValue::encode(EncodeStream& str) const
{
   str.write(mField, mFieldLength);
   return str;
}

EncodeStream& resip::operator<<(EncodeStream& stream, HeaderFieldValue& hfv)
{
   hfv.encode(stream);
   return stream;
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
