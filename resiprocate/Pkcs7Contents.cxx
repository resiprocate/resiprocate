#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/Pkcs7Contents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

ContentsFactory<Pkcs7Contents> Pkcs7Contents::Factory;
ContentsFactory<Pkcs7SignedContents> Pkcs7SignedContents::Factory;

Pkcs7Contents::Pkcs7Contents()
   : Contents(getStaticType()),
     mText()
{
}


Pkcs7SignedContents::Pkcs7SignedContents()
{
}

Pkcs7Contents::Pkcs7Contents(const Data& txt)
   : Contents(getStaticType()),
     mText(txt)
{
}

Pkcs7Contents::Pkcs7Contents(const Data& txt, const Mime& contentsType)
   : Contents(contentsType),
     mText(txt)
{
}
 
Pkcs7Contents::Pkcs7Contents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mText()
{
}
 
Pkcs7SignedContents::Pkcs7SignedContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Pkcs7Contents(hfv, contentsType)
{
}
 
Pkcs7Contents::Pkcs7Contents(const Pkcs7Contents& rhs)
   : Contents(rhs),
     mText(rhs.mText)
{
}

Pkcs7SignedContents::Pkcs7SignedContents(const Pkcs7SignedContents& rhs)
   : Pkcs7Contents(rhs)
{
}

Pkcs7Contents::~Pkcs7Contents()
{
}

Pkcs7SignedContents::~Pkcs7SignedContents()
{
}

Pkcs7Contents&
Pkcs7Contents::operator=(const Pkcs7Contents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mText = rhs.mText;
   }
   return *this;
}

Contents* 
Pkcs7Contents::clone() const
{
   return new Pkcs7Contents(*this);
}

Contents* 
Pkcs7SignedContents::clone() const
{
   return new Pkcs7SignedContents(*this);
}

const Mime& 
Pkcs7Contents::getStaticType() 
{
   static Mime type("application","pkcs7-mime");
   return type;
}


const Mime& 
Pkcs7SignedContents::getStaticType() 
{
   static Mime type("application","pkcs7-signature");
   return type;
}


std::ostream& 
Pkcs7Contents::encodeParsed(std::ostream& str) const
{
   //DebugLog(<< "Pkcs7Contents::encodeParsed " << mText);
   str << mText;
   return str;
}


void 
Pkcs7Contents::parse(ParseBuffer& pb)
{
   const char* anchor = pb.position();
   pb.skipToEnd();
   pb.data(mText, anchor);

   DebugLog(<< "Pkcs7Contents::parsed <" << mText.escaped() << ">" );
}


Data 
Pkcs7Contents::getBodyData() const
{
   checkParsed();
   return mText;
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
