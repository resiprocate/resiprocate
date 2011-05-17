#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/NameAddr.hxx"
#include "rutil/ParseException.hxx"
#include "resip/stack/UnknownParameter.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// NameAddr:
//====================
NameAddr::NameAddr() : 
   ParserCategory(),
   mAllContacts(false),
   mDisplayName()
{}

NameAddr::NameAddr(HeaderFieldValue* hfv,
                   Headers::Type type)
   : ParserCategory(hfv, type), 
     mAllContacts(false),
     mDisplayName()
{}

NameAddr::NameAddr(const NameAddr& rhs)
   : ParserCategory(rhs),
     mAllContacts(rhs.mAllContacts),
     mUri(rhs.mUri),
     mDisplayName(rhs.mDisplayName)
{}

static const Data parseContext("NameAddr constructor");
NameAddr::NameAddr(const Data& unparsed, bool preCacheAor)
   : ParserCategory(),
     mAllContacts(false),
     mDisplayName()
{
   HeaderFieldValue hfv(unparsed.data(), unparsed.size());
   // must copy because parse creates overlays
   NameAddr tmp(&hfv, Headers::UNKNOWN);
   tmp.checkParsed();
   *this = tmp;
   if(preCacheAor)
   {
      mUri.getAor();
   }
}

NameAddr::NameAddr(const Uri& uri)
   : ParserCategory(),
     mAllContacts(false),
     mUri(uri),
     mDisplayName()
{}

NameAddr::~NameAddr()
{}

NameAddr&
NameAddr::operator=(const NameAddr& rhs)
{
   if (this != &rhs)
   {
      assert( &rhs != 0 );
      
      ParserCategory::operator=(rhs);
      mAllContacts = rhs.mAllContacts;
      mDisplayName = rhs.mDisplayName;
      mUri = rhs.mUri;
   }
   return *this;
}

bool 
NameAddr::operator==(const NameAddr& other) const
{
    return uri() == other.uri() && displayName() == other.displayName();
}

bool
NameAddr::operator<(const NameAddr& rhs) const
{
   return uri() < rhs.uri();
}

ParserCategory *
NameAddr::clone() const
{
   return new NameAddr(*this);
}

const Uri&
NameAddr::uri() const 
{
   checkParsed(); 
   return mUri;
}

Uri&
NameAddr::uri()
{
   checkParsed(); 
   return mUri;
}

Data& 
NameAddr::displayName()
{
   checkParsed(); 
   return mDisplayName;
}

const Data& 
NameAddr::displayName() const 
{
   checkParsed(); 
   return mDisplayName;
}

bool 
NameAddr::isAllContacts() const 
{
   checkParsed(); 
   return mAllContacts;
}

void 
NameAddr::setAllContacts()
{
   mAllContacts = true;
}

void
NameAddr::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   bool laQuote = false;
   bool starContact = false;
   
   if (!pb.eof() && *pb.position() == Symbols::STAR[0])
   {
      pb.skipChar(Symbols::STAR[0]);
      pb.skipWhitespace();
      if (pb.eof() || *pb.position() == Symbols::SEMI_COLON[0])
      {
         starContact = true;
      }
   }

   if (starContact)
   {
      mAllContacts = true;
      // now fall through to parse header parameters
   }
   else
   {
      pb.reset(start);
      if (!pb.eof() && *pb.position() == Symbols::DOUBLE_QUOTE[0])
      {
         start = pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
         pb.skipToEndQuote();
         pb.data(mDisplayName, start);
         pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
         laQuote = true;
         pb.skipToChar(Symbols::LA_QUOTE[0]);
         if (pb.eof())
         {
            throw ParseException("Expected '<'", 
                                 "NameAddr", 
                                 __FILE__, 
                                 __LINE__);
         }
         else
         {
            pb.skipChar(Symbols::LA_QUOTE[0]);
         }
      }
      else if (!pb.eof() && *pb.position() == Symbols::LA_QUOTE[0])
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
            pb.skipBackWhitespace();
            pb.data(mDisplayName, start);
            pb.skipToChar(Symbols::LA_QUOTE[0]);
            pb.skipChar(Symbols::LA_QUOTE[0]);
         }
      }
      pb.skipWhitespace();
      mUri.parse(pb);
      if (laQuote)
      {
         pb.skipChar(Symbols::RA_QUOTE[0]);
         pb.skipWhitespace();
         // now fall through to parse header parameters
      }
      else
      {
         Data temp;
         {
            oDataStream str(temp);
            // deal with Uri/NameAddr parameter ambiguity
            // heuristically assign Uri parameters to the Uri
            for (ParameterList::iterator it = mUri.mUnknownParameters.begin(); 
                 it != mUri.mUnknownParameters.end(); ++it)
            {
               // We're just going to assume all unknown (to Uri) params really
               // belong on the header. This is not necessarily the case.
               str << ";";
               (*it)->encode(str);
            }
            mUri.clearUnknownParameters();
         }
         ParseBuffer pb2(temp);
         parseParameters(pb2);
      }
   }
   parseParameters(pb);
}

EncodeStream&
NameAddr::encodeParsed(EncodeStream& str) const
{
   //bool displayName = !mDisplayName.empty();
  if (mAllContacts)
  {
     str << Symbols::STAR;
  }
  else
  {
     if (!mDisplayName.empty())
     {
#ifndef HANDLE_EMBEDDED_QUOTES_DNAME
        // .dlb. doesn't deal with embedded quotes
        str << Symbols::DOUBLE_QUOTE << mDisplayName << Symbols::DOUBLE_QUOTE;
#else
        // does nothing if display name is properly quoted
        if (mustQuoteDisplayName())
        {
           str << Symbols::DOUBLE_QUOTE;
           for (unsigned int i=0; i < mDisplayName.size(); i++)
           {
              char c = mDisplayName[i];
              switch(c)
              {
                 case '"':
                 case '\\':
                    str << '\\' << c;
                    break;
                 default:
                    str << c;
              }
           }
           str << Symbols::DOUBLE_QUOTE;
        }
        else
        {
           str << mDisplayName;           
        }
#endif
     }     
     str << Symbols::LA_QUOTE;
     mUri.encodeParsed(str);
     str << Symbols::RA_QUOTE;
  }
  
  encodeParameters(str);
  return str;
}


bool 
NameAddr::mustQuoteDisplayName() const
{
   if (mDisplayName.empty())
   {
      return false;
   }
   ParseBuffer pb(mDisplayName.data(), mDisplayName.size());   
   
   //shouldn't really be any leading whitespace
   pb.skipWhitespace();
   if (pb.eof())
   {
      return false;
   }
   if ((*pb.position() == '"'))
   {
      bool escaped = false;
      while(!pb.eof())
      {
         pb.skipChar();
         if (escaped)
         {
            escaped = false;
         }
         else if (*pb.position() == '\\')
         {
            escaped = true;
         }
         else if (*pb.position() == '"')
         {
            break;
         }
      }
      if (*pb.position() == '"')
      {
         //should only have whitespace left, and really non of that
         pb.skipChar();
         if (pb.eof())
         {
            return false;
         }
         pb.skipWhitespace();
         if (pb.eof())
         {
            return false; //properly quoted
         }
         else
         {
            return true; 
         }
      }
      else
      {
         return true; //imbalanced quotes
      }
   }
   else
   {
      while (!pb.eof())
      {
         const char* start;
         start = pb.skipWhitespace();
         pb.skipNonWhitespace();
		 const char* end = pb.position();
         for (const char* c = start; c < end; c++)
         {
            if ( (*c >= 'a' && *c <= 'z') ||
                 (*c >= 'A' && *c <= 'Z') ||
                 (*c >= '0' && *c <= '9'))
            {
               continue;
            }
            switch(*c)
            {
               case '-':
               case '.':
               case '!':
               case '%':
               case '*':
               case '_':
               case '+':
               case '`':
               case '\'':
               case '~':
                  break;
               default:
                  return true;
            }
         }
      }
   }
   return false;
}

ParameterTypes::Factory NameAddr::ParameterFactories[ParameterTypes::MAX_PARAMETER]={0};

Parameter* 
NameAddr::createParam(ParameterTypes::Type type, ParseBuffer& pb, const char* terminators)
{
   if(ParameterFactories[type])
   {
      return ParameterFactories[type](type, pb, terminators);
   }
   return 0;
}

bool 
NameAddr::exists(const Param<NameAddr>& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

void 
NameAddr::remove(const Param<NameAddr>& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

// BEGIN AUTOGENERATED

ExistsParameter::Type&
NameAddr::param(const data_Param& paramType)
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ExistsParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const ExistsParameter::Type&
NameAddr::param(const data_Param& paramType) const
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"data\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"data\"", __FILE__, __LINE__);
   }
   return p->value();
}


ExistsParameter::Type&
NameAddr::param(const control_Param& paramType)
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ExistsParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const ExistsParameter::Type&
NameAddr::param(const control_Param& paramType) const
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"control\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"control\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const mobility_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const mobility_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"mobility\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"mobility\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const description_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const description_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"description\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"description\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const events_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const events_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"events\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"events\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const priority_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const priority_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"priority\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"priority\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const methods_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const methods_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"methods\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"methods\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const schemes_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const schemes_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"schemes\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"schemes\"", __FILE__, __LINE__);
   }
   return p->value();
}


ExistsParameter::Type&
NameAddr::param(const application_Param& paramType)
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ExistsParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const ExistsParameter::Type&
NameAddr::param(const application_Param& paramType) const
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"application\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"application\"", __FILE__, __LINE__);
   }
   return p->value();
}


ExistsParameter::Type&
NameAddr::param(const video_Param& paramType)
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ExistsParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const ExistsParameter::Type&
NameAddr::param(const video_Param& paramType) const
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"video\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"video\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const language_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const language_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"language\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"language\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const type_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const type_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"type\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"type\"", __FILE__, __LINE__);
   }
   return p->value();
}


ExistsParameter::Type&
NameAddr::param(const isFocus_Param& paramType)
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ExistsParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const ExistsParameter::Type&
NameAddr::param(const isFocus_Param& paramType) const
{
   checkParsed();
   ExistsParameter* p =
      static_cast<ExistsParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"isfocus\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"isfocus\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const actor_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const actor_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"actor\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"actor\"", __FILE__, __LINE__);
   }
   return p->value();
}


ExistsOrDataParameter::Type&
NameAddr::param(const text_Param& paramType)
{
   checkParsed();
   ExistsOrDataParameter* p =
      static_cast<ExistsOrDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ExistsOrDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const ExistsOrDataParameter::Type&
NameAddr::param(const text_Param& paramType) const
{
   checkParsed();
   ExistsOrDataParameter* p =
      static_cast<ExistsOrDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"text\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"text\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const extensions_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const extensions_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"extensions\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"extensions\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const Instance_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const Instance_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"+sip.instance\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"+sip.instance\"", __FILE__, __LINE__);
   }
   return p->value();
}


UInt32Parameter::Type&
NameAddr::param(const regid_Param& paramType)
{
   checkParsed();
   UInt32Parameter* p =
      static_cast<UInt32Parameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new UInt32Parameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const UInt32Parameter::Type&
NameAddr::param(const regid_Param& paramType) const
{
   checkParsed();
   UInt32Parameter* p =
      static_cast<UInt32Parameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"reg-id\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"reg-id\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const pubGruu_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const pubGruu_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"pub-gruu\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"pub-gruu\"", __FILE__, __LINE__);
   }
   return p->value();
}


QuotedDataParameter::Type&
NameAddr::param(const tempGruu_Param& paramType)
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QuotedDataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QuotedDataParameter::Type&
NameAddr::param(const tempGruu_Param& paramType) const
{
   checkParsed();
   QuotedDataParameter* p =
      static_cast<QuotedDataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"temp-gruu\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"temp-gruu\"", __FILE__, __LINE__);
   }
   return p->value();
}


UInt32Parameter::Type&
NameAddr::param(const expires_Param& paramType)
{
   checkParsed();
   UInt32Parameter* p =
      static_cast<UInt32Parameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new UInt32Parameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const UInt32Parameter::Type&
NameAddr::param(const expires_Param& paramType) const
{
   checkParsed();
   UInt32Parameter* p =
      static_cast<UInt32Parameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"expires\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"expires\"", __FILE__, __LINE__);
   }
   return p->value();
}


QValueParameter::Type&
NameAddr::param(const q_Param& paramType)
{
   checkParsed();
   QValueParameter* p =
      static_cast<QValueParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new QValueParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const QValueParameter::Type&
NameAddr::param(const q_Param& paramType) const
{
   checkParsed();
   QValueParameter* p =
      static_cast<QValueParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"q\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"q\"", __FILE__, __LINE__);
   }
   return p->value();
}


DataParameter::Type&
NameAddr::param(const tag_Param& paramType)
{
   checkParsed();
   DataParameter* p =
      static_cast<DataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new DataParameter(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

const DataParameter::Type&
NameAddr::param(const tag_Param& paramType) const
{
   checkParsed();
   DataParameter* p =
      static_cast<DataParameter*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      InfoLog(<< "Missing parameter \"tag\" " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);
      DebugLog(<< *this);
      throw Exception("Missing parameter \"tag\"", __FILE__, __LINE__);
   }
   return p->value();
}

// END AUTOGENERATED

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
