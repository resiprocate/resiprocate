#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/OctetContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const OctetContents OctetContents::Empty;

bool
OctetContents::init()
{
   static ContentsFactory<OctetContents> factory;
   (void)factory;
   return true;
}

OctetContents::OctetContents()
   : Contents(getStaticType()),
     mOctets()
{}

OctetContents::OctetContents(const Data& octets)
   : Contents(getStaticType()),
     mOctets(octets)
{}

OctetContents::OctetContents(const HeaderFieldValue& hfv, const Mime& contentsType)
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

EncodeStream& 
OctetContents::encodeParsed(EncodeStream& str) const
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
