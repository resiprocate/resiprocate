#if !defined(RESIP_PROFILE_HXX)
#define RESIP_PROFILE_HXX

namespace resip
{

class Data;
class Mime;
class Token;


class Profile
{
   public:
      bool isSchemeSupported(const Data& scheme);
      bool isMethodSupported(MethodTypes method);
      bool isMimeTypeSupported(const Mime& mimeType);
      bool isContentEncodingSupported(const Token& contentEncoding);
      bool isLanguageSupported(const Tokens& lang);
      
      // return the list of unsupported tokens from set of requires tokens
      Tokens isSupported(Token& requires);
      Tokens getSupportedOptionTags();
      Tokens getAllowedMethods();
      Mimes getSupportedMimeTypes();
      Tokens getSupportedEncodings();
      Tokens getSupportedLanguages();
      NameAddr& getDefaultAor();
      
      void addSupportedScheme(const Data& scheme);
      void addSupportedMethod(const MethodTypes& method);
      void addSuportedOptionTags(const Token& tag);
      void addSupportedMimeType(const Mime& mimeType);
      void addSupportedEncoding(const Token& encoding);
      void addSupportedLanguage(const Token& lang);

      void setDefaultAor(const NameAddr& from);
      const NameAddr& getDefaultAor() const;

      void setOutboundProxy( const Data& uri );
      void disableGruu();

      /// The following functions deal with getting digest credentals 
      //@{ 

      void addDigestCredential( const Data& realm, const Data& users, const Data& password);
      
      /** This class is used as a callback to get digest crednetials. The
       * derived class must define one of computeA1 or getPaswword. computeA1 is
       * tried first and it it returns an empty string, then getPassword is
       * tried. */
      class DigestCredentialHandler
      {
         public:
            virtual Data computeA1( const Data& realm, const Data& users );
            virtual Data getPassword( const Data& realm, const Data& users );
      };
      
      void setDigestHandler( DigestCredentialHandler* handler );
      //@}
         
   private:
};
   

 
}

#endif
