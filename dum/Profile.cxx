
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/HeaderTypes.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


Profile::Profile() : 
   mDefaultRegistrationExpires(3600),
   mHasOutboundProxy(false),
   mLooseToTagMatching(false),
   mRportEnabled(true),
   mValidateContentEnabled(true),
   mValidateContentLanguageEnabled(false),
   mValidateAcceptEnabled(true),
   mHasUserAgent(false),
   mHasOverrideHostPort(true)
{
   // Default settings
   addAdvertisedCapability(Headers::Allow);  
   addAdvertisedCapability(Headers::Supported);  
   addSupportedMimeType(Mime("application", "sdp"));
   addSupportedLanguage(Token("en"));
   addSupportedMethod(INVITE);
   addSupportedMethod(ACK);
   addSupportedMethod(CANCEL);
   addSupportedMethod(OPTIONS);
   addSupportedMethod(BYE);
   addSupportedScheme(Symbols::Sip);  
}

void
Profile::setDefaultFrom(const NameAddr& from)
{
   mDefaultFrom = from;
}

void
Profile::setDefaultRegistrationTime(int secs)
{
   mDefaultRegistrationExpires = secs;
}

void 
Profile::addSupportedScheme(const Data& scheme)
{
   mSupportedSchemes.insert(scheme);
}

void 
Profile::addSupportedMethod(const MethodTypes& method)
{
   mSupportedMethodTypes.insert(method);
   mSupportedMethods.push_back(Token(getMethodName(method)));
}

void 
Profile::addSupportedOptionTag(const Token& tag)
{
   mSupportedOptionTags.push_back(tag);
}

void 
Profile::addSupportedMimeType(const Mime& mimeType)
{
   mSupportedMimeTypes.push_back(mimeType);
}

void 
Profile::addSupportedEncoding(const Token& encoding)
{
   mSupportedEncodings.push_back(encoding);
}

void 
Profile::addSupportedLanguage(const Token& lang)
{
   mSupportedLanguages.push_back(lang);
}

void 
Profile::clearSupportedSchemes()
{
   mSupportedSchemes.clear();
}

void 
Profile::clearSupportedMethods()
{
   mSupportedMethodTypes.clear();
   mSupportedMethods.clear();
}

void 
Profile::clearSupportedOptionTags()
{
   mSupportedOptionTags.clear();
}

void 
Profile::clearSupportedMimeTypes()
{
   mSupportedMimeTypes.clear();
}

void 
Profile::clearSupportedEncodings()
{
   mSupportedEncodings.clear();
}

void 
Profile::clearSupportedLanguages()
{
   mSupportedLanguages.clear();
}

NameAddr& 
Profile::getDefaultFrom()
{
   return mDefaultFrom;
}

int 
Profile::getDefaultSubscriptionTime()
{
   return 3600;
}

int 
Profile::getDefaultRegistrationTime()
{
   return mDefaultRegistrationExpires;
}

bool 
Profile::isSchemeSupported(const Data& scheme)
{
   return mSupportedSchemes.count(scheme) != 0;
}

bool 
Profile::isMethodSupported(MethodTypes method)
{
   return mSupportedMethodTypes.count(method) != 0;
}

bool 
Profile::isMimeTypeSupported(const Mime& mimeType)
{
   return mSupportedMimeTypes.find(mimeType);
}

bool 
Profile::isContentEncodingSupported(const Token& encoding)
{
   return mSupportedEncodings.find(encoding);
}

bool 
Profile::isLanguageSupported(const Tokens& langs)
{
   for (Tokens::const_iterator i=langs.begin(); i != langs.end(); ++i)
   {
      if (mSupportedLanguages.find(*i) == false)
      {
         return false;
      }
   }
   return true;
}

Tokens 
Profile::getUnsupportedOptionsTags(const Tokens& requiresOptionTags)
{
   Tokens tokens;
   for (Tokens::const_iterator i=requiresOptionTags.begin(); i != requiresOptionTags.end(); ++i)
   {
      // if this option is not supported
      if (!mSupportedOptionTags.find(*i))
      {
         tokens.push_back(*i);
      }
   }
   
   return tokens;
}

Tokens 
Profile::getSupportedOptionTags()
{
   return mSupportedOptionTags;
}

Tokens 
Profile::getAllowedMethods()
{
   return mSupportedMethods;
}

Mimes 
Profile::getSupportedMimeTypes()
{
   return mSupportedMimeTypes;
}

Tokens 
Profile::getSupportedEncodings()
{
   return mSupportedEncodings;
}

Tokens 
Profile::getSupportedLanguages()
{
   return mSupportedLanguages;
}

void 
Profile::addAdvertisedCapability(const Headers::Type header)
{
   assert(header == Headers::Allow ||
		  header == Headers::AcceptEncoding ||
		  header == Headers::AcceptLanguage ||
		  header == Headers::Supported);

   mAdvertisedCapabilities.insert(header);
}
 
bool 
Profile::isAdvertisedCapability(const Headers::Type header)
{
   return mAdvertisedCapabilities.count(header) != 0;
}

void 
Profile::clearAdvertisedCapabilities(void)
{
   return mAdvertisedCapabilities.clear();
}

void 
Profile::addGruu(const Data& aor, const NameAddr& contact)
{
}

bool 
Profile::hasGruu(const Data& aor)
{
   return false;
}

bool 
Profile::hasGruu(const Data& aor, const Data& instance)
{
   return false;
}

NameAddr&
Profile:: getGruu(const Data& aor)
{
   assert(0);
   static NameAddr gruu;
   return gruu;
}

NameAddr&
Profile:: getGruu(const Data& aor, const NameAddr& contact)
{
   assert(0);
   static NameAddr gruu;
   return gruu;
}

void 
Profile::setOutboundProxy( const Uri& uri )
{
   mOutboundProxy = NameAddr(uri);
   mHasOutboundProxy = true;
}

const NameAddr&
Profile::getOutboundProxy() const
{
   assert(mHasOutboundProxy);
   return mOutboundProxy;
}

bool
Profile::hasOutboundProxy() const
{
   return mHasOutboundProxy;
}
   
void 
Profile::disableGruu()
{
   assert(0);
}

void 
Profile::setDigestCredential( const Data& aor, const Data& realm, const Data& user, const Data& password)
{
   DigestCredential cred(aor, realm, user, password);
   DebugLog (<< "Adding credential: " << cred);
   mDigestCredentials.erase(cred);
   mDigestCredentials.insert(cred);
}
     
Profile::DigestCredentialHandler* 
Profile::getDigestHandler()
{
   return mDigestCredentialHandler;
}

const Profile::DigestCredential&
Profile::getDigestCredential( const Data& realm )
{
   DigestCredential dc;
   dc.realm = realm;
   
   DigestCredentials::const_iterator i = mDigestCredentials.find(dc);
   if (i != mDigestCredentials.end())
   {
      return *i;
   }
   
   static const DigestCredential empty;
   return empty;
}

const Profile::DigestCredential&
Profile::getDigestCredential( const SipMessage& challenge )
{
   StackLog (<< Inserter(mDigestCredentials));
   DebugLog (<< "Using From header: " <<  challenge.header(h_From).uri().getAor() << " to find credential");   
   const Data& aor = challenge.header(h_From).uri().getAor();
   for (DigestCredentials::const_iterator it = mDigestCredentials.begin(); 
        it != mDigestCredentials.end(); it++)
   {
      if (it->aor == aor)
      {
         return *it;
      }
   }
   const Data& user = challenge.header(h_From).uri().user();
   for (DigestCredentials::const_iterator it = mDigestCredentials.begin(); 
        it != mDigestCredentials.end(); it++)
   {
      if (it->user == user)
      {
         return *it;
      }
   }

   // !jf! why not just throw here? 
   static const DigestCredential empty;
   return empty;
}

Profile::DigestCredential::DigestCredential(const Data& a, const Data& r, const Data& u, const Data& p) :
   aor(a),
   realm(r),
   user(u),
   password(p)
{
}

Profile::DigestCredential::DigestCredential() : 
   aor(Data::Empty),
   realm(Data::Empty),
   user(Data::Empty),
   password(Data::Empty)
{
}  

bool
Profile::DigestCredential::operator<(const DigestCredential& rhs) const
{
   if (realm < rhs.realm)
   {
      return true;
   }
   else if (realm == rhs.realm)
   {
      return aor < rhs.aor;
   }
   else
   {
      return false;
   }
}

std::ostream&
resip::operator<<(std::ostream& strm, const Profile::DigestCredential& cred)
{
   strm << "realm=" << cred.realm 
        << " aor=" << cred.aor
        << " user=" << cred.user ;
   return strm;
}

bool& 
Profile::looseToTagMatching()
{
   return mLooseToTagMatching;
}

const bool 
Profile::looseToTagMatching() const
{
   return mLooseToTagMatching;
}

bool& 
Profile::rportEnabled()
{
   return mRportEnabled;   
}

const bool 
Profile::rportEnabled() const
{
   return mRportEnabled;   
}

bool& 
Profile::validateContentEnabled()
{
   return mValidateContentEnabled;   
}

const bool 
Profile::validateContentEnabled() const
{
   return mValidateContentEnabled;   
}

bool& 
Profile::validateContentLanguageEnabled()
{
   return mValidateContentLanguageEnabled;   
}

const bool 
Profile::validateContentLanguageEnabled() const
{
   return mValidateContentLanguageEnabled;   
}

bool& 
Profile::validateAcceptEnabled()
{
   return mValidateContentEnabled;   
}

const bool 
Profile::validateAcceptEnabled() const
{
   return mValidateContentEnabled;   
}

void 
Profile::setUserAgent( const Data& userAgent )
{
   mHasUserAgent = true;   
   mUserAgent = userAgent;   
}

const Data& 
Profile::getUserAgent() const
{
   assert(mHasUserAgent);
   return mUserAgent;
}
 
bool 
Profile::hasUserAgent() const
{
   return mHasUserAgent;
}

bool 
Profile::hasOverrideHostAndPort() const
{
   return mHasOverrideHostPort;
}

void 
Profile::setOverrideHostAndPort(const Uri& hostPort)
{
   mHasOverrideHostPort = true;   
   mOverrideHostPort = hostPort;   
}

const Uri& 
Profile::getOverideHostAndPort() const
{
   return mOverrideHostPort;
}
   
