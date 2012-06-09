
#ifndef __DIALINSTANCE_H
#define __DIALINSTANCE_H

#include <time.h>

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

   time_t mReferSentTime;

   DialResult mResult;

};

#endif


/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

