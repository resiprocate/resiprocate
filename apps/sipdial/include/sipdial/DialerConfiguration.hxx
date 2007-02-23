
#ifndef __DIALERCONFIGURATION_H
#define __DIALERCONFIGURATION_H

#include <iostream>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"

class DialerConfiguration {

public:
   DialerConfiguration();
   virtual ~DialerConfiguration();

   void loadStream(std::istream& in);

   void setDialerIdentity(const resip::NameAddr& dialerIdentity)
      { mDialerIdentity = dialerIdentity; };
   const resip::NameAddr& getDialerIdentity() 
      { return mDialerIdentity; };
   void setAuthRealm(const resip::Data& authRealm)
      { mAuthRealm = authRealm; };
   const resip::Data& getAuthRealm()
      { return mAuthRealm; };
   void setAuthUser(const resip::Data& authUser) 
      { mAuthUser = authUser; };
   const resip::Data& getAuthUser() 
      { return mAuthUser; };
   void setAuthPassword(const resip::Data& authPassword)
      { mAuthPassword = authPassword; };
   const resip::Data& getAuthPassword()
      { return mAuthPassword; };
   void setCallerUserAgentAddress(const resip::Uri& callerUserAgentAddress)
      { mCallerUserAgentAddress = callerUserAgentAddress; };
   const resip::Uri& getCallerUserAgentAddress()
      { return mCallerUserAgentAddress; };
   typedef enum
   {
      Generic,
      LinksysSPA941,
      PolycomIP501,
      Cisco7940
   } UserAgentVariety;
   void setCallerUserAgentVariety(UserAgentVariety callerUserAgentVariety)
      { mCallerUserAgentVariety = callerUserAgentVariety; };
   const UserAgentVariety getCallerUserAgentVariety()
      { return mCallerUserAgentVariety; };
   void setTargetPrefix(const resip::Data& targetPrefix)
      { mTargetPrefix = targetPrefix; };
   const resip::Data& getTargetPrefix() 
      { return mTargetPrefix; };
   void setTargetDomain(const resip::Data& targetDomain)
      { mTargetDomain = targetDomain; };
   const resip::Data& getTargetDomain() 
      { return mTargetDomain; };

protected:

   // Used for the `From' field of the INVITE
   resip::NameAddr mDialerIdentity;
   // Credentials we must send if challenged in the given realm
   // Todo: allow a hashmap of credentials for multiple realms
   resip::Data mAuthRealm;
   resip::Data mAuthUser;
   resip::Data mAuthPassword;
   // The SIP URI of the device which is being forced to make a call
   resip::Uri mCallerUserAgentAddress;
   // the type of UA which we are sending a REFER to, depending on the
   // type of device, we may need to force auto-answer with a specific
   // value of the Alert-Info header, put specific SDP in the INVITE,
   // and construct the REFER in a particular way
   UserAgentVariety mCallerUserAgentVariety;

   // The following parameters determine the way the REFER will be addressed

   // If the target URI is a tel: URI and it begins with a +, which 
   // signifies a full international number in E.164 format, 
   // then the + will be replaced with the value of mTargetPrefix
   resip::Data mTargetPrefix;
   // For any tel: URI, the targetDomain will be appended to the number, to
   // construct a SIP URI
   resip::Data mTargetDomain;


};


#endif

