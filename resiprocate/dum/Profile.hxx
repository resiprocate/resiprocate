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

      /// Call is stale if UAC gets no final response within the stale call timeout (default 3 minutes)
      virtual void setDefaultStaleCallTime(int secs);
      virtual int getDefaultStaleCallTime() const;

      // Only used if timer option tag is set in MasterProfile.
      // Note:  Value must be higher than 90 (as specified in session-timer draft)
      virtual void setDefaultSessionTime(int secs); 
      virtual int getDefaultSessionTime() const;

      // Only used if timer option tag is set in MasterProfile.
      // Set to PreferLocalRefreshes if you prefer that the local UA performs the refreshes.  
      // Set to PreferRemoteRefreshes if you prefer that the remote UA peforms the refreshes.
      // Set to PreferUACRefreshes if you prefer that the UAC performs the refreshes.
      // Set to PreferUASRefreshes if you prefer that the UAS performs the refreshes.
      // Note: determining the refresher is a negotiation, so despite this setting the remote 
      // end may end up enforcing their preference.  Also if the remote end doesn't support 
      // SessionTimers then the refresher will always be local.
      // This implementation follows the RECOMMENDED practices from section 7.1 of the draft 
      // and does not specify a refresher parameter as in UAC requests.
      typedef enum
      {
         PreferLocalRefreshes,
         PreferRemoteRefreshes,
         PreferUACRefreshes,
         PreferUASRefreshes
      } SessionTimerMode;
      virtual void setDefaultSessionTimerMode(Profile::SessionTimerMode mode);
      virtual Profile::SessionTimerMode getDefaultSessionTimerMode() const;

      virtual void set1xxRetransmissionTime(int secs);
      virtual int get1xxRetransmissionTime() const;

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

      bool mHasDefaultStaleCallTime;
      int mDefaultStaleCallTime;

      bool mHasDefaultSessionExpires;
      int mDefaultSessionExpires;

      bool mHasDefaultSessionTimerMode;
      SessionTimerMode mDefaultSessionTimerMode;

      bool mHas1xxRetransmissionTime;
      int m1xxRetransmissionTime;

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
