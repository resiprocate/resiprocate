#include <algorithm>
#include <iostream>
#include <cassert>

#include <util/Data.hxx>
#include <util/Logger.hxx>
#include <sipstack/ParserCategories.hxx>
#include <sipstack/Uri.hxx>

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
Token::parse()
{
   const char* pstart = getHeaderField().mField;
   const char* pos = find(pstart, pstart+getHeaderField().mFieldLength, Symbols::SEMI_COLON[0]);
   mValue = Data(pstart, pos-pstart);
   DebugLog (<< "Token::parse() --> " << mValue);
   parseParameters(pos, pos-pstart);
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
Mime::parse()
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
Auth::parse()
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
CSeqCategory::parse()
{
   // !jf! this does not need to copy the memory until after parsing - see Token
   Data number;
   Data method = Data(mHeaderField->mField, mHeaderField->mFieldLength);
   int ret = method.match(" ", &number, true);
   if (ret == FOUND)
   {
      mSequence = number.convertInt();
      mMethod = getMethodType(method);
   }
   else if (ret == NOT_FOUND)
   {
      DebugLog(<< "Failed to parse CSeq: " << Data(mHeaderField->mField, mHeaderField->mFieldLength));
      throw ParseException("failed parse of CSeq", __FILE__,__LINE__);
   }
   else if (ret == FIRST)
   {
      DebugLog(<< "Failed to parse CSeq: " << Data(mHeaderField->mField, mHeaderField->mFieldLength));
      throw ParseException("failed parse of CSeq", __FILE__,__LINE__);
   }
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
DateCategory::parse()
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
WarningCategory::parse()
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
IntegerCategory::parse()
{

  Data rawdata = Data(mHeaderField->mField, mHeaderField->mFieldLength);

  // look for a comment
  Data comment;
  int retn = rawdata.match("(", &comment, true);
  Data params;

  // Starts with a comment, bad
  if (retn == FIRST)
  {
     DebugLog(<< "Failed to parse Integer: " << Data(mHeaderField->mField, mHeaderField->mFieldLength));
     throw ParseException("failed parse of Integer", __FILE__,__LINE__);
  }

  // we have a comment, handle it
  else if (retn == FOUND)
  {
     // right now only look to verify that they close
     // we really need to also look for escaped \) parens (or anything
     // else for that matter) and also replace cr lf with two spaces
      
     Data remainder;
     retn = comment.match(")", &remainder, true);
      
     // if it is not found, they never close, if it is first, the comment
     // is empty. Either is illegal
     if (retn != FOUND)
     {
        DebugLog(<< "Failed to parse Integer: " << Data(mHeaderField->mField, mHeaderField->mFieldLength));
        throw ParseException("failed parse of Integer", __FILE__,__LINE__);
     }

     mComment = comment;

     // we should immediately see a ; or nothing, so see if there 
     // is anything after the )
      
     if (remainder.size() != 0)
     {
        retn = remainder.match(";", &params, true);
        if (retn != FIRST)
        {
           // something between the comment and the ;, or something
           // after the ) and no ;. Both are bad.
           DebugLog(<< "Failed to parse Integer: " << Data(mHeaderField->mField, mHeaderField->mFieldLength));
           throw ParseException("failed parse of Integer", __FILE__,__LINE__);
        }
        else
        {
           // walk params and populate list
        }
     }
      
     mValue = rawdata.convertInt();
  }
  
  // no comment
  else if (retn == NOT_FOUND)
  {
      
     int ret = rawdata.match(";", &params, true);
     if (ret == FOUND)
     {
        // walk params and populate list
     }
     
     mValue = rawdata.convertInt();
  }
}

std::ostream& 
IntegerCategory::encode(std::ostream& str) const
{
  str << mValue;
  if (!mComment.empty())
    {
      str << "(" << mComment << ")";
    }
  
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
StringCategory::parse()
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
GenericURI::parse()
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
Via::parse()
{
   assert(0);
}

ostream&
Via::encode(ostream& str) const
{
   assert(0);
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
CallId::parse()
{
   assert(0);
}

ostream&
CallId::encode(ostream& str) const
{
   str << mValue;
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
NameAddr::parse()
{
   assert(0);
}

ostream&
NameAddr::encode(ostream& str) const
{
   assert(0);
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
RequestLine::parse()
{
   mUri = new Uri();
   assert(0);
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
StatusLine::parse()
{
   assert(0);
}

ostream&
StatusLine::encode(ostream& str) const
{
   str << mSipVersion << Symbols::SPACE 
       << mResponseCode << Symbols::SPACE
       << mReason;
   return str;
}
