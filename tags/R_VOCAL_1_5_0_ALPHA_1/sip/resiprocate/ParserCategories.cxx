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
   mSequence = atoi(start);

   pb.skipNonWhitespace();
   start = pb.skipWhitespace();
   pb.skipNonWhitespace(); // .dcm. maybe pass an arg that says throw if you don't move
   mMethod = getMethodType(pb.data(start));
}

std::ostream& 
CSeqCategory::encode(std::ostream& str) const
{
   str << MethodNames[mMethod] << Symbols::SPACE << mSequence;
   return str;
}

//====================
// Date
//====================
DateCategory::DateCategory(const DateCategory& rhs)
   : ParserCategory(rhs),
     mValue(rhs.mValue)
{}

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

ParserCategory* IntegerCategory::clone() const
{
   return new IntegerCategory(*this);
}

void
IntegerCategory::parse(ParseBuffer& pb)
{
   const char* start = pb.skipWhitespace();
   mValue = atoi(start);
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
      mSentPort = atoi(startMark);
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
   : ParserCategory(rhs)
{}

NameAddr::~NameAddr()
{
   delete mUri;
}

ParserCategory *
NameAddr::clone() const
{
   return new NameAddr(*this);
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
   : mUri(rhs.mUri ? new Uri(*rhs.mUri) : 0),
     mMethod(rhs.mMethod),
     mSipVersion(rhs.mSipVersion)
{}

RequestLine::~RequestLine()
{
   delete mUri;
}

ParserCategory *
RequestLine::clone() const
{
   return new RequestLine(*this);
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
   mResponseCode = atoi(start);
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
