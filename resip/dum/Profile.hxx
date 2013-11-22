#if !defined(RESIP_PROFILE_HXX)
#define RESIP_PROFILE_HXX

#include <iosfwd>
#include <set>
#include "resip/stack/Headers.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "rutil/SharedPtr.hxx"
#include "resip/stack/MessageDecorator.hxx"

namespace resip
{

class Data;

class Profile
{
   public:        
      Profile();  // Default to no base profile
      Profile(SharedPtr<Profile> baseProfile);
      virtual ~Profile();

      /// Reset this profile to it's initial state - (same as calling unsetXXX for each setting)
      /// If no fall through (base) profile was provided as creation time 
      ///   - Reset all settings to Default settings  
      /// else
      ///   - Reset all setting to fall through to base profile      
      virtual void reset();  

      /// Note:
      /// setXXXX methods will set this setting internally in this object.  If you do not call
      /// a particular setXXX method on this object then a corresponding getXXXX call will attempt
      /// to retrieve that value from the BaseProfile (provided in the constructor).  This allows
      /// you to setup a heirarchy of profiles and settings.
      /// unsetXXX methods are used to re-enable fallthrough after calling a setXXXX method.  If
      /// you call an unsetXXXX method on an object with a NULL BaseProfile the setting will return to
      /// it's creation time default.  Note:  Defaults are described below.

      /// This default is used if no value is passed in when creating a registration
      virtual void setDefaultRegistrationTime(UInt32 secs);
      virtual UInt32 getDefaultRegistrationTime() const;
      virtual void unsetDefaultRegistrationTime();  

      /// If a registration gets rejected with a 423, then we ensure the MinExpires value is less than this before re-registering
      /// Set to 0 to disable this check and accept any time suggested by the server.
      virtual void setDefaultMaxRegistrationTime(UInt32 secs);
      virtual UInt32 getDefaultMaxRegistrationTime() const;
      virtual void unsetDefaultMaxRegistrationTime();   

      /// The time to retry registrations on error responses (if Retry-After header is not present in error)
      /// Set to 0 to never retry on errors.  Note:  onRequestRetry is called before this setting is
      /// checked.  Return -1 from onRequestRetry in order to use this setting.
      virtual void setDefaultRegistrationRetryTime(int secs);
      virtual int getDefaultRegistrationRetryTime() const;
      virtual void unsetDefaultRegistrationRetryTime();   

      /// This default is used if no value is passed in when creating a subscription
      virtual void setDefaultSubscriptionTime(UInt32 secs);
      virtual UInt32 getDefaultSubscriptionTime() const;
      virtual void unsetDefaultSubscriptionTime();   

      /// This default is used if no value is passed in when creating a publication
      virtual void setDefaultPublicationTime(UInt32 secs);
      virtual UInt32 getDefaultPublicationTime() const;
      virtual void unsetDefaultPublicationTime();  

      /// Call is stale if UAC gets no final response within the stale call timeout (default 3 minutes)
      virtual void setDefaultStaleCallTime(int secs);
      virtual int getDefaultStaleCallTime() const;
      virtual void unsetDefaultStaleCallTime();  

      /// ReInvite is stale if UAC gets no final response within the stale reinvite timeout (default 40 seconds)
      virtual void setDefaultStaleReInviteTime(int secs);
      virtual int getDefaultStaleReInviteTime() const;
      virtual void unsetDefaultStaleReInviteTime();  

      /// Only used if timer option tag is set in MasterProfile.
      /// Note:  Value must be higher than 90 (as specified in RFC 4028)
      virtual void setDefaultSessionTime(UInt32 secs); 
      virtual UInt32 getDefaultSessionTime() const;
      virtual void unsetDefaultSessionTime(); 

      /// Only used if timer option tag is set in MasterProfile.
      /// Set to PreferLocalRefreshes if you prefer that the local UA performs the refreshes.  
      /// Set to PreferRemoteRefreshes if you prefer that the remote UA peforms the refreshes.
      /// Set to PreferCallerRefreshes if you prefer that the Caller performs the refreshes.
      /// Set to PreferCalleeRefreshes if you prefer that the Callee (called party) performs the refreshes.
      /// Note: determining the refresher is a negotiation, so despite this setting the remote 
      /// end may end up enforcing their preference.  Also if the remote end doesn't support 
      /// SessionTimers then the refresher will always be local.
      /// This implementation follows the RECOMMENDED practices from section 7.1 of the draft 
      /// and does not specify a refresher parameter in UAC requests.
      typedef enum
      {
         PreferLocalRefreshes,
         PreferRemoteRefreshes,
         PreferCallerRefreshes,
         PreferCalleeRefreshes
      } SessionTimerMode;
      virtual void setDefaultSessionTimerMode(Profile::SessionTimerMode mode);
      virtual Profile::SessionTimerMode getDefaultSessionTimerMode() const;
      virtual void unsetDefaultSessionTimerMode();   

      /// The amount of time that can pass before dum will resubmit an unreliable provisional response
      virtual void set1xxRetransmissionTime(int secs);
      virtual int get1xxRetransmissionTime() const;
      virtual void unset1xxRetransmissionTime();

      /// The amount of time that can pass before dum will resubmit a reliable provisional response
      virtual void set1xxRelResubmitTime(int secs);
      virtual int get1xxRelResubmitTime() const;
      virtual void unset1xxRelResubmitTime();

      ///overrides the value used to populate the contact
      ///?dcm? -- also change via entries? Also, dum currently uses(as a uas)
      ///the request uri of the dialog constructing request for the local contact
      ///within that dialog. A transport paramter here could also be used to
      ///force tcp vs udp vs tls?
      virtual void setOverrideHostAndPort(const Uri& hostPort);
      virtual bool hasOverrideHostAndPort() const;
      virtual const Uri& getOverrideHostAndPort() const;      
      virtual void unsetOverrideHostAndPort(); 
      
      ///enable/disable sending of Allow/Supported/Accept-Language/Accept-Encoding headers 
      ///on initial outbound requests (ie. Initial INVITE, REGISTER, etc.) and Invite 200 responses
      ///Note:  Default is to advertise Headers::Allow and Headers::Supported, use clearAdvertisedCapabilities to remove these
      ///       Currently implemented header values are: Headers::Allow, Headers::AllowEvents
      ///       Headers::AcceptEncoding, Headers::AcceptLanguage, Headers::Supported
      virtual void addAdvertisedCapability(const Headers::Type header);
      virtual bool isAdvertisedCapability(const Headers::Type header) const;
      virtual void clearAdvertisedCapabilities(); 
      virtual void unsetAdvertisedCapabilities();
      
      /// Use to route all outbound requests through a particular proxy
      virtual void setOutboundProxy( const Uri& uri );
      virtual const NameAddr& getOutboundProxy() const;
      virtual bool hasOutboundProxy() const;
      virtual void unsetOutboundProxy(); 

      ///If enabled, forces use of outbound proxy on all requests, including
      ///mid-dialog requests.  WARNING:  Using this setting breaks 3261 mid-dialog
      ///routing and disables any ability to react to target refreshes.  However
      ///there are certain scenarios/endpoints for which this setting may make 
      ///sense - for example: to communicate with an endpoint that never populates 
      ///it's Contact header correctly.
      virtual void setForceOutboundProxyOnAllRequestsEnabled(bool enabled) ;
      virtual bool getForceOutboundProxyOnAllRequestsEnabled() const;
      virtual void unsetForceOutboundProxyOnAllRequestsEnabled();

      ///If enabled, add a route header for the outbound proxy
      virtual void setExpressOutboundAsRouteSetEnabled(bool enabled) ;
      virtual bool getExpressOutboundAsRouteSetEnabled() const;
      virtual void unsetExpressOutboundAsRouteSetEnabled();

      ///enable/disable rport for requests. rport is enabled by default
      virtual void setRportEnabled(bool enabled);
      virtual bool getRportEnabled() const;      
      virtual void unsetRportEnabled(); 

      ///if set then UserAgent header is added to outbound messages
      virtual void setUserAgent( const Data& userAgent );
      virtual const Data& getUserAgent() const;
      virtual bool hasUserAgent() const;
      virtual void unsetUserAgent(); 

      ///if set then ProxyRequires header is added to outbound messages
      virtual void setProxyRequires( const Tokens& proxyRequires );
      virtual const Tokens& getProxyRequires() const;
      virtual bool hasProxyRequires() const;
      virtual void unsetProxyRequires(); 

      ///time between CR/LF keepalive messages in seconds.  Set to 0 to disable. 
      ///Default is 30 seconds for datagram and 180 seconds for stream.
      ///Note:  You must set a KeepAliveManager on DUM for this to work.
      virtual void setKeepAliveTimeForDatagram(int keepAliveTime);
      virtual int getKeepAliveTimeForDatagram() const;
      virtual void unsetKeepAliveTimeForDatagram();
      virtual void setKeepAliveTimeForStream(int keepAliveTime);
      virtual int getKeepAliveTimeForStream() const;
      virtual void unsetKeepAliveTimeForStream();

      ///If set dum will provide a port in the via for requests sent down to the stack.  This
      ///will tell the transport selector to only look at those transports using this port.
      ///Default is 0 (Disabled).
      ///WARNING:  Setting this can cause undesirable behaviour in the case when you want
      ///          DNS entries to decided your transport and you are supporting TLS.
      ///          For example: if you add UDP:5060, TCP:5060 and TLS:5061 and setFixedTransportPort 
      ///          to 5060 - then the TLS transport cannot be used.
      virtual void setFixedTransportPort(int fixedTransportPort);
      virtual int getFixedTransportPort() const;
      virtual void unsetFixedTransportPort(); 

      ///If set dum will provide a interface in the via for requests sent down to the stack.  This
      ///will tell the transport selector to only look at those transports using this interface.
      ///Default is Data::Empty (Disabled).
      virtual void setFixedTransportInterface(const Data& fixedTransportInterface);
      virtual const Data& getFixedTransportInterface() const;
      virtual void unsetFixedTransportInterface(); 

      ///If enabled then rinstance parameter is added to contacts.  The rinstance
      ///parameter is added by default.
      virtual void setRinstanceEnabled(bool enabled);
      virtual bool getRinstanceEnabled() const;
      virtual void unsetRinstanceEnabled();

      //If set then dum will add this MessageDecorator to all outbound messages
      virtual void setOutboundDecorator(SharedPtr<MessageDecorator> outboundDecorator);
      virtual SharedPtr<MessageDecorator> getOutboundDecorator();
      virtual void unsetOutboundDecorator();

      ///If enabled then methods parameter is added to contacts.
      virtual void setMethodsParamEnabled(bool enabled) ;
      virtual bool getMethodsParamEnabled() const;
      virtual void unsetMethodsParamEnabled();

      ///If set, the parameters on the provided NameAddr are used in the contact header
      ///Example: 
      ///  #include <resip/stack/ExtensionParameter.hxx>
      ///  static const resip::ExtensionParameter p_automaton("automaton");
      ///  static const resip::ExtensionParameter p_byeless("+sip.byeless");
      ///  static const resip::ExtensionParameter p_rendering("+sip.rendering");
      ///  ...
      ///  NameAddr capabilities;
      ///  capabilities.param(p_automaton); 
      ///  capabilities.param(p_byeless);
      ///  capabilities.param(p_rendering) = "\"no\"";
      ///  profile->setUserAgentCapabilities(capabilities);
      virtual void setUserAgentCapabilities(const NameAddr& capabilities) ;
      virtual bool hasUserAgentCapabilities() const;
      virtual const NameAddr& getUserAgentCapabilities() const;
      virtual void unsetUserAgentCapabilities();

      ///If enabled then dialog identifying headers are added to SipFrag bodies 
      ///that are generated in an InviteSession
      virtual void setExtraHeadersInReferNotifySipFragEnabled(bool enabled) ;
      virtual bool getExtraHeadersInReferNotifySipFragEnabled() const;
      virtual void unsetExtraHeadersInReferNotifySipFragEnabled();

   private:
      bool mHasDefaultRegistrationExpires;
      UInt32 mDefaultRegistrationExpires;
      
      bool mHasDefaultMaxRegistrationExpires;
      UInt32 mDefaultMaxRegistrationExpires;

      bool mHasDefaultRegistrationRetryInterval;
      int  mDefaultRegistrationRetryInterval;

      bool mHasDefaultSubscriptionExpires;
      UInt32 mDefaultSubscriptionExpires;

      bool mHasDefaultPublicationExpires;
      UInt32 mDefaultPublicationExpires;

      bool mHasDefaultStaleCallTime;
      int mDefaultStaleCallTime;

      bool mHasDefaultStaleReInviteTime;
      int mDefaultStaleReInviteTime;

      bool mHasDefaultSessionExpires;
      UInt32 mDefaultSessionExpires;

      bool mHasDefaultSessionTimerMode;
      SessionTimerMode mDefaultSessionTimerMode;

      bool mHas1xxRetransmissionTime;
      int m1xxRetransmissionTime;

      bool mHas1xxRelResubmitTime;
      int m1xxRelResubmitTime;

      bool mHasOutboundProxy;
      NameAddr mOutboundProxy;
      
      bool mHasForceOutboundProxyOnAllRequestsEnabled;
      bool mForceOutboundProxyOnAllRequestsEnabled;

      bool mHasExpressOutboundAsRouteSetEnabled;
      bool mExpressOutboundAsRouteSetEnabled;

      bool mHasAdvertisedCapabilities;
      std::set<Headers::Type> mAdvertisedCapabilities;
      
      bool mHasRportEnabled;
      bool mRportEnabled;
      
      bool mHasUserAgent;            
      Data mUserAgent;
      
      bool mHasOverrideHostPort;
      Uri  mOverrideHostPort;
      
      bool mHasKeepAliveTimeForDatagram;
      int  mKeepAliveTimeForDatagram;

      bool mHasKeepAliveTimeForStream;
      int  mKeepAliveTimeForStream;

      bool mHasFixedTransportPort;
      int  mFixedTransportPort;
      
      bool mHasFixedTransportInterface;
      Data mFixedTransportInterface;

      bool mHasProxyRequires;
      Tokens mProxyRequires;

      bool mHasRinstanceEnabled;
      bool mRinstanceEnabled;
      
      bool mHasOutboundDecorator;
      SharedPtr<MessageDecorator> mOutboundDecorator;
      
      bool mHasMethodsParamEnabled;
      bool mMethodsParamEnabled;

      bool mHasUserAgentCapabilities;
      NameAddr mUserAgentCapabilities;

      bool mHasExtraHeadersInReferNotifySipFragEnabled;
      bool mExtraHeadersInReferNotifySipFragEnabled;

      SharedPtr<Profile> mBaseProfile;  // All non-set settings will fall through to this Profile (if set)
};

}

#endif

/* ====================================================================
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
