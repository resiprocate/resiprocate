#if !defined(RESIP_MASTERPROFILE_HXX)
#define RESIP_MASTERPROFILE_HXX

#include <iosfwd>
#include <set>
#include "resiprocate/Headers.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/dum/UserProfile.hxx"

namespace resip
{

class Data;

class MasterProfile : public UserProfile
{
   public:  
      
	  MasterProfile();  // Creates an Indentity/Profile with no BaseProfile - this is the root of all profiles
      
      virtual void addSupportedScheme(const Data& scheme);          // Default is "sip"
      virtual bool isSchemeSupported(const Data& scheme) const;
      virtual void clearSupportedSchemes(void);

      virtual void addSupportedMethod(const MethodTypes& method);   // Defaults are: INVITE, ACK, CANCEL, OPTIONS, BYE
      virtual bool isMethodSupported(MethodTypes method) const;
      virtual Tokens getAllowedMethods() const;
      virtual void clearSupportedMethods(void);

      virtual void addSupportedOptionTag(const Token& tag);        // Default is none
      virtual Tokens getUnsupportedOptionsTags(const Tokens& requires); // Returns list of unsupported option tags
      virtual Tokens getSupportedOptionTags() const;
      virtual void clearSupportedOptionTags(void);

      virtual void addSupportedMimeType(const Mime& mimeType);      // Default is application/sdp
      virtual bool isMimeTypeSupported(const Mime& mimeType) const;
      virtual Mimes getSupportedMimeTypes() const;
      virtual void clearSupportedMimeTypes(void);

      virtual void addSupportedEncoding(const Token& encoding);     // Default is no encoding
      virtual bool isContentEncodingSupported(const Token& contentEncoding) const;
      virtual Tokens getSupportedEncodings() const;
      virtual void clearSupportedEncodings(void);

      virtual void addSupportedLanguage(const Token& lang);         // Default is all - if nothing is set, then all are allowed
      virtual bool isLanguageSupported(const Tokens& lang) const;
      virtual Tokens getSupportedLanguages() const;
      virtual void clearSupportedLanguages(void);
      
      //enable/disable content validation
      virtual bool& validateContentEnabled();
      virtual const bool validateContentEnabled() const;

      //enable/disable content language validation
      virtual bool& validateContentLanguageEnabled();
      virtual const bool validateContentLanguageEnabled() const;

      //enable/disable Accept header validation
      virtual bool& validateAcceptEnabled();
      virtual const bool validateAcceptEnabled() const;

   private:
      std::set<Data> mSupportedSchemes;
      std::set<MethodTypes> mSupportedMethodTypes;
      Tokens mSupportedMethods;
      Tokens mSupportedOptionTags;
      Mimes mSupportedMimeTypes;
      Tokens mSupportedEncodings;
      Tokens mSupportedLanguages;

      bool mValidateContentEnabled;
      bool mValidateContentLanguageEnabled;
      bool mValidateAcceptEnabled;
};
   
}

#endif
