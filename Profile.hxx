#if !defined(RESIP_PROFILE_HXX)
#define RESIP_PROFILE_HXX

#include <iosfwd>
#include <set>
#include "resiprocate/Headers.hxx"
#include "resiprocate/MethodTypes.hxx"

namespace resip
{

class Data;

// !jf! will want to provide a default subclass of Profile that provides some
// good defaults for the Profile
class Profile
{
   public:  
      
      struct DigestCredential
      {
            DigestCredential();
            DigestCredential(const Data& aor, const Data& realm, const Data& username, const Data& password);
            Data aor;
            Data realm;
            Data user;
            Data password;

            bool operator<(const DigestCredential& rhs) const;
      };
      
      Profile();
      
      void setDefaultFrom(const NameAddr& from);
      NameAddr& getDefaultFrom();

      void setDefaultRegistrationTime(int secs);
      int getDefaultRegistrationTime() const;

      void setDefaultSubscriptionTime(int secs);
      int getDefaultSubscriptionTime() const;

      // Only used if timer option tag is set.
      // Note:  Add timer to options tag and set DefaultSessionTime to 0 to show  
      // support, but not request a session timer.
      void setDefaultSessionTime(int secs); 
      int getDefaultSessionTime() const;

      /// Call is stale if UAC gets no final response within the stale call timeout (default 3 minutes)
      void setDefaultStaleCallTime(int secs);
      int getDefaultStaleCallTime() const;

      //overrides the value used to populate the contact
      //?dcm? -- also change via entries? Also, dum currently uses(as a uas)
      //the request uri of the dialog constructing request for the local contact
      //within that dialog. A transport paramter here could also be used to
      //force tcp vs udp vs tls?
      void setOverrideHostAndPort(const Uri& hostPort);
      bool hasOverrideHostAndPort() const;
      const Uri& getOverideHostAndPort() const;      

      void addSupportedScheme(const Data& scheme);          // Default is "sip"
      bool isSchemeSupported(const Data& scheme) const;
      void clearSupportedSchemes(void);

      void addSupportedMethod(const MethodTypes& method);   // Defaults are: INVITE, ACK, CANCEL, OPTIONS, BYE
      bool isMethodSupported(MethodTypes method) const;
      Tokens getAllowedMethods() const;
      void clearSupportedMethods(void);

      void addSupportedOptionTag(const Token& tag);        // Default is none
      Tokens getUnsupportedOptionsTags(const Tokens& requires); // Returns list of unsupported option tags
      Tokens getSupportedOptionTags() const;
      void clearSupportedOptionTags(void);

      void addSupportedMimeType(const Mime& mimeType);      // Default is application/sdp
      bool isMimeTypeSupported(const Mime& mimeType) const;
      Mimes getSupportedMimeTypes() const;
      void clearSupportedMimeTypes(void);

      void addSupportedEncoding(const Token& encoding);     // Default is no encoding
      bool isContentEncodingSupported(const Token& contentEncoding) const;
      Tokens getSupportedEncodings() const;
      void clearSupportedEncodings(void);

      void addSupportedLanguage(const Token& lang);         // Default is all - if nothing is set, then all are allowed
      bool isLanguageSupported(const Tokens& lang) const;
      Tokens getSupportedLanguages() const;

      void clearSupportedLanguages(void);
      
	  //enable/disable sending of Allow/Supported/Accept-Language/Accept-Encoding headers 
	  //on initial outbound requests (ie. Initial INVITE, REGISTER, etc.) and Invite 200 responses
	  //Note:  Default is to advertise Headers::Allow and Headers::Supported, use clearAdvertisedCapabilities to remove these
	  //       Currently implemented header values are: Headers::Allow, 
	  //       Headers::AcceptEncoding, Headers::AcceptLanguage, Headers::Supported
	  void addAdvertisedCapability(const Headers::Type header);
      bool isAdvertisedCapability(const Headers::Type header) const;
	  void clearAdvertisedCapabilities();

      void addGruu(const Data& aor, const NameAddr& contact);
      bool hasGruu(const Data& aor) const;
      bool hasGruu(const Data& aor, const Data& instance) const;
      NameAddr& getGruu(const Data& aor);
      NameAddr& getGruu(const Data& aor, const NameAddr& contact);
      void disableGruu();
      
      void setOutboundProxy( const Uri& uri );
      const NameAddr& getOutboundProxy() const;
      bool hasOutboundProxy() const;
      
      void setUAName(const Data& name);
      const Data& getUAName() const;

      /// The following functions deal with getting digest credentals 
      //@{ 
      void setDigestCredential( const Data& aor, const Data& realm, const Data& user, const Data& password);
      
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
      
      DigestCredentialHandler* getDigestHandler();
      const DigestCredential& getDigestCredential( const Data& realm );
      const DigestCredential& getDigestCredential( const SipMessage& challenge );      

      //defaults to false, turn on for VONAGE
      bool& looseToTagMatching();      
      const bool looseToTagMatching() const;      

      //enable/disable rport for requests. rport is enabled by default
      bool& rportEnabled();      
      const bool rportEnabled() const; 

      //enable/disable content validation
      bool& validateContentEnabled();
      const bool validateContentEnabled() const;

      //enable/disable content language validation
      bool& validateContentLanguageEnabled();
      const bool validateContentLanguageEnabled() const;

      //enable/disable Accept header validation
      bool& validateAcceptEnabled();
      const bool validateAcceptEnabled() const;

	  //if set then UserAgent header is added to outbound messages
      void setUserAgent( const Data& userAgent );
      const Data& getUserAgent() const;
      bool hasUserAgent() const;

   private:
      NameAddr mDefaultFrom;
      int mDefaultRegistrationExpires;
      int mDefaultSubscriptionExpires;
      int mDefaultSessionExpires;
      int mDefaultStaleCallTime;

      bool mHasOutboundProxy;
      NameAddr mOutboundProxy;

      Data mUAName;
      
      std::set<Data> mSupportedSchemes;
      std::set<MethodTypes> mSupportedMethodTypes;
      Tokens mSupportedMethods;
      Tokens mSupportedOptionTags;
      Mimes mSupportedMimeTypes;
      Tokens mSupportedEncodings;
      Tokens mSupportedLanguages;
	  std::set<Headers::Type> mAdvertisedCapabilities;

      DigestCredentialHandler* mDigestCredentialHandler;

      bool mLooseToTagMatching;
      bool mRportEnabled;
      bool mValidateContentEnabled;
      bool mValidateContentLanguageEnabled;
      bool mValidateAcceptEnabled;
      bool mHasUserAgent;      
      
      Data mUserAgent;
      Uri  mOverrideHostPort;
      bool mHasOverrideHostPort;
      
      typedef std::set<DigestCredential> DigestCredentials;
      DigestCredentials mDigestCredentials;
};
  
std::ostream& 
operator<<(std::ostream&, const Profile::DigestCredential& cred);
 
}

#endif
