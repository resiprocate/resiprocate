#include <algorithm>
#include <cassert>
#include <iostream>

#include <util/Data.hxx>
#include <util/Logger.hxx>
#include <util/ParseBuffer.hxx>
#include <sipstack/ParserCategories.hxx>
#include <sipstack/Uri.hxx>
#include <sipstack/UnknownParameter.hxx>

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
   pb.data(mValue, startMark);
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
      mScheme = rhs.mScheme;
   }
   return *this;
}


void
Auth::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, ",=");

   if (*pb.position() == Symbols::EQUALS[0])
   {
      // Authoentication-Info only
      // back up, and then parse
      pb.reset(start);
      parseAuthParameters(pb);
   }
   else
   {
      // everything else
      pb.data(mScheme, start);

      pb.skipWhitespace();
      parseAuthParameters(pb);
   }
}

std::ostream& 
Auth::encode(std::ostream& str) const
{
   if (!mScheme.empty())
   {
      str << mScheme << Symbols::SPACE;
   }

   encodeAuthParameters(str);

   return str;
}

ParserCategory* 
Auth::clone() const
{
   return new Auth(*this);
}


Data&
Auth::scheme()
{
   checkParsed();
   return mScheme;
}

void
Auth::parseAuthParameters(ParseBuffer& pb)
{
   while (!pb.eof())
   {
      const char* keyStart = pb.position();
      const char* keyEnd = pb.skipToOneOf(" \t\r\n=");
      mUnknownParameters.push_back(new UnknownParameter(keyStart, int((keyEnd - keyStart)), pb, " \t\r\n,"));
      pb.skipWhitespace();
      if (*pb.position() != Symbols::COMMA[0])
      {
	 break;
      }
      pb.skipChar();
      pb.skipWhitespace();
   }
}      

ostream&
Auth::encodeAuthParameters(ostream& str) const
{
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if (it != mUnknownParameters.begin()) 
      {
	 str << Symbols::COMMA;
      }
      (*it)->encode(str);
   }
   return str;
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
// "CSeq:1xihzihsihtqnognsd INVITE" // not ok, but parses (?)

void
CSeqCategory::parse(ParseBuffer& pb)
{
   const char* anchorPtr;
   //      anchorPtr = 
   pb.skipWhitespace();
   mSequence = pb.integer();

   pb.skipNonWhitespace();
   anchorPtr = pb.skipWhitespace();
   pb.skipNonWhitespace(); // .dcm. maybe pass an arg that says throw if you
                           // don't move
   mMethod = getMethodType(anchorPtr, pb.position() - anchorPtr);
   if (mMethod == UNKNOWN)
   {
      pb.data(mUnknownMethodName, anchorPtr);
   }
}

std::ostream& 
CSeqCategory::encode(std::ostream& str) const
{
   str << mSequence << Symbols::SPACE << (mMethod != UNKNOWN ? MethodNames[mMethod] : mUnknownMethodName);
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
   // Mon, 04 Nov 2002 17:34:15 GMT

   const char* anchor = pb.skipWhitespace();

   pb.skipToChar(",");
   Data dayOfWeek;
   pb.data(dayOfWeek, anchor);
   mDayOfWeek = DateCategory::DayOfWeek(dayOfWeek);

   pb.skipChar(",");

   pb.skipWhitespace();

   mDayOfMonth = pb.int();

   anchor = pb.skipWhitespace();
   pb.skipNonWhitespace();

   Data month;
   pb.data(month, anchor);
   mMonth = DateCategory::Month(month);

   pb.skipWhitespace();
   mYear = pb.int();

   pb.skipWhitespace();

   mHour = pb.int();
   pb.skipChar(":");
   mMin = pb.int();
   pb.skipChar(":");
   mSec = pb.int();

   pb.skipWhitespace();
   pb.skipChar("G");
   pb.skipChar("M");
   pb.skipChar("T");

   pb.skipWhitespace();
   pb.assertEof();
}

ParserCategory* 
DateCategory::clone() const
{
   return new DateCategory(*this);
}

static void pad2(const int x, std::ostream& str)
{
   str << ( (x < 10) ? Symbols::ZERO[0] : Symbols::Empty[0] );
}

std::ostream& 
DateCategory::encode(std::ostream& str) const
{
   str << DateCategory::DayOfWeekData[mDayOfWeek] // Mon
       << Symbols::COMMA[0] << Symbols::SPACE[0];
   
   pad2(mDayOfMonth, str);  //  04

   str << mDayOfMonth << Symbols::SPACE[0]
       << DateCategory::MonthData[mMonth] << Symbols::SPACE[0] // Nov
       << mYear << Symbols::SPACE[0] // 2002
      ;
   pad2(mHour, str);
   str << Symbols::COLON[0];
   pad2(mMin, str);
   str << Symbols::COLON[0];
   pad2(mSec, str);
   str << " GMT";

   return str;
}

//====================
// WarningCategory
//====================
WarningCategory::WarningCategory(const WarningCategory& rhs)
   : ParserCategory(rhs)
{}

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
     mValue(rhs.mValue)
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
      pb.data(mComment, start);
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
      mUri = rhs.mUri;
   }
   return *this;
}


void
GenericURI::parse(ParseBuffer& pb)
{
   pb.skipWhitespace();
   const char* anchor = pb.skipChar(Symbols::LA_QUOTE[0]);

   pb.skipToChar(Symbols::RA_QUOTE[0]);
   pb.data(mUri, anchor);
   pb.skipChar(Symbols::RA_QUOTE[0]);

   pb.skipWhitespace();

   parseParameters(pb);
}


Data& GenericURI::uri()
{
   checkParsed();
   return mUri;
}

ParserCategory* 
GenericURI::clone() const
{
   return new GenericURI(*this);
}

std::ostream& 
GenericURI::encode(std::ostream& str) const
{
   str << Symbols::LA_QUOTE[0]
       << mUri
       << Symbols::RA_QUOTE[0];

   encodeParameters(str);

   return str;
}

//====================
// Via:
//====================
Via::Via(const Via& rhs)
   : ParserCategory(rhs),
     mProtocolName(rhs.mProtocolName),
     mProtocolVersion(rhs.mProtocolVersion),
     mTransport(rhs.mTransport),
     mSentHost(rhs.mSentHost),
     mSentPort(rhs.mSentPort)
{}

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
   pb.data(mProtocolName, startMark);
   pb.skipToChar('/');
   pb.skipChar();
   startMark = pb.skipWhitespace();
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::SLASH);
   pb.data(mProtocolVersion, startMark);

   pb.skipToChar('/');
   pb.skipChar();
   startMark = pb.skipWhitespace();
   pb.skipNonWhitespace();
   pb.data(mTransport, startMark);

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
   pb.data(mSentHost, startMark);
   pb.skipToOneOf(";:");
   if (*pb.position() == ':')
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
Via::encode(ostream& str) const
{
   str << mProtocolName << Symbols::SLASH << mProtocolVersion << Symbols::SLASH << mTransport 
       << Symbols::SPACE << mSentHost;
   if (mSentPort != 0)
   {
      str << Symbols::COLON << mSentPort;
   }
   encodeParameters(str);
   return str;
}


//====================
// CallId:
//====================
CallId::CallId(const CallId& rhs)
   : ParserCategory(rhs),
     mValue(rhs.mValue)
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
   pb.data(mValue, start);

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
     mUri(rhs.mUri),
     mDisplayName(rhs.mDisplayName)
{}

NameAddr&
NameAddr::operator=(const NameAddr& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mAllContacts = rhs.mAllContacts;
      mDisplayName = rhs.mDisplayName;
      mUri = rhs.mUri;
   }
   return *this;
}

NameAddr::~NameAddr()
{}

ParserCategory *
NameAddr::clone() const
{
   return new NameAddr(*this);
}

Uri&
NameAddr::uri() const 
{
   checkParsed(); 
   return mUri;
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
      pb.data(mDisplayName, start);
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
         pb.data(mDisplayName, start);
         pb.skipChar(Symbols::LA_QUOTE[0]);
      }
   }
   pb.skipWhitespace();
   mUri.parse(pb);
   if (laQuote)
   {
      mUri.parseParameters(pb);
      pb.skipChar('>');
      pb.skipWhitespace();
   }
   parseParameters(pb);
   for (ParameterList::iterator it = mParameters.begin(); 
        it != mParameters.end();)
   {
      switch ((*it)->getType())
      {
         case ParameterTypes::transport:
         case ParameterTypes::ttl:
         case ParameterTypes::maddr:
         case ParameterTypes::lr:
         case ParameterTypes::method: 
         case ParameterTypes::comp:             
         {
            mUri.mParameters.push_back(*it);
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
   bool displayName = !mDisplayName.empty();
    
   if (displayName)
   {
      str << mDisplayName << Symbols::LA_QUOTE;
   }

   mUri.encode(str);

   if (displayName)
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
     mUri(rhs.mUri),
     mMethod(rhs.mMethod),
     mUnknownMethodName(rhs.mUnknownMethodName),
     mSipVersion(rhs.mSipVersion)
{}

RequestLine&
RequestLine::operator=(const RequestLine& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mUri = rhs.mUri;
      mMethod = rhs.mMethod;
      mUnknownMethodName = rhs.mUnknownMethodName;
      mSipVersion = rhs.mSipVersion;
   }
   return *this;
}

RequestLine::~RequestLine()
{}

ParserCategory *
RequestLine::clone() const
{
   return new RequestLine(*this);
}

Uri&
RequestLine::uri() const 
{
   checkParsed(); 
   return mUri;
}

void 
RequestLine::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mMethod = getMethodType(start, pb.position() - start);
   if (mMethod == UNKNOWN)
   {
      pb.data(mUnknownMethodName, start);
   }
   pb.skipWhitespace();
   mUri.parse(pb);
   mUri.parseParameters(pb);
   start = pb.skipWhitespace();
   pb.skipNonWhitespace();
   pb.data(mSipVersion, start);
}

ostream&
RequestLine::encode(ostream& str) const
{
   str << (mMethod != UNKNOWN ? MethodNames[mMethod] : mUnknownMethodName) << Symbols::SPACE;
   mUri.encode(str);
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
   pb.data(mSipVersion, start);

   start = pb.skipWhitespace();
   mResponseCode = pb.integer();
   start = pb.skipNonWhitespace();
   if (*pb.position() != ' ' && pb.position() != pb.end())
   {
      start = pb.skipChar(' ');
      pb.reset(pb.end());
      pb.data(mReason, start);
   }
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

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
