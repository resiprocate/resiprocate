#include <sipstack/Uri.hxx>
#include <util/ParseBuffer.hxx>

using namespace Vocal2;

Uri::Uri(const Uri& rhs)
   : ParserCategory(rhs),
     mScheme(rhs.mScheme),
     mHost(rhs.mHost),
     mUser(rhs.mUser),
     mAor(rhs.mAor),
     mPort(rhs.mPort),
     mPassword(rhs.mPassword)
{
}

const Data&
Uri::getAor() const
{
   // could keep last mUser, mHost, mPort around and compare
   // rather than always reallocate...
   mAor = mUser + Symbols::AT_SIGN + mHost + Symbols::COLON + Data(mPort);
   return mAor;
}

void
Uri::parse(ParseBuffer& pb)
{
   const char* start = pb.position();
   pb.skipToChar(Symbols::COLON[0]);
   mScheme = Data(start, pb.position() - start);
   pb.skipChar();
   if (mScheme == Symbols::Sip || mScheme == Symbols::Sips)
   {
      start = pb.position();
      pb.skipToChar(Symbols::AT_SIGN[0]);
      if (!pb.eof())
      {
         pb.reset(start);
         start = pb.position();
         pb.skipToOneOf(":@");
         mUser = Data(start, pb.position() - start);
         if (*pb.position() == Symbols::COLON[0])
         {
            start = pb.skipChar();
            pb.skipToChar(Symbols::AT_SIGN[0]);
            mPassword = Data(start, pb.position() - start);
         }
         start = pb.skipChar();
      }
      else
      {
         pb.reset(start);
      }

      if (*start == '[')
      {
         pb.skipToChar(']');
      }
      else
      {
         pb.skipToOneOf(ParseBuffer::Whitespace, ":;>");
      }
      mHost = Data(start, pb.position() - start);
      pb.skipToOneOf(ParseBuffer::Whitespace, ":;>");
      if (!pb.eof() && *pb.position() == ':')
      {
         start = pb.skipChar();
         pb.skipToOneOf(ParseBuffer::Whitespace, ";>");
         mPort = pb.integer();
      }
      else
      {
         mPort = Symbols::DefaultSipPort;
      }
   }
   else
   {
      // generic URL
      assert(0);
   }
}

ParserCategory*
Uri::clone() const
{
   return new Uri(*this);
}
 
std::ostream& 
Uri::encode(std::ostream& str) const
{
   str << mScheme << Symbols::COLON;
   if (!mUser.empty())
   {
      str << mUser;
      if (!mPassword.empty())
      {
         str << Symbols::COLON << mPassword;
      }
      str << Symbols::AT_SIGN;
   }
   str << mHost << Symbols::COLON << mPort;
   encodeParameters(str);
   return str;
}

void
Uri::parseEmbeddedHeaders()
{
   assert(0);
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
