
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/HeaderTypes.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


MasterProfile::MasterProfile() : 
   mValidateContentEnabled(true),
   mValidateContentLanguageEnabled(false),
   mValidateAcceptEnabled(true)
{
   // Default settings
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
MasterProfile::addSupportedScheme(const Data& scheme)
{
   mSupportedSchemes.insert(scheme);
}

bool 
MasterProfile::isSchemeSupported(const Data& scheme) const
{
   return mSupportedSchemes.count(scheme) != 0;
}

void 
MasterProfile::clearSupportedSchemes()
{
   mSupportedSchemes.clear();
}

void 
MasterProfile::addSupportedMethod(const MethodTypes& method)
{
   mSupportedMethodTypes.insert(method);
   mSupportedMethods.push_back(Token(getMethodName(method)));
}

bool 
MasterProfile::isMethodSupported(MethodTypes method) const
{
   return mSupportedMethodTypes.count(method) != 0;
}

Tokens 
MasterProfile::getAllowedMethods() const
{
   return mSupportedMethods;
}

void 
MasterProfile::clearSupportedMethods()
{
   mSupportedMethodTypes.clear();
   mSupportedMethods.clear();
}

void 
MasterProfile::addSupportedOptionTag(const Token& tag)
{
   mSupportedOptionTags.push_back(tag);
}

Tokens 
MasterProfile::getUnsupportedOptionsTags(const Tokens& requiresOptionTags)
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
MasterProfile::getSupportedOptionTags() const
{
   return mSupportedOptionTags;
}

void 
MasterProfile::clearSupportedOptionTags()
{
   mSupportedOptionTags.clear();
}

void 
MasterProfile::addSupportedMimeType(const Mime& mimeType)
{
   mSupportedMimeTypes.push_back(mimeType);
}

bool 
MasterProfile::isMimeTypeSupported(const Mime& mimeType) const
{
   return mSupportedMimeTypes.find(mimeType);
}

Mimes 
MasterProfile::getSupportedMimeTypes() const
{
   return mSupportedMimeTypes;
}

void 
MasterProfile::clearSupportedMimeTypes()
{
   mSupportedMimeTypes.clear();
}

void 
MasterProfile::addSupportedEncoding(const Token& encoding)
{
   mSupportedEncodings.push_back(encoding);
}

bool 
MasterProfile::isContentEncodingSupported(const Token& encoding) const
{
   return mSupportedEncodings.find(encoding);
}

Tokens 
MasterProfile::getSupportedEncodings() const
{
   return mSupportedEncodings;
}

void 
MasterProfile::clearSupportedEncodings()
{
   mSupportedEncodings.clear();
}

void 
MasterProfile::addSupportedLanguage(const Token& lang)
{
   mSupportedLanguages.push_back(lang);
}

bool 
MasterProfile::isLanguageSupported(const Tokens& langs) const
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
MasterProfile::getSupportedLanguages() const
{
   return mSupportedLanguages;
}

void 
MasterProfile::clearSupportedLanguages()
{
   mSupportedLanguages.clear();
}

bool& 
MasterProfile::validateContentEnabled()
{
   return mValidateContentEnabled;   
}

const bool 
MasterProfile::validateContentEnabled() const
{
   return mValidateContentEnabled;   
}

bool& 
MasterProfile::validateContentLanguageEnabled()
{
   return mValidateContentLanguageEnabled;   
}

const bool 
MasterProfile::validateContentLanguageEnabled() const
{
   return mValidateContentLanguageEnabled;   
}

bool& 
MasterProfile::validateAcceptEnabled()
{
   return mValidateAcceptEnabled;   
}

const bool 
MasterProfile::validateAcceptEnabled() const
{
   return mValidateContentEnabled;   
}

