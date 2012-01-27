/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/Mime.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// MIME
//====================
Mime::Mime()
   : ParserCategory(), 
     mType(),
     mSubType() 
{}
  
Mime::Mime(const Data& type, const Data& subType) 
   : ParserCategory(), 
     mType(type), 
     mSubType(subType) 
{}

Mime::Mime(const HeaderFieldValue& hfv, 
            Headers::Type type,
            PoolBase* pool)
   : ParserCategory(hfv, type, pool),
     mType(), 
     mSubType()
{}

Mime::Mime(const Mime& rhs,
            PoolBase* pool)
   : ParserCategory(rhs, pool),
     mType(rhs.mType),
     mSubType(rhs.mSubType)
{}

Mime&
Mime::operator=(const Mime& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mType = rhs.mType;
      mSubType = rhs.mSubType;
   }
   return *this;
}

bool
Mime::operator<(const Mime& rhs) const
{
   if (isLessThanNoCase(type(), rhs.type()))
   {
      return true;
   }
   else if (isLessThanNoCase(rhs.type(), type()))
   {
      return false;
   }
   return isLessThanNoCase(subType(), rhs.subType());
}

bool
Mime::isEqual(const Mime& rhs) const
{
   return (isEqualNoCase(type(), rhs.type()) &&
           isEqualNoCase(subType(), rhs.subType()));
}

bool
Mime::operator==(const Mime& rhs) const
{
   return (isEqualNoCase(type(), rhs.type()) &&
           isEqualNoCase(subType(), rhs.subType()));
}

bool
Mime::operator!=(const Mime& rhs) const
{
   return !(*this == rhs);
}

const Data& 
Mime::type() const 
{
   checkParsed(); 
   return mType;
}

const Data& Mime::subType() const 
{
   checkParsed(); 
   return mSubType;
}

Data& 
Mime::type()
{
   checkParsed(); 
   return mType;
}

Data& Mime::subType()
{
   checkParsed(); 
   return mSubType;
}

void
Mime::parse(ParseBuffer& pb)
{
   const char* anchor = pb.skipWhitespace();
   static std::bitset<256> delimiter1=Data::toBitset("\r\n\t /");
   pb.skipToOneOf(delimiter1);
   pb.data(mType, anchor);

   pb.skipWhitespace();
   pb.skipChar(Symbols::SLASH[0]);

   anchor = pb.skipWhitespace();
   static std::bitset<256> delimiter2=Data::toBitset("\r\n\t ;");
   pb.skipToOneOf(delimiter2);
   pb.data(mSubType, anchor);

   pb.skipWhitespace();
   parseParameters(pb);
}

ParserCategory* 
Mime::clone() const
{
   return new Mime(*this);
}

ParserCategory* 
Mime::clone(void* location) const
{
   return new (location) Mime(*this);
}

ParserCategory* 
Mime::clone(PoolBase* pool) const
{
   return new (pool) Mime(*this, pool);
}

std::ostream&
Mime::encodeParsed(std::ostream& str) const
{
   str << mType << Symbols::SLASH << mSubType ;
   encodeParameters(str);
   return str;
}

HashValueImp(resip::Mime, data.type().caseInsensitivehash() ^ data.subType().caseInsensitivehash());


/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
