#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/ParserCategories.hxx"

using namespace resip;

Profile::Profile() : mDefaultRegistrationExpires(3600)
{
}

void
Profile::setDefaultAor(const NameAddr& from)
{
   mAor = from;
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
   mSupportedMethods.push_back(Token(MethodNames[method]));
}

void 
Profile::addSupportedOptionTags(const Token& tag)
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

NameAddr& 
Profile::getDefaultAor()
{
   return mAor;
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
Profile::isSupported(const Tokens& requiresOptionTags)
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
Profile::setOutboundProxy( const Data& uri )
{
}

void 
Profile::disableGruu()
{
}

void 
Profile::addDigestCredential( const Data& realm, const Data& users, const Data& password)
{
}
