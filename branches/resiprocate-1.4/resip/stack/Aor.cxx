#include <iostream>

#include "rutil/ParseBuffer.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/DnsUtil.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Aor.hxx"

using namespace resip;

Aor::Aor()
{
}

Aor::Aor(const Data& value)
{
   ParseBuffer pb(value);
   
   pb.skipWhitespace();
   const char* start = pb.position();
   pb.skipToOneOf(":@"); // make sure the colon precedes

   pb.assertNotEof();

   pb.data(mScheme, start);
   pb.skipChar(Symbols::COLON[0]);
   mScheme.lowercase();

   if (isEqualNoCase(mScheme, Symbols::Tel))
   {
      const char* anchor = pb.position();
      pb.skipToOneOf(ParseBuffer::Whitespace, ";>");
      pb.data(mUser, anchor);
      if (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0])
      {
         anchor = pb.skipChar();
         pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::RA_QUOTE);
      }
      return;
   }
   
   start = pb.position();
   pb.skipToChar(Symbols::AT_SIGN[0]);
   if (!pb.eof())
   {
      pb.reset(start);
      start = pb.position();
      pb.skipToOneOf(":@");
      pb.data(mUser, start);
      if (!pb.eof() && *pb.position() == Symbols::COLON[0])
      {
         start = pb.skipChar();
         pb.skipToChar(Symbols::AT_SIGN[0]);
      }
      start = pb.skipChar();
   }
   else
   {
      pb.reset(start);
   }
   
   if (*start == '[')
   {
      start = pb.skipChar();
      pb.skipToChar(']');
      pb.data(mHost, start);
      DnsUtil::canonicalizeIpV6Address(mHost);
      pb.skipChar();
   }
   else
   {
      pb.skipToOneOf(ParseBuffer::Whitespace, ":;?>");
      pb.data(mHost, start);
   }

   pb.skipToOneOf(ParseBuffer::Whitespace, ":;?>");
   if (!pb.eof() && *pb.position() == ':')
   {
      start = pb.skipChar();
      mPort = pb.integer();
      pb.skipToOneOf(ParseBuffer::Whitespace, ";?>");
   }
   else
   {
      mPort = 0;
   }
}

Aor::Aor(const Uri& uri) : 
   mScheme(uri.scheme()),
   mUser(uri.user()),
   mHost(uri.host()),
   mPort(uri.port())
{
   
}

Aor::Aor(const Aor& aor)
{
   *this = aor;
}

Aor& 
Aor::operator=(const Aor& aor)
{
   if (this != &aor)
   {
      mScheme = aor.mScheme;
      mUser = aor.mUser;
      mHost = aor.mHost;
      mPort = aor.mPort;
   }
   return *this;
}

      
bool 
Aor::operator==(const Aor& other) const
{
   return value() == other.value();
}

bool 
Aor::operator!=(const Aor& other) const
{
   return value() != other.value();
}

bool 
Aor::operator<(const Aor& other) const
{
   return value() < other.value();
}

const Data& 
Aor::value() const
{
   if (mOldScheme != mScheme || 
       mOldUser != mUser ||
       mOldHost != mHost ||
       mOldPort != mPort)
   {
      mOldHost = mHost;
      if (DnsUtil::isIpV6Address(mHost))
      {
         mCanonicalHost = DnsUtil::canonicalizeIpV6Address(mHost);
      }
      else
      {
         mCanonicalHost = mHost;
         mCanonicalHost.lowercase();
      }

      mOldScheme = mScheme;
      mOldUser = mUser;
      mOldPort = mPort;

      mValue.reserve(mUser.size() + mCanonicalHost.size() + 10);

      DataStream strm(mValue);
      strm << mScheme;
      strm << Symbols::COLON;
      strm << mUser;
      if (!mCanonicalHost.empty())
      {
         strm << Symbols::AT_SIGN;
         strm << mCanonicalHost;

         if (mPort != 0)
         {
            strm << Symbols::COLON;
            strm << Data(mPort);
         }
      }
   }

   return mValue;
}

Data& 
Aor::scheme()
{
   return mScheme;
}

const Data& 
Aor::scheme() const
{
   return mScheme;
}

Data& 
Aor::host()
{
   return mHost;
}

const Data& 
Aor::host() const
{
   return mHost;
}

Data& 
Aor::user()
{
   return mUser;
}

const Data& 
Aor::user() const
{
   return mUser;
}

int& 
Aor::port()
{
   return mPort;
}

int 
Aor::port() const
{
   return mPort;
}
      
EncodeStream& 
Aor::operator<<(EncodeStream& str) const
{
   str << value();
   return str;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
