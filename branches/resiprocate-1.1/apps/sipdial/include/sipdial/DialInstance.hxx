
#ifndef __DIALINSTANCE_H
#define __DIALINSTANCE_H

#include "resip/dum/DialogUsageManager.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"

#include "DialerConfiguration.hxx"

class MyInviteSessionHandler;

class DialInstance {

public:
   DialInstance(const DialerConfiguration& dialerConfiguration, const resip::Uri& targetUri);
   
   typedef enum 
   {
      ReferSuccessful,
      ReferUnsuccessful,
      Error
   } DialResult;
   DialResult execute();

protected:

   void prepareAddress();
   void sendInvite();
   resip::Data processNumber(const resip::Data& verboseNumber);

   // Receive notifications from MyInviteSessionHandler

   // some kind of failure message (4xx, 5xx, 6xx) received
   void onFailure();
   // the session has connected and is ready for a REFER
   void onConnected(resip::ClientInviteSessionHandle cis);
   // the REFER succeeded
   void onReferSuccess();
   // the REFER failed
   void onReferFailed();
   // the session has been terminated
   void onTerminated();

private:
   // Copy of values supplied when instance created
   DialerConfiguration mDialerConfiguration;
   resip::Uri mTargetUri;
   // The target URI, converted to a sip: URI if necessary
   resip::Uri mFullTarget;

   resip::SipStack *mSipStack;
   resip::DialogUsageManager *mDum;
   
   //MyInviteSessionHandler *mInviteSessionHandler;

   resip::ClientInviteSessionHandle mClient;

   // MyInviteSessionHandler will notify us of progress
   friend class MyInviteSessionHandler;

   typedef enum 
   {
      Dialing,
      Connected,
      ReferSent,
      Done
   } DialProgress;
   DialProgress mProgress;

   DialResult mResult;

};

#endif

