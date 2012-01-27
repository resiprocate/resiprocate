/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <set>
#include <sstream>

#include "resip/stack/Embedded.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/NameAddr.hxx" 
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/UnknownParameter.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP
#define HANDLE_CHARACTER_ESCAPING //undef for old behaviour

// class static variables listing the default characters not to encode
// in user and password strings respectively
const Data Uri::mUriNonEncodingUserChars = Data("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.!~*\\()&=+$,;?/");
const Data Uri::mUriNonEncodingPasswordChars = Data("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.!~*\\()&=+$");

// ?bwc? 'p' and 'w' are allowed in 2806, but have been removed in 3966. Should
// we support these or not?
const Data Uri::mLocalNumberChars = Data("*#-.()0123456789ABCDEFpw");
const Data Uri::mGlobalNumberChars = Data("-.()0123456789");


Uri::EncodingTable Uri::mUriEncodingUserTable(Data::toBitset(mUriNonEncodingUserChars).flip());
Uri::EncodingTable Uri::mUriEncodingPasswordTable(Data::toBitset(mUriNonEncodingPasswordChars).flip());
Uri::EncodingTable Uri::mLocalNumberTable(Data::toBitset(mLocalNumberChars));
Uri::EncodingTable Uri::mGlobalNumberTable(Data::toBitset(mGlobalNumberChars));
std::set<resip::Data> Uri::theEnumSchemes(Uri::initEnumSchemes());

std::set<resip::Data> 
Uri::initEnumSchemes()
{
   std::set<resip::Data> result;
   result.insert("tel");
   return result;
}


Uri::Uri(PoolBase* pool) 
   : ParserCategory(pool),
     mScheme(Data::Share, Symbols::DefaultSipScheme),
     mPort(0),
     mHostCanonicalized(false),
     mEmbeddedHeadersText(0),
     mEmbeddedHeaders(0)
{
}

Uri::Uri(const HeaderFieldValue& hfv, Headers::Type type, PoolBase* pool) :
   ParserCategory(hfv, type, pool),
   mPort(0),
   mHostCanonicalized(false),
   mEmbeddedHeadersText(0),
   mEmbeddedHeaders(0)
{}


static const Data parseContext("Uri constructor");
Uri::Uri(const Data& data)
   : ParserCategory(), 
     mScheme(Symbols::DefaultSipScheme),
     mPort(0),
     mHostCanonicalized(false),
     mEmbeddedHeadersText(0),
     mEmbeddedHeaders(0)
{
   HeaderFieldValue hfv(data.data(), data.size());
   // must copy because parse creates overlays
   Uri tmp(hfv, Headers::UNKNOWN);
   tmp.checkParsed();
   *this = tmp;
}

Uri::Uri(const Uri& rhs,
         PoolBase* pool)
   : ParserCategory(rhs, pool),
     mScheme(rhs.mScheme),
     mHost(rhs.mHost),
     mUser(rhs.mUser),
     mUserParameters(rhs.mUserParameters),
     mPort(rhs.mPort),
     mPassword(rhs.mPassword),
     mHostCanonicalized(rhs.mHostCanonicalized),
     mEmbeddedHeadersText(rhs.mEmbeddedHeadersText ? new Data(*rhs.mEmbeddedHeadersText) : 0),
     mEmbeddedHeaders(rhs.mEmbeddedHeaders ? new SipMessage(*rhs.mEmbeddedHeaders) : 0)
{}


Uri::~Uri()
{
   delete mEmbeddedHeaders;
   delete mEmbeddedHeadersText;
}

// RFC 3261 19.1.6
#if 0  // deprecated
Uri
Uri::fromTel(const Uri& tel, const Data& host)
{
   assert(tel.scheme() == Symbols::Tel);

   Uri u;
   u.scheme() = Symbols::Sip;
   u.user() = tel.user();
   u.host() = host;
   u.param(p_user) = Symbols::Phone;

   // need to sort the user parameters
   if (!tel.userParameters().empty())
   {
      DebugLog(<< "Uri::fromTel: " << tel.userParameters());
      Data isub;
      Data postd;

      int totalSize  = 0;
      std::set<Data> userParameters;

      ParseBuffer pb(tel.userParameters().data(), tel.userParameters().size());
      while (true)
      {
         const char* anchor = pb.position();
         pb.skipToChar(Symbols::SEMI_COLON[0]);
         Data param = pb.data(anchor);
         // !dlb! not supposed to lowercase extension parameters
         param.lowercase();
         totalSize += (int)param.size() + 1;

         if (param.prefix(Symbols::Isub))
         {
            isub = param;
         }
         else if (param.prefix(Symbols::Postd))
         {
            postd = param;
         }
         else
         {
            userParameters.insert(param);
         }
         if (pb.eof())
         {
            break;
         }
         else
         {
            pb.skipChar();
         }
      }

      u.userParameters().reserve(totalSize);
      if (!isub.empty())
      {
         u.userParameters() = isub;
      }
      if (!postd.empty())
      {
         if (!u.userParameters().empty())
         {
            u.userParameters() += Symbols::SEMI_COLON[0];
         }
         u.userParameters() += postd;
      }
      
      for(std::set<Data>::const_iterator i = userParameters.begin();
          i != userParameters.end(); ++i)
      {
         DebugLog(<< "Adding param: " << *i);
         if (!u.userParameters().empty())
         {
            u.userParameters() += Symbols::SEMI_COLON[0];
         }
         u.userParameters() += *i;
      }
   }

   return u;
}
#endif // deprecated

Uri
Uri::fromTel(const Uri& tel, const Uri& hostUri)
{
   assert(tel.scheme() == Symbols::Tel);

   Uri u(hostUri);
   u.scheme() = Symbols::Sip;
   u.user() = tel.user();
   u.param(p_user) = Symbols::Phone;

   // need to sort the user parameters
   if (tel.numKnownParams() || tel.numUnknownParams())
   {
      std::ostringstream telParams;
      tel.encodeParameters(telParams);
      DebugLog(<< "Uri::fromTel: " << telParams.str().c_str()+1);
      Data isub;
      Data postd;
      Data telParamStr (telParams.str());
      int totalSize  = 0;
      std::set<Data> userParameters;
      
      ParseBuffer pb(telParamStr.data(), telParamStr.size());
      while (true)
      {
         const char* anchor = pb.position();
         pb.skipToChar(Symbols::SEMI_COLON[0]);
         Data param = pb.data(anchor);
         DebugLog(<< "Uri::fromTel: "<<param);
         // !dlb! not supposed to lowercase extension parameters
         param.lowercase();
         totalSize += param.size() + 1;
         if (param.empty())
         {
            
         }
         else if (param.prefix(Symbols::Isub))
         {
            isub = param;
         }
         else if (param.prefix(Symbols::Postd))
         {
            postd = param;
         }
         else
         {
            userParameters.insert(param);
         }
         if (pb.eof())
         {
            break;
         }
         else
         {
            pb.skipChar();
         }
      }

      u.userParameters().reserve(totalSize);
      if (!isub.empty())
      {
         u.userParameters() = isub;
      }
      if (!postd.empty())
      {
         if (!u.userParameters().empty())
         {
            u.userParameters() += Symbols::SEMI_COLON[0];
         }
         u.userParameters() += postd;
      }
      
      for(std::set<Data>::const_iterator i = userParameters.begin();
          i != userParameters.end(); ++i)
      {
         DebugLog(<< "Adding param: " << *i);
         if (!u.userParameters().empty())
         {
            u.userParameters() += Symbols::SEMI_COLON[0];
         }
         u.userParameters() += *i;
      }
   }

   return u;
}

bool
Uri::isEnumSearchable() const
{
   checkParsed();
   return (!mUser.empty() && mUser.size() >= 2 && mUser[0] == '+' && theEnumSchemes.count(mScheme)!=0);
}

std::vector<Data> 
Uri::getEnumLookups(const std::vector<Data>& suffixes) const
{
   std::vector<Data> results;
   Data prefix;
   if (isEnumSearchable())
   {
      // skip the leading +
      for (const char* i=user().end()-1 ; i!= user().begin(); --i)
      {
         if (isdigit(*i))
         {
            prefix += *i;
            prefix += Symbols::DOT;
         }
      }
      if(!prefix.empty())
      {
         for (std::vector<Data>::const_iterator j=suffixes.begin(); j != suffixes.end(); ++j)
         {
            results.push_back(prefix + *j);
         }
      }
   }
   return results;
}


bool
Uri::hasEmbedded() const
{
   checkParsed(); 
   return (mEmbeddedHeadersText && !mEmbeddedHeadersText->empty()) || mEmbeddedHeaders != 0;
}

void 
Uri::removeEmbedded()
{
   checkParsed();
   delete mEmbeddedHeaders;
   mEmbeddedHeaders = 0;
   delete mEmbeddedHeadersText;
   mEmbeddedHeadersText = 0;
}



Uri&
Uri::operator=(const Uri& rhs)
{
   if (this != &rhs)
   {
      ParserCategory::operator=(rhs);
      mScheme = rhs.mScheme;
      mHost = rhs.mHost;
      mHostCanonicalized=rhs.mHostCanonicalized;
      mUser = rhs.mUser;
      mUserParameters = rhs.mUserParameters;
      mPort = rhs.mPort;
      mPassword = rhs.mPassword;
      if (rhs.mEmbeddedHeaders != 0)
      {
         delete mEmbeddedHeaders;
         mEmbeddedHeaders = new SipMessage(*rhs.mEmbeddedHeaders);
      }
      else if(rhs.mEmbeddedHeadersText != 0)
      {
         if(!mEmbeddedHeadersText)
         {
            mEmbeddedHeadersText = new Data(*rhs.mEmbeddedHeadersText);
         }
         else
         {
            // !bwc! Data::operator= is smart enough to handle this safely.
            *mEmbeddedHeadersText = *rhs.mEmbeddedHeadersText;
         }
      }
   }
   return *this;
}

/**
  @class OrderUnknownParameters
  @brief used as a comparator for sorting purposes
  */
class OrderUnknownParameters
{
   public:
      /**
        @brief constructor ; never called explicitly
        */
      OrderUnknownParameters() { notUsed=false; };

      /**
        @brief empty destructor
        */
      ~OrderUnknownParameters() {};

      /**
        @brief used as a comparator for sorting purposes
         This does a straight Data comparison for name and returns true/false
        @param p1 pointer to parameter 1
        @param p2 pointer to parameter 2
        @return true if p1->getName() is less than p2->getName() 
         else return false
        */
      bool operator()(const Parameter* p1, const Parameter* p2) const
      {
         return dynamic_cast<const UnknownParameter*>(p1)->getName() < dynamic_cast<const UnknownParameter*>(p2)->getName();
      }

   private:
      bool notUsed;
};

bool 
Uri::operator==(const Uri& other) const
{
   checkParsed();
   other.checkParsed();

   // compare hosts
   if (DnsUtil::isIpV6Address(mHost) &&
       DnsUtil::isIpV6Address(other.mHost))
   {

      // compare canonicalized IPV6 addresses

      // update canonicalized if host changed
      if (!mHostCanonicalized)
      {
         mHost = DnsUtil::canonicalizeIpV6Address(mHost);
         mHostCanonicalized=true;
      }

      // update canonicalized if host changed
      if (!other.mHostCanonicalized)
      {
         other.mHost = DnsUtil::canonicalizeIpV6Address(other.mHost);
         other.mHostCanonicalized=true;
      }

      if (mHost != other.mHost)
      {
         return false;
      }
   }
   else
   {
      if (!isEqualNoCase(mHost, other.mHost))
      {
         return false;
      }
   }
   
   if (isEqualNoCase(mScheme, other.mScheme) &&
       ((isEqualNoCase(mScheme, Symbols::Sip) || isEqualNoCase(mScheme, Symbols::Sips)) ? mUser == other.mUser : isEqualNoCase(mUser, other.mUser)) &&
       isEqualNoCase(mUserParameters,other.mUserParameters) &&
       mPassword == other.mPassword &&
       mPort == other.mPort)
   {
      for (ParameterList::const_iterator it = mParameters.begin(); it != mParameters.end(); ++it)
      {
         Parameter* otherParam = other.getParameterByEnum((*it)->getType());

         switch ((*it)->getType())
         {
            case ParameterTypes::user:
            {
               if (!(otherParam &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(otherParam)->value())))
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::ttl:
            {
               if (!(otherParam &&
                     (dynamic_cast<UInt32Parameter*>(*it)->value() ==
                      dynamic_cast<UInt32Parameter*>(otherParam)->value())))
               {
                  return false;
               }
               break;
            }
            case ParameterTypes::method:
            {
               // this should possibly be case sensitive, but is allowed to be
               // case insensitive for robustness.  
               
               if (otherParam)
               {
                  DataParameter* dp1 = dynamic_cast<DataParameter*>(*it);
                  DataParameter* dp2 = dynamic_cast<DataParameter*>(otherParam);
                  (void)dp1;
                  (void)dp2;
                  // ?bwc? It looks like we're just assuming the dynamic_cast 
                  // will succeed everywhere else; why are we bothering to 
                  // assert()?
                  assert(dp1);
                  assert(dp2);
               }
               if (!(otherParam &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(otherParam)->value())))
               {
                  return false;
               }
               break;
            }
            case ParameterTypes::maddr:
            {               
               if (!(otherParam &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(otherParam)->value())))
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::transport:
            {
               if (!(otherParam &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(otherParam)->value())))
               {
                  return false;
               }
            }
            break;
            // the parameters that follow don't affect comparison if only present
            // in one of the URI's
            case ParameterTypes::lr:
               break;
            default:
               break;
               //treat as unknown parameter?
         }
      }         

      // now check the other way, sigh
      for (ParameterList::const_iterator it = other.mParameters.begin(); it != other.mParameters.end(); ++it)
      {
         Parameter* param = getParameterByEnum((*it)->getType());
         switch ((*it)->getType())
         {
            case ParameterTypes::user:
            {
               if (!(param &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(param)->value())))
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::ttl:
            {
               if (!(param &&
                     (dynamic_cast<UInt32Parameter*>(*it)->value() == 
                      dynamic_cast<UInt32Parameter*>(param)->value())))
               {
                  return false;
               }
               break;
            }
            case ParameterTypes::method:
            {
               // this should possilby be case sensitive, but is allowed to be
               // case insensitive for robustness.  
               if (!(param &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(param)->value())))
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::maddr:
            {               
               if (!(param &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(param)->value())))
               {
                  return false;
               }
            }
            break;
            case ParameterTypes::transport:
            {
               if (!(param &&
                     isEqualNoCase(dynamic_cast<DataParameter*>(*it)->value(),
                                   dynamic_cast<DataParameter*>(param)->value())))
               {
                  return false;
               }
            }
            break;
            // the parameters that follow don't affect comparison if only present
            // in one of the URI's
            case ParameterTypes::lr:
               break;
            default:
               break;
               //treat as unknown parameter?
         }
      }
   }
   else
   {
      return false;
   }

   OrderUnknownParameters orderUnknown;

#if defined(__SUNPRO_CC) || defined(WIN32) || defined(__sun__)
   // The Solaris Forte STL implementation does not support the
   // notion of a list.sort() function taking a BinaryPredicate.
   // The hacky workaround is to load the Parameter pointers into
   // an STL set which does support an ordering function.

   typedef std::set<Parameter*, OrderUnknownParameters> ParameterSet;
   ParameterSet unA, unB;

   for (ParameterList::iterator i = mUnknownParameters.begin();
        i != mUnknownParameters.end(); ++i)
   {
      unA.insert(*i);
   }
   for (ParameterList::iterator i = other.mUnknownParameters.begin();
        i != other.mUnknownParameters.end(); ++i)
   {
      unB.insert(*i);
   }

   ParameterSet::iterator a = unA.begin();
   ParameterSet::iterator b = unB.begin();
#else
   // .dlb. more efficient to copy to vector for sorting?
   // Uri comparison is expensive; consider caching? ugh
   ParameterList unA = mUnknownParameters;
   ParameterList unB = other.mUnknownParameters;

   sort(unA.begin(), unA.end(), orderUnknown);
   sort(unB.begin(), unB.end(), orderUnknown);
 
   ParameterList::iterator a = unA.begin();
   ParameterList::iterator b = unB.begin();
#endif

   while(a != unA.end() && b != unB.end())
   {
      if (orderUnknown(*a, *b))
      {
         ++a;
      }
      else if (orderUnknown(*b, *a))
      {
         ++b;
      }
      else
      {
         if (!isEqualNoCase(dynamic_cast<UnknownParameter*>(*a)->value(),
                            dynamic_cast<UnknownParameter*>(*b)->value()))
         {
            return false;
         }
         ++a;
         ++b;
      }
   }
   return true;
}

bool 
Uri::operator!=(const Uri& other) const
{
   return !(*this == other);
}

bool
Uri::operator<(const Uri& other) const
{
   other.checkParsed();
   checkParsed();
   if (mUser < other.mUser)
   {
      return true;
   }

   if (mUser > other.mUser)
   {
      return false;
   }

   if (mUserParameters < other.mUserParameters)
   {
      return true;
   }

   if (mUserParameters > other.mUserParameters)
   {
      return false;
   }

   // !bwc! Canonicalize before we compare! Jeez...
   if (!mHostCanonicalized)
   {
      if(DnsUtil::isIpV6Address(mHost))
      {
         mHost = DnsUtil::canonicalizeIpV6Address(mHost);
      }
      else
      {
         mHost.lowercase();
      }
      mHostCanonicalized=true;
   }
   
   if (!other.mHostCanonicalized)
   {
      if(DnsUtil::isIpV6Address(other.mHost))
      {
         other.mHost = DnsUtil::canonicalizeIpV6Address(other.mHost);
      }
      else
      {
         other.mHost.lowercase();
      }
      other.mHostCanonicalized=true;
   }

   if (mHost < other.mHost)
   {
      return true;
   }

   if (mHost > other.mHost)
   {
      return false;
   }

   return mPort < other.mPort;
}

bool
Uri::aorEqual(const resip::Uri& rhs) const
{
   checkParsed();
   rhs.checkParsed();

   if (!mHostCanonicalized)
   {
      if(DnsUtil::isIpV6Address(mHost))
      {
         mHost = DnsUtil::canonicalizeIpV6Address(mHost);
      }
      else
      {
         mHost.lowercase();
      }
      mHostCanonicalized=true;
   }
   
   if (!rhs.mHostCanonicalized)
   {
      if(DnsUtil::isIpV6Address(rhs.mHost))
      {
         rhs.mHost = DnsUtil::canonicalizeIpV6Address(rhs.mHost);
      }
      else
      {
         rhs.mHost.lowercase();
      }
      rhs.mHostCanonicalized=true;
   }
   
   return (mUser==rhs.mUser) && (mHost==rhs.mHost) && (mPort==rhs.mPort) && 
            (isEqualNoCase(mScheme,rhs.mScheme));
}

void 
Uri::getAorInternal(bool dropScheme, bool addPort, Data& aor) const
{
   checkParsed();
   // canonicalize host

   addPort = addPort && mPort!=0;

   bool hostIsIpV6Address = false;
   if(!mHostCanonicalized)
   {
      if (DnsUtil::isIpV6Address(mHost))
      {
         mHost = DnsUtil::canonicalizeIpV6Address(mHost);
         hostIsIpV6Address = true;
      }
      else
      {
         mHost.lowercase();
      }
   }

   // !bwc! Maybe reintroduce caching of aor. (Would use a bool instead of the
   // mOldX cruft)
   //                                                  @:10000
   aor.clear();
   aor.reserve((dropScheme ? 0 : mScheme.size()+1)
               + mUser.size() + mHost.size() + 7);
   if(!dropScheme)
   {
      aor += mScheme;
      aor += ':';
   }

   if (!mUser.empty())
   {
#ifdef HANDLE_CHARACTER_ESCAPING
      {
         oDataStream str(aor);
         mUser.escapeToStream(str, mUriEncodingUserTable); 
      }
#else
      aor += mUser;
#endif
      if(!mHost.empty())
      {
         aor += Symbols::AT_SIGN;
      }
   }

   if(hostIsIpV6Address && addPort)
   {
      aor += Symbols::LS_BRACKET;
      aor += mHost;
      aor += Symbols::RS_BRACKET;
   }
   else
   {
      aor += mHost;
   }

   if(addPort)
   {
      aor += Symbols::COLON;
      aor += Data(mPort);
   }
}

Data 
Uri::getAOR(bool addPort) const
{
   Data result;
   getAorInternal(false, addPort, result);
   return result;
}

bool 
Uri::userIsTelephoneSubscriber() const
{
   try
   {
      ParseBuffer pb(mUser);
      pb.assertNotEof();
      const char* anchor=pb.position();
      bool local=false;
      if(*pb.position()=='+')
      {
         // Might be a global phone number
         pb.skipChar();
         pb.skipChars(mGlobalNumberTable);
      }
      else
      {
         pb.skipChars(mLocalNumberTable);
         local=true;
      }

      Data dialString(pb.data(anchor));
      if(dialString.empty())
      {
         pb.fail(__FILE__, __LINE__, "Dial string is empty.");
      }

      // ?bwc? More dial-string checking? For instance, +/ (or simply /) is not 
      // a valid dial-string according to the BNF; the string must contain at 
      // least one actual digit (or in the local number case, one hex digit or 
      // '*' or '#'. Interestingly, this means that stuff like ///*/// is 
      // valid)

      // Dial string looks ok so far; now look for params (there must be a 
      // phone-context param if this is a local number, otherwise there might 
      // or might not be one)
      if(local || !pb.eof())
      {
         // The only thing that can be here is a ';'. If it does, we're going 
         // to say it is good enough for us. If something in the parameter 
         // string is malformed, it'll get caught when/if 
         // getUserAsTelephoneSubscriber() is called.
         pb.skipChar(';');
      }

      return true;
   }
   catch(ParseException& e)
   {
      return false;
   }
}

Token 
Uri::getUserAsTelephoneSubscriber() const
{
   // !bwc! Ugly. Someday, refactor all this lazy-parser stuff and make it 
   // possible to control ownership explicitly.
   // Set this up as lazy-parsed, to prevent exceptions from being thrown.
   HeaderFieldValue temp(mUser.data(), mUser.size());
   Token tempToken(temp, Headers::NONE);
   // tempToken does not own the HeaderFieldValue temp, and temp does not own 
   // its buffer.

   // Here's the voodoo; invoking operator= makes a deep copy of the stuff in
   // tempToken, with result owning the memory, and result is in the unparsed 
   // state.
   Token result = tempToken;
   return result;
}

void 
Uri::setUserAsTelephoneSubscriber(const Token& telephoneSubscriber)
{
   mUser.clear();
   oDataStream str(mUser);
   str << telephoneSubscriber;
}

Data 
Uri::getUserAtHost(bool addPort) const
{
   Data result;
   getAorInternal(true, addPort, result);
   return result;
}


Uri 
Uri::getAorAsUri() const
{   
   //.dcm. -- tel conversion?
   checkParsed();
   Uri ret;
   ret.scheme() = mScheme;   
   ret.user() = mUser;
   ret.host() = mHost;
   ret.port() = mPort;
   
   return ret;
}

void
Uri::parse(ParseBuffer& pb)
{
   pb.skipWhitespace();
   const char* start = pb.position();
   // The '@' is for causing the parse to fail on bob@foo (missing scheme)
   // This doesn't work for Uris with only a host:port, though.
   pb.skipToOneOf<':','@'>();

   pb.data(mScheme, start);
   pb.skipChar(Symbols::COLON[0]);
   mScheme.schemeLowercase();

   if (mScheme==Symbols::Tel)
   {
      const char* anchor = pb.position();
      static std::bitset<256> delimiter=Data::toBitset("\r\n\t ;>");
      pb.skipToOneOf(delimiter);
      pb.data(mUser, anchor);
      if (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0])
      {
         parseParameters(pb);
      }
      return;
   }
   
   start = pb.position();
   pb.skipToOneOf<':','@','\"'>();
   if (!pb.eof())
   {
      const char* atSign=0;
      if (*pb.position() == Symbols::COLON[0])
      {
         // Either a password, or a port
         const char* afterColon = pb.skipChar();
         pb.skipToOneOf<'@','\"'>();
         if(!pb.eof() && *pb.position() == Symbols::AT_SIGN[0])
         {
            atSign=pb.position();
            // password
#ifdef HANDLE_CHARACTER_ESCAPING
            pb.dataUnescaped(mPassword, afterColon);
#else
            pb.data(mPassword, afterColon);
#endif
            pb.reset(afterColon-1);
         }
         else
         {
            // port. No user part.
            pb.reset(start);
         }
      }
      else if(*pb.position() == Symbols::AT_SIGN[0])
      {
         atSign=pb.position();
      }
      else
      {
         // Only a hostpart
         pb.reset(start);
      }

      if(atSign)
      {
#ifdef HANDLE_CHARACTER_ESCAPING
         pb.dataUnescaped(mUser, start);
#else
         pb.data(mUser, start);
#endif
         pb.reset(atSign);
         start = pb.skipChar();
      }
   }
   else
   {
      pb.reset(start);
   }

   mHostCanonicalized=false;
   if (*start == '[')
   {
      start = pb.skipChar();
      pb.skipToChar(']');
      pb.data(mHost, start);
      // .bwc. We do not save this canonicalization, since we weren't doing so
      // before. This may change soon.
      Data canonicalizedHost=DnsUtil::canonicalizeIpV6Address(mHost);
      if(canonicalizedHost.empty())
      {
         // .bwc. So the V6 addy is garbage.
         throw ParseException("Unparsable V6 address (note, this might"
                                    " be unparsable because IPV6 support is not"
                                    " enabled)","Uri",__FILE__,
                                       __LINE__);
      }
      pb.skipChar();
      static std::bitset<256> delimiter=Data::toBitset("\r\n\t :;?>");
      pb.skipToOneOf(delimiter);
   }
   else
   {
      static std::bitset<256> delimiter=Data::toBitset("\r\n\t :;?>");
      pb.skipToOneOf(delimiter);
      pb.data(mHost, start);
   }

   if (!pb.eof() && *pb.position() == ':')
   {
      start = pb.skipChar();
      mPort = pb.uInt32();
   }
   else
   {
      mPort = 0;
   }
   
   parseParameters(pb);

   if (!pb.eof() && *pb.position() == Symbols::QUESTION[0])
   {
      const char* anchor = pb.position();
      pb.skipToOneOf(">;", ParseBuffer::Whitespace);
      if(!mEmbeddedHeadersText) mEmbeddedHeadersText = new Data;
      pb.data(*mEmbeddedHeadersText, anchor);
   }
}

ParserCategory*
Uri::clone() const
{
   return new Uri(*this);
}

ParserCategory*
Uri::clone(void* location) const
{
   return new (location) Uri(*this);
}

ParserCategory*
Uri::clone(PoolBase* pool) const
{
   return new (pool) Uri(*this);
}

void Uri::setUriUserEncoding(unsigned char c, bool encode) 
{
   mUriEncodingUserTable[c] = encode; 
}

void Uri::setUriPasswordEncoding(unsigned char c, bool encode)
{
   mUriEncodingPasswordTable[c] = encode;
}

// should not encode user parameters unless its a tel?
std::ostream& 
Uri::encodeParsed(std::ostream& str) const
{
   str << mScheme << Symbols::COLON; 
   if (!mUser.empty())
   {
#ifdef HANDLE_CHARACTER_ESCAPING
      mUser.escapeToStream(str, mUriEncodingUserTable); 
#else
      str << mUser;
#endif
      if (!mUserParameters.empty())
      {
         str << Symbols::SEMI_COLON[0] << mUserParameters;
      }
      if (!mPassword.empty())
      {
         str << Symbols::COLON;
#ifdef HANDLE_CHARACTER_ESCAPING
         mPassword.escapeToStream(str, mUriEncodingPasswordTable);
#else
         str << mPassword;
#endif
      }
   }
   if (!mHost.empty())
   {
     if (!mUser.empty())
     {
       str << Symbols::AT_SIGN;
     }
     if (DnsUtil::isIpV6Address(mHost))
     {
        str << '[' << mHost << ']';
     }
     else
     {
        str << mHost;
     }
   }
   if (mPort != 0)
   {
      str << Symbols::COLON;
      oDataStream::fastEncode(mPort, str);
   }
   encodeParameters(str);
   encodeEmbeddedHeaders(str);

   return str;
}

SipMessage&
Uri::embedded()
{
   checkParsed();
   if (mEmbeddedHeaders == 0)
   {
      this->mEmbeddedHeaders = new SipMessage();
      if (mEmbeddedHeadersText && !mEmbeddedHeadersText->empty())
      {
         ParseBuffer pb(mEmbeddedHeadersText->data(), mEmbeddedHeadersText->size());
         this->parseEmbeddedHeaders(pb);
      }
   }

   return *mEmbeddedHeaders;
}

const SipMessage&
Uri::embedded() const
{
   Uri* ncthis = const_cast<Uri*>(this);
   return ncthis->embedded();
}

static const Data bodyData("Body");
void
Uri::parseEmbeddedHeaders(ParseBuffer& pb)
{
   DebugLog(<< "Uri::parseEmbeddedHeaders");
   if (!pb.eof() && *pb.position() == Symbols::QUESTION[0])
   {
      pb.skipChar();
   }

   const char* anchor;
   Data headerName;
   Data headerContents;

   bool first = true;
   while (!pb.eof())
   {
      if (first)
      {
         first = false;
      }
      else
      {
         pb.skipChar(Symbols::AMPERSAND[0]);
      }

      anchor = pb.position();
      pb.skipToChar(Symbols::EQUALS[0]);
      pb.data(headerName, anchor);
      // .dlb. in theory, need to decode header name

      anchor = pb.skipChar(Symbols::EQUALS[0]);
      pb.skipToChar(Symbols::AMPERSAND[0]);
      pb.data(headerContents, anchor);

      unsigned int len;
      char* decodedContents = Embedded::decode(headerContents, len);
      mEmbeddedHeaders->addBuffer(decodedContents);

      if (isEqualNoCase(bodyData, headerName))
      {
         mEmbeddedHeaders->setBody(decodedContents, len); 
      }
      else
      {
         DebugLog(<< "Uri::parseEmbeddedHeaders(" << headerName << ", " << Data(decodedContents, len) << ")");
         mEmbeddedHeaders->addHeader(Headers::getType(headerName.data(), (int)headerName.size()),
                                     headerName.data(), (int)headerName.size(),
                                     decodedContents, len);
      }
   }
}

std::ostream& 
Uri::encodeEmbeddedHeaders(std::ostream& str) const
{
   if (mEmbeddedHeaders)
   {
      mEmbeddedHeaders->encodeEmbedded(str);
   }
   else if(mEmbeddedHeadersText)
   {
      // never decoded
      str << *mEmbeddedHeadersText;
   }
   return str;
}

Data 
Uri::toString() const
{
   Data out;
   {
      oDataStream dataStream(out);
      this->encodeParsed(dataStream);
   }
   return out;
}

HashValueImp(resip::Uri, resip::Data::from(data).hash());

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
