#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Via.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


//====================
// Via:
//====================
Via::Via() 
   : ParserCategory(), 
     mProtocolName(Data::Share,Symbols::ProtocolName),
     mProtocolVersion(Data::Share,Symbols::ProtocolVersion),
     mTransport(),
     mSentHost(),
     mSentPort(0) 
{
   // insert a branch in all Vias (default constructor)
   this->param(p_branch);
   this->param(p_rport); // add the rport parameter by default as per rfc 3581
}

Via::Via(const HeaderFieldValue& hfv, 
         Headers::Type type,
         PoolBase* pool) 
   : ParserCategory(hfv, type, pool),
     mProtocolName(Data::Share,Symbols::ProtocolName),
     mProtocolVersion(Data::Share,Symbols::ProtocolVersion),
     mTransport(Data::Share,Symbols::UDP), // !jf! 
     mSentHost(),
     mSentPort(-1) 
{}

Via::Via(const Via& rhs,
         PoolBase* pool)
   : ParserCategory(rhs, pool),
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

ParserCategory *
Via::clone(void* location) const
{
   return new (location) Via(*this);
}

ParserCategory* 
Via::clone(PoolBase* pool) const
{
   return new (pool) Via(*this, pool);
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
   static std::bitset<256> wos=Data::toBitset("\r\n\t /");
   pb.skipToOneOf(wos);
   pb.data(mProtocolName, startMark);
   pb.skipToChar('/');
   pb.skipChar();
   startMark = pb.skipWhitespace();
   pb.skipToOneOf(wos);
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
         // .bwc. So the V6 addy is garbage.
         throw ParseException("Unparsable V6 address (note, this might"
                                    " be unparsable because IPV6 support is not"
                                    " enabled)", "Via",
                                       __FILE__,
                                       __LINE__);
      }
      pb.skipChar();
   }
   else
   {
      // .bwc. If we hit whitespace, we have the host.
      static std::bitset<256> delimiter=Data::toBitset(";: \t\r\n");
      pb.skipToOneOf(delimiter);
      pb.data(mSentHost, startMark);
   }

   pb.skipToOneOf(";:");
   
   if (!pb.eof() && *pb.position() == ':')
   {
      startMark = pb.skipChar(':');
      mSentPort = pb.integer();
      static std::bitset<256> delimiter=Data::toBitset("; \t\r\n");
      pb.skipToOneOf(delimiter);
   }
   else
   {
      mSentPort = 0;
   }
   parseParameters(pb);
}

EncodeStream&
Via::encodeParsed(EncodeStream& str) const
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

ParameterTypes::Factory Via::ParameterFactories[ParameterTypes::MAX_PARAMETER]={0};

Parameter* 
Via::createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool)
{
   if(type > ParameterTypes::UNKNOWN && type < ParameterTypes::MAX_PARAMETER && ParameterFactories[type])
   {
      return ParameterFactories[type](type, pb, terminators, pool);
   }
   return 0;
}

bool 
Via::exists(const Param<Via>& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

void 
Via::remove(const Param<Via>& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                                                      \
_enum##_Param::DType&                                                                                           \
Via::param(const _enum##_Param& paramType)                                                           \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p =                                                                                     \
      static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));                            \
   if (!p)                                                                                                      \
   {                                                                                                            \
      p = new _enum##_Param::Type(paramType.getTypeNum());                                                      \
      mParameters.push_back(p);                                                                                 \
   }                                                                                                            \
   return p->value();                                                                                           \
}                                                                                                               \
                                                                                                                \
const _enum##_Param::DType&                                                                                     \
Via::param(const _enum##_Param& paramType) const                                                     \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p =                                                                                     \
      static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));                            \
   if (!p)                                                                                                      \
   {                                                                                                            \
      InfoLog(<< "Missing parameter " _name " " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);     \
      DebugLog(<< *this);                                                                                       \
      throw Exception("Missing parameter " _name, __FILE__, __LINE__);                                          \
   }                                                                                                            \
   return p->value();                                                                                           \
}

defineParam(branch, "branch", BranchParameter, "RFC 3261");
defineParam(comp, "comp", DataParameter, "RFC 3486");
defineParam(received, "received", DataParameter, "RFC 3261");
defineParam(rport, "rport", RportParameter, "RFC 3581");
defineParam(ttl, "ttl", UInt32Parameter, "RFC 3261");
defineParam(sigcompId, "sigcomp-id", QuotedDataParameter, "RFC 5049");
defineParam(maddr, "maddr", DataParameter, "RFC 3261");

#undef defineParam

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
