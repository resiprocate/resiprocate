#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/Via.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


//====================
// Via:
//====================
Via::Via() 
   : ParserCategory(), 
     mProtocolName(Symbols::ProtocolName),
     mProtocolVersion(Symbols::ProtocolVersion),
     mTransport(),
     mSentHost(),
     mSentPort(0) 
{
   // insert a branch in all Vias (default constructor)
   this->param(p_branch);
   this->param(p_rport); // add the rport parameter by default as per rfc 3581
}

Via::Via(HeaderFieldValue* hfv, Headers::Type type) 
   : ParserCategory(hfv, type),
     mProtocolName(Symbols::ProtocolName),
     mProtocolVersion(Symbols::ProtocolVersion),
     mTransport(Symbols::UDP), // !jf! 
     mSentHost(),
     mSentPort(-1) 
{}

Via::Via(const Via& rhs)
   : ParserCategory(rhs),
     mProtocolName(rhs.mProtocolName),
     mProtocolVersion(rhs.mProtocolVersion),
     mTransport(rhs.mTransport),
     mSentHost(rhs.mSentHost),
     mSentPort(rhs.mSentPort)
{
}

Via&
Via::operator=(const Via& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mProtocolName = rhs.mProtocolName;
      mProtocolVersion = rhs.mProtocolVersion;
      mTransport = rhs.mTransport;
      mSentHost = rhs.mSentHost;
      mSentPort = rhs.mSentPort;
   }
   return *this;
}

ParserCategory *
Via::clone() const
{
   return new Via(*this);
}

Data& 
Via::protocolName()
{
   checkParsed(); 
   return mProtocolName;
}

const Data& 
Via::protocolName() const 
{
   checkParsed(); 
   return mProtocolName;
}

Data& 
Via::protocolVersion()
{
   checkParsed(); 
   return mProtocolVersion;
}

const Data& 
Via::protocolVersion() const 
{
   checkParsed(); 
   return mProtocolVersion;
}

Data& Via::
transport()
{
   checkParsed(); 
   return mTransport;
}

const Data& Via::
transport() const 
{
   checkParsed(); 
   return mTransport;
}

Data& 
Via::sentHost()
{
   checkParsed(); 
   return mSentHost;
}

const Data&
 Via::sentHost() const
{
   checkParsed(); 
   return mSentHost;
}

int& 
Via::sentPort()
{
   checkParsed(); 
   return mSentPort;
}

int
Via::sentPort() const 
{
   checkParsed(); 
   return mSentPort;
}

void
Via::parse(ParseBuffer& pb)
{
   const char* startMark;
   startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SLASH);
   pb.data(mProtocolName, startMark);
   pb.skipToChar('/');
   pb.skipChar();
   startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SLASH);
   pb.data(mProtocolVersion, startMark);

   pb.skipToChar('/');
   pb.skipChar();
   startMark = pb.skipWhitespace();

   // !jf! this should really be skipTokenChar() since for instance, if the
   // protocol token is missing it will read the rest of the Via into this field
   pb.skipNonWhitespace(); 
   pb.data(mTransport, startMark);
   
   startMark = pb.skipWhitespace();
   pb.assertNotEof();
   if (*startMark == '[')
   {
      startMark = pb.skipChar();
      pb.skipToChar(']');
      pb.data(mSentHost, startMark);
      // .bwc. We do not save this canonicalization, since we weren't doing so
      // before. This may change soon.
      Data canonicalizedHost=DnsUtil::canonicalizeIpV6Address(mSentHost);
      if(canonicalizedHost.empty())
      {
         // ?bwc? So the V6 addy is garbage. I think the parse should fail at 
         // this point, but most of the code assumes that it will fail by 
         // throwing a ParseBuffer::Exception. We are not a ParseBuffer, so this
         // is a little ugly. Oh well.
         throw ParseBuffer::Exception("Unparsable V6 address (note, this might"
                                    " be unparsable because IPV6 support is not"
                                    " enabled)","Via",__FILE__,
                                       __LINE__);
      }
      pb.skipChar();
   }
   else
   {
      // !bwc! If we hit whitespace, we have the host.
      pb.skipToOneOf(";: \t\r\n");
      pb.data(mSentHost, startMark);
   }

   pb.skipToOneOf(";:");
   
   if (!pb.eof() && *pb.position() == ':')
   {
      startMark = pb.skipChar(':');
      mSentPort = pb.integer();
      pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
   }
   else
   {
      mSentPort = 0;
   }
   parseParameters(pb);
}

ostream&
Via::encodeParsed(ostream& str) const
{
   str << mProtocolName << Symbols::SLASH << mProtocolVersion << Symbols::SLASH << mTransport 
       << Symbols::SPACE;

   if (DnsUtil::isIpV6Address(mSentHost))
   {
      str << '[' << mSentHost << ']';
   }
   else
   {
      str << mSentHost;
   }
   
   if (mSentPort != 0)
   {
      str << Symbols::COLON << mSentPort;
   }
   encodeParameters(str);
   return str;
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
