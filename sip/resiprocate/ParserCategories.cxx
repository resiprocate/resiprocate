#include <algorithm>
#include <cassert>

#include <util/Data.hxx>
#include <util/Logger.hxx>
#include <util/ParseBuffer.hxx>
#include <sipstack/ParserCategories.hxx>
#include <sipstack/Uri.hxx>
#include <iostream>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

//====================
// Token
//===================
Token::Token(const Token& rhs)
   : ParserCategory(rhs),
     mValue(rhs.mValue)
{}

Token&
Token::operator=(const Token& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
   }
   return *this;
}

void
Token::parse(ParseBuffer& pb)
{
   const char* startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
   mValue = pb.data(startMark);
   pb.skipToChar(Symbols::SEMI_COLON[0]);
   parseParameters(pb);
}

ParserCategory* 
Token::clone() const
{
   return new Token(*this);
}

std::ostream& 
Token::encode(std::ostream& str) const
{
   str << mValue;
   return str;
}

//====================
// MIME
//====================
Mime::Mime(const Mime& rhs)
   : ParserCategory(rhs),
     mType(rhs.mType),
     mSubType(rhs.mSubType)
{
}

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

void
Mime::parse(ParseBuffer& pb)
{
   assert(0);
}

ParserCategory* 
Mime::clone() const
{
   return new Mime(*this);
}

std::ostream&
Mime::encode(std::ostream& str) const
{
   str << mType << Symbols::SLASH << mSubType;
   return str;
}

//====================
// Auth:
//====================
Auth::Auth(const Auth& rhs)
   : ParserCategory(rhs)
{}

Auth&
Auth::operator=(const Auth& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      assert(0);
   }
   return *this;
}
void
Auth::parse(ParseBuffer& pb)
{
   assert(0);
}

std::ostream& 
Auth::encode(std::ostream& str) const
{
   assert(0);
   return str;
}

ParserCategory* 
Auth::clone() const
{
   return new Auth(*this);
}

//====================
// CSeqCategory:
//====================
CSeqCategory::CSeqCategory(const CSeqCategory& rhs)
   : ParserCategory(rhs),
     mMethod(rhs.mMethod),
     mSequence(rhs.mSequence)
{}

CSeqCategory&
CSeqCategory::operator=(const CSeqCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mMethod = rhs.mMethod;
      mSequence = rhs.mSequence;
   }
   return *this;
}
ParserCategory* 
CSeqCategory::clone() const
{
   return new CSeqCategory(*this);
}

// examples to test: 
// "CSeq:15 ACK"  // ok
// "CSeq:ACK"     // bad
// "CSeq:JOE"     // ok
// "CSeq:1 JOE"   // ok
// "CSeq:1323333 INVITE" // ok 
// "CSeq:1323333 Invite" // ok - not invite
// "CSeq:1323333 InviTe" // ok - not invite
// "CSeq:\t\t  \t15\t\t\t    \t ACK"  // ok
// "CSeq:\t\t  \t15\t\t\t    \t"  // bad

void
CSeqCategory::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   mSequence = pb.integer();

   pb.skipNonWhitespace();
   start = pb.skipWhitespace();
   pb.skipNonWhitespace(); // .dcm. maybe pass an arg that says throw if you
                           // don't move
   mMethod = getMethodType(pb.data(start));
}

std::ostream& 
CSeqCategory::encode(std::ostream& str) const
{
   str << mSequence << Symbols::SPACE << MethodNames[mMethod];
   return str;
}

//====================
// Date
//====================
DateCategory::DateCategory(const DateCategory& rhs)
   : ParserCategory(rhs),
     mValue(rhs.mValue)
{}

DateCategory&
DateCategory::operator=(const DateCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
   }
   return *this;
}
void
DateCategory::parse(ParseBuffer& pb)
{
   mValue = Data(getHeaderField().mField, getHeaderField().mFieldLength);
}

ParserCategory* 
DateCategory::clone() const
{
   return new DateCategory(*this);
}

std::ostream& 
DateCategory::encode(std::ostream& str) const
{
   str << mValue;
   return str;
}

//====================
// WarningCategory
//====================
WarningCategory::WarningCategory(const WarningCategory& rhs)
   : ParserCategory(rhs)
{
}

WarningCategory&
WarningCategory::operator=(const WarningCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      assert(0);
   }
   return *this;
}
void
WarningCategory::parse(ParseBuffer& pb)
{
   assert(0);
}

ParserCategory* 
WarningCategory::clone() const
{
   return new WarningCategory(*this);
}

std::ostream& 
WarningCategory::encode(std::ostream& str) const
{
   return str;
}

//====================
// Integer:
//====================
IntegerCategory::IntegerCategory(const IntegerCategory& rhs)
   : ParserCategory(rhs),
     mValue(0)
{}

IntegerCategory&
IntegerCategory::operator=(const IntegerCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
   }
   return *this;
}
ParserCategory* IntegerCategory::clone() const
{
   return new IntegerCategory(*this);
}

void
IntegerCategory::parse(ParseBuffer& pb)
{
   const char* start = pb.skipWhitespace();
   mValue = pb.integer();
   pb.skipToChar('(');
   if (!pb.eof())
   {
      start = pb.skipChar();
      pb.skipToEndQuote(')');
      mComment = pb.data(start);
      pb.skipChar();
   }
   else
   {
      pb.reset(start);
      start = pb.skipNonWhitespace();
   }
   
   parseParameters(pb);
}

std::ostream& 
IntegerCategory::encode(std::ostream& str) const
{
  str << mValue;
  if (!mComment.empty())
  {
     str << "(" << mComment << ")";
  }
  
  encodeParameters(str);
  
  // call encode on the list to get params
  return str;
}

//====================
// StringCategory
//====================
StringCategory::StringCategory(const StringCategory& rhs)
   : ParserCategory(rhs)
{}

StringCategory&
StringCategory::operator=(const StringCategory& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
   }
   return *this;
}
ParserCategory* 
StringCategory::clone() const
{
   return new StringCategory(*this);
}

void 
StringCategory::parse(ParseBuffer& pb)
{
   mValue = Data(getHeaderField().mField, getHeaderField().mFieldLength);
}

std::ostream& 
StringCategory::encode(std::ostream& str) const
{
   str << mValue;
   return str;
}

//====================
// GenericUri
//====================
GenericURI::GenericURI(const GenericURI& rhs)
   : ParserCategory(rhs)
{}

GenericURI&
GenericURI::operator=(const GenericURI& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      assert(0);
   }
   return *this;
}
void
GenericURI::parse(ParseBuffer& pb)
{
   assert(0);
}

ParserCategory* 
GenericURI::clone() const
{
   return new GenericURI(*this);
}

std::ostream& 
GenericURI::encode(std::ostream& str) const
{
   return str;
}

//====================
// Via:
//====================
Via::Via(const Via& rhs)
   : ParserCategory(rhs),
     mSentPort(0)
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

void
Via::parse(ParseBuffer& pb)
{
   const char* startMark;
   startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SLASH);
   mProtocolName = pb.data(startMark);
   pb.skipToChar('/');
   pb.skipChar();
   startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SLASH);
   mProtocolVersion = pb.data(startMark);

   pb.skipToChar('/');
   pb.skipChar();
   startMark = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mTransport = pb.data(startMark);

   startMark = pb.skipWhitespace();
   if (*startMark == '[')
   {
      pb.skipToChar(']');
      pb.skipChar();
   }
   else
   {
      pb.skipToOneOf(";:");
   }
   mSentHost = pb.data(startMark);
   pb.skipToOneOf(";:");
   if (*pb.position() == ':')
   {
      startMark = pb.skipChar();
      pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
      mSentPort = pb.integer();
   }
   else
   {
      mSentPort = Symbols::DefaultSipPort;
   }
   parseParameters(pb);
}

ostream&
Via::encode(ostream& str) const
{
   str << mProtocolName << Symbols::SLASH << mProtocolVersion << Symbols::SLASH << mTransport 
       << Symbols::SPACE << mSentHost << Symbols::COLON << mSentPort;
   encodeParameters(str);
   return str;
}


//====================
// CallId:
//====================
CallId::CallId(const CallId& rhs)
   : ParserCategory(rhs)
{}

CallId&
CallId::operator=(const CallId& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mValue = rhs.mValue;
   }
   return *this;
}
ParserCategory *
CallId::clone() const
{
   return new CallId(*this);
}

void
CallId::parse(ParseBuffer& pb)
{
   const char* start = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SEMI_COLON);
   mValue = pb.data(start);
   
   parseParameters(pb);
}

ostream&
CallId::encode(ostream& str) const
{
   str << mValue;
   encodeParameters(str);
   return str;
}

//====================
// NameAddr:
//====================
NameAddr::NameAddr(const NameAddr& rhs)
   : ParserCategory(rhs),
     mAllContacts(rhs.mAllContacts),
     mUri(rhs.mUri == 0 ? 0 : new Uri(*rhs.mUri)),
     mDisplayName(rhs.mDisplayName)
{
}

NameAddr&
NameAddr::operator=(const NameAddr& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      delete mUri;
      if (rhs.mUri != 0)
      {
         mUri = new Uri(*rhs.mUri);
      }
      else
      {
         mUri = 0;
      }
      mAllContacts = rhs.mAllContacts;
      mDisplayName = rhs.mDisplayName;
   }
   return *this;
}

NameAddr::~NameAddr()
{
   delete mUri;
}

ParserCategory *
NameAddr::clone() const
{
   return new NameAddr(*this);
}

Uri&
NameAddr::uri() const 
{
   checkParsed(); 
   if (mUri == 0)
   {
      mUri = new Uri();
   }
   return *mUri;
}

void
NameAddr::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   bool laQuote = false;
   if (*pb.position() == Symbols::DOUBLE_QUOTE[0])
   {
      pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
      pb.skipToEndQuote();
      pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
      mDisplayName = pb.data(start);
      laQuote = true;
      pb.skipToChar(Symbols::LA_QUOTE[0]);
      if (pb.eof())
      {
         throw ParseException("Expected '<'", __FILE__, __LINE__);
      }
      else
      {
         pb.skipChar(Symbols::LA_QUOTE[0]);
      }
   }
   else if (*pb.position() == Symbols::LA_QUOTE[0])
   {
      pb.skipChar(Symbols::LA_QUOTE[0]);
      laQuote = true;
   }
   else
   {
      start = pb.position();
      pb.skipToChar(Symbols::LA_QUOTE[0]);
      if (pb.eof())
      {
         pb.reset(start);
      }
      else
      {
         laQuote = true;
         mDisplayName = pb.data(start);
         pb.skipChar(Symbols::LA_QUOTE[0]);
      }
   }
   pb.skipWhitespace();
   mUri = new Uri();
   mUri->parse(pb);
   if (laQuote)
   {
      mUri->parseParameters(pb);
      pb.skipChar('>');
      pb.skipWhitespace();
   }
   parseParameters(pb);
   for (ParameterList::iterator it = mParameters.begin(); 
        it != mParameters.end();)
   {
      switch ((*it)->getType())
      {
         case ParameterTypes::mobility:
         case ParameterTypes::tag:
         case ParameterTypes::q: 
         {
            mUri->mParameters.push_back(*it);
            it = mParameters.erase(it);
            break;
         }
         default:
         {
            it++;
         }
      }
   }
}

ostream&
NameAddr::encode(ostream& str) const
{
   if (!mDisplayName.empty())
   {
      str << mDisplayName << Symbols::LA_QUOTE;
   }

   assert(mUri != 0);
   mUri->encode(str);
   
   if (!mDisplayName.empty())
   {
      str << Symbols::RA_QUOTE;
   }
   
   encodeParameters(str);
   return str;
}

//====================
// RequestLine:
//====================
RequestLine::RequestLine(const RequestLine& rhs)
   : ParserCategory(rhs),
     mUri(rhs.mUri ? new Uri(*rhs.mUri) : 0),
     mMethod(rhs.mMethod),
     mSipVersion(rhs.mSipVersion)
{}

RequestLine&
RequestLine::operator=(const RequestLine& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      delete mUri;
      if (rhs.mUri != 0)
      {
         mUri = new Uri(*rhs.mUri);
      }
      else
      {
         mUri = 0;
      }
      mMethod = rhs.mMethod;
      mSipVersion = rhs.mSipVersion;
   }
   return *this;
}

RequestLine::~RequestLine()
{
   delete mUri;
}

ParserCategory *
RequestLine::clone() const
{
   return new RequestLine(*this);
}

Uri&
RequestLine::uri() const 
{
   checkParsed(); 
   if (mUri == 0)
   {
      mUri = new Uri();
   }
   return *mUri;
}

void 
RequestLine::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mMethod = getMethodType(start, pb.position() - start);
   pb.skipWhitespace();
   mUri = new Uri();
   mUri->parse(pb);
   mUri->parseParameters(pb);
   start = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mSipVersion = pb.data(start);
}

ostream&
RequestLine::encode(ostream& str) const
{
   str << MethodNames[mMethod] << Symbols::SPACE;
   mUri->encode(str);
   str << Symbols::SPACE << mSipVersion;
   return str;
}

//====================
// StatusLine:
//====================
StatusLine::StatusLine(const StatusLine& rhs)
   : ParserCategory(rhs),
     mResponseCode(rhs.mResponseCode),
     mSipVersion(rhs.mSipVersion),
     mReason(rhs.mReason)
{}
     
StatusLine&
StatusLine::operator=(const StatusLine& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mResponseCode = rhs.mResponseCode;
      mSipVersion = rhs.mSipVersion;
      mReason = rhs.mReason;
   }
   return *this;
}

ParserCategory *
StatusLine::clone() const
{
   return new StatusLine(*this);
}

void
StatusLine::parse(ParseBuffer& pb)
{
   const char* start = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mSipVersion = pb.data(start);

   start = pb.skipWhitespace();
   mResponseCode = pb.integer();
   start = pb.skipNonWhitespace();

   start = pb.skipChar(' ');
   pb.reset(pb.end());
   mReason = pb.data(start);
}

ostream&
StatusLine::encode(ostream& str) const
{
   str << mSipVersion << Symbols::SPACE 
       << mResponseCode << Symbols::SPACE
       << mReason;
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
