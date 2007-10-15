#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/Mime.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#if !defined(DISABLE_RESIP_LOG)
#include "resiprocate/os/Logger.hxx"
#endif
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

using namespace resip;
using namespace std;
#if !defined(DISABLE_RESIP_LOG)
#define RESIPROCATE_SUBSYSTEM Subsystem::SIP
#endif
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

Mime::Mime(HeaderFieldValue* hfv, Headers::Type type)
   : ParserCategory(hfv, type),
     mType(), 
     mSubType()
{}

Mime::Mime(const Mime& rhs)
   : ParserCategory(rhs),
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

Data& 
Mime::type() const 
{
   checkParsed(); 
   return mType;
}

Data& Mime::subType() const 
{
   checkParsed(); 
   return mSubType;
}

void
Mime::parse(ParseBuffer& pb)
{
   const char* anchor = pb.skipWhitespace();

   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SLASH);
   pb.data(mType, anchor);

   pb.skipWhitespace();
   pb.skipChar(Symbols::SLASH[0]);

   anchor = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
   pb.data(mSubType, anchor);

   pb.skipWhitespace();
   parseParameters(pb);
}

ParserCategory* 
Mime::clone() const
{
   return new Mime(*this);
}

std::ostream&
Mime::encodeParsed(std::ostream& str) const
{
   str << mType << Symbols::SLASH << mSubType ;
   encodeParameters(str);
   return str;
}

#if defined(HASH_MAP_NAMESPACE)
size_t HASH_MAP_NAMESPACE::hash<resip::Mime>::operator()(const resip::Mime& data) const
{
   return data.type().caseInsensitivehash() ^ data.subType().caseInsensitivehash();
}
#endif

#if defined(__INTEL_COMPILER)
size_t std::hash_value(const resip::Mime& data)
{
   return data.type().caseInsensitivehash() ^ data.subType().caseInsensitivehash();
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
