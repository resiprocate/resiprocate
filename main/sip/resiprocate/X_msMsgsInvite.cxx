#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/X_msMsgsInvite.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

ContentsFactory<X_msMsgsInvite> X_msMsgsInvite::Factory;

X_msMsgsInvite::X_msMsgsInvite()
   : Contents(getStaticType()),
     mPort(0),
     mHost()
{}

X_msMsgsInvite::X_msMsgsInvite(const Data& txt)
   : Contents(getStaticType()),
     mPort(0),
     mHost()
{}

X_msMsgsInvite::X_msMsgsInvite(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mPort(0),
     mHost()
{}
 
X_msMsgsInvite::X_msMsgsInvite(const Data& txt, const Mime& contentsType)
   : Contents(contentsType),
     mPort(0),
     mHost()
{}

X_msMsgsInvite::X_msMsgsInvite(const X_msMsgsInvite& rhs)
   : Contents(rhs),
     mPort(rhs.mPort),
     mHost(rhs.mHost)
{}

X_msMsgsInvite::~X_msMsgsInvite()
{}

X_msMsgsInvite&
X_msMsgsInvite::operator=(const X_msMsgsInvite& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mPort = rhs.mPort;
      mHost = rhs.mHost;
   }
   return *this;
}

Contents* 
X_msMsgsInvite::clone() const
{
   return new X_msMsgsInvite(*this);
}

const Mime& 
X_msMsgsInvite::getStaticType() 
{
   // set charset ?dlb?
   static Mime type("text","x-msmsgsinvite");

   return type;
}

std::ostream& 
X_msMsgsInvite::encodeParsed(std::ostream& str) const
{
   for (Attributes::const_iterator i = mAttributes.begin();
	i != mAttributes.end(); i++)
   {
      str << i->first << ": "  << i->second << Symbols::CRLF;
   }
   if (mPort)
   {
      str << "Port: " << mPort << Symbols::CRLF;
   }
   
   if (!mHost.empty())
   {
      str << "IP-Address: " << mHost << Symbols::CRLF;
   }

   return str;
}

void 
X_msMsgsInvite::parse(ParseBuffer& pb)
{
   DebugLog(<< "X_msMsgsInvite::parse: " << pb.position());

   Data attribute;
   Data value;

   while (!pb.eof())
   {
      const char* anchor = pb.skipWhitespace(); 
      pb.skipToChar(Symbols::COLON[0]);
      pb.data(attribute, anchor);
      pb.skipChar(Symbols::COLON[0]);
      pb.skipWhitespace();

      if (attribute == "Port")
      {
         mPort = pb.integer();
      }
      else if (attribute == "IP-Address")
      {
         pb.skipToChars(Symbols::CRLF);
         pb.data(mHost, anchor);
      }
      else
      {
         pb.skipToChars(Symbols::CRLF);
         pb.data(value, anchor);
         mAttributes.push_back(make_pair(attribute, value));
      }
      pb.skipWhitespace();
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
