#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/OctetContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


ContentsFactory<OctetContents> OctetContents::Factory;
const OctetContents OctetContents::Empty;

OctetContents::OctetContents()
   : Contents(getStaticType()),
     mOctets()
{}

OctetContents::OctetContents(const Data& octets)
   : Contents(getStaticType()),
     mOctets(octets)
{}

OctetContents::OctetContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mOctets()
{
}
 
OctetContents::OctetContents(const Data& octets, const Mime& contentsType)
   : Contents(contentsType),
     mOctets(octets)
{
}

OctetContents::OctetContents(const OctetContents& rhs)
   : Contents(rhs),
     mOctets(rhs.mOctets)
{
}

OctetContents::~OctetContents()
{
}

OctetContents&
OctetContents::operator=(const OctetContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mOctets = rhs.mOctets;
   }
   return *this;
}

Contents* 
OctetContents::clone() const
{
   return new OctetContents(*this);
}

const Mime& 
OctetContents::getStaticType() 
{
   static Mime type("application","octet-stream");
   return type;
}

std::ostream& 
OctetContents::encodeParsed(std::ostream& str) const
{
   //DebugLog(<< "OctetContents::encodeParsed " << mOctets);
   str << mOctets;
   return str;
}

void 
OctetContents::parse(ParseBuffer& pb)
{
   //DebugLog(<< "OctetContents::parse: " << pb.position());

   const char* anchor = pb.position();
   pb.skipToEnd();
   pb.data(mOctets, anchor);

   //DebugLog("OctetContents::parsed <" << mOctets << ">" );
}


Data
OctetContents::getBodyData() const
{
   checkParsed(); 
   return mOctets;
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
