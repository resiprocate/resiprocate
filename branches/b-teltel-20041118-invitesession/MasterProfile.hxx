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
      
       // Creates an Indentity/Profile with no BaseProfile - this is the root of all profiles
       MasterProfile();  
      
      // Default is "sip"
      virtual void addSupportedScheme(const Data& scheme);          
      virtual bool isSchemeSupported(const Data& scheme) const;
      virtual void clearSupportedSchemes(void);

      // Defaults are: INVITE, ACK, CANCEL, OPTIONS, BYE
      virtual void addSupportedMethod(const MethodTypes& method);   
      virtual bool isMethodSupported(MethodTypes method) const;
      virtual Tokens getAllowedMethods() const;
      virtual void clearSupportedMethods(void);

      // Default is none
      virtual void addSupportedOptionTag(const Token& tag);        
      virtual Tokens getUnsupportedOptionsTags(const Tokens& requires); // Returns list of unsupported option tags
      virtual Tokens getSupportedOptionTags() const;
      virtual void clearSupportedOptionTags(void);

      // Default is application/sdp for INVITE, OPTIONS, PRACK and UPDATE Methods
      virtual void addSupportedMimeType(const MethodTypes& method, const Mime& mimeType);      
      virtual bool isMimeTypeSupported(const MethodTypes& method, const Mime& mimeType);
      virtual Mimes getSupportedMimeTypes(const MethodTypes& method);
      virtual void clearSupportedMimeTypes(const MethodTypes& method);
      virtual void clearSupportedMimeTypes(void);  // Clear for all Methods

      // Default is no encoding
      virtual void addSupportedEncoding(const Token& encoding);     
      virtual bool isContentEncodingSupported(const Token& contentEncoding) const;
      virtual Tokens getSupportedEncodings() const;
      virtual void clearSupportedEncodings(void);

      // Default is all - if nothing is set, then all are allowed
      virtual void addSupportedLanguage(const Token& lang);         
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
      std::map<MethodTypes, Mimes> mSupportedMimeTypes;
      Tokens mSupportedEncodings;
      Tokens mSupportedLanguages;

      bool mValidateContentEnabled;
      bool mValidateContentLanguageEnabled;
      bool mValidateAcceptEnabled;
};
   
}

#endif
