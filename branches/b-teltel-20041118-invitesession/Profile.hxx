#if !defined(RESIP_PROFILE_HXX)
#define RESIP_PROFILE_HXX

#include <iosfwd>
#include <set>
#include "resiprocate/Headers.hxx"
#include "resiprocate/MethodTypes.hxx"

namespace resip
{

class Data;

class Profile
{
   public:        
      Profile(Profile *baseProfile = 0);  // Default to no base profile
      virtual ~Profile();
      
      virtual void setDefaultRegistrationTime(int secs);
      virtual int getDefaultRegistrationTime() const;

      virtual void setDefaultSubscriptionTime(int secs);
      virtual int getDefaultSubscriptionTime() const;

      virtual void setDefaultPublicationTime(int secs);
      virtual int getDefaultPublicationTime() const;

      // Only used if timer option tag is set.
      // Note:  Add timer to options tag and set DefaultSessionTime to 0 to show  
      // support, but not request a session timer.
      virtual void setDefaultSessionTime(int secs); 
      virtual int getDefaultSessionTime() const;

      /// Call is stale if UAC gets no final response within the stale call timeout (default 3 minutes)
      virtual void setDefaultStaleCallTime(int secs);
      virtual int getDefaultStaleCallTime() const;

      //overrides the value used to populate the contact
      //?dcm? -- also change via entries? Also, dum currently uses(as a uas)
      //the request uri of the dialog constructing request for the local contact
      //within that dialog. A transport paramter here could also be used to
      //force tcp vs udp vs tls?
      virtual void setOverrideHostAndPort(const Uri& hostPort);
      virtual bool hasOverrideHostAndPort() const;
      virtual const Uri& getOverrideHostAndPort() const;      
      
	  //enable/disable sending of Allow/Supported/Accept-Language/Accept-Encoding headers 
	  //on initial outbound requests (ie. Initial INVITE, REGISTER, etc.) and Invite 200 responses
	  //Note:  Default is to advertise Headers::Allow and Headers::Supported, use clearAdvertisedCapabilities to remove these
	  //       Currently implemented header values are: Headers::Allow, 
	  //       Headers::AcceptEncoding, Headers::AcceptLanguage, Headers::Supported
	  virtual void addAdvertisedCapability(const Headers::Type header);
      virtual bool isAdvertisedCapability(const Headers::Type header) const;
	  virtual void clearAdvertisedCapabilities();
      
      virtual void setOutboundProxy( const Uri& uri );
      virtual const NameAddr& getOutboundProxy() const;
      virtual bool hasOutboundProxy() const;
      
      //defaults to false, turn on for VONAGE
      virtual void setLooseToTagMatching(bool enabled);
      virtual bool getLooseToTagMatching() const;      

      //enable/disable rport for requests. rport is enabled by default
      virtual void setRportEnabled(bool enabled);
      virtual bool getRportEnabled() const;      

	  //if set then UserAgent header is added to outbound messages
      virtual void setUserAgent( const Data& userAgent );
      virtual const Data& getUserAgent() const;
      virtual bool hasUserAgent() const;

   private:
      // !slg! - should we provide a mechanism to clear the mHasxxxxx members to re-enable fall through after setting?
	  bool mHasDefaultRegistrationExpires;
      int mDefaultRegistrationExpires;

      bool mHasDefaultSubscriptionExpires;
      int mDefaultSubscriptionExpires;

      bool mHasDefaultPublicationExpires;
      int mDefaultPublicationExpires;

      bool mHasDefaultSessionExpires;
      int mDefaultSessionExpires;

      bool mHasDefaultStaleCallTime;
      int mDefaultStaleCallTime;

      bool mHasOutboundProxy;
      NameAddr mOutboundProxy;

	  bool mHasAdvertisedCapabilities;
	  std::set<Headers::Type> mAdvertisedCapabilities;

	  bool mHasLooseToTagMatching;
      bool mLooseToTagMatching;

	  bool mHasRportEnabled;
      bool mRportEnabled;

      bool mHasUserAgent;            
      Data mUserAgent;

      bool mHasOverrideHostPort;
      Uri  mOverrideHostPort;

	  Profile *mBaseProfile;  // All non-set settings will fall through to this Profile (if set)
};

}

#endif
