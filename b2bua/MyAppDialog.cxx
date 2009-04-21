
#include "MyAppDialog.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

MyAppDialog::MyAppDialog(HandleManager& ham) : AppDialog(ham) {
  setB2BCall(NULL);
}

MyAppDialog::MyAppDialog(HandleManager& ham, B2BCall *b2BCall) : AppDialog(ham) {
  setB2BCall(b2BCall);
  // FIXME - is this the B Leg?
  if(b2BCall != NULL)
    b2BCall->setBLegAppDialog(this);
}

MyAppDialog::~MyAppDialog() {
  if(b2BCall != NULL)
    b2BCall->releaseAppDialog(this);
}

B2BCall *MyAppDialog::getB2BCall() {
  return b2BCall;
}

void MyAppDialog::setB2BCall(B2BCall *b2BCall) {
  this->b2BCall = b2BCall;
}

MyAppDialogSet::MyAppDialogSet(DialogUsageManager& dum) : AppDialogSet(dum) {
  this->b2BCall = NULL;
  //userProfile = NULL;
}

MyAppDialogSet::MyAppDialogSet(DialogUsageManager& dum, B2BCall *b2BCall, SharedPtr<UserProfile>& userProfile) : AppDialogSet(dum) {
  this->b2BCall = b2BCall;
  this->userProfile = userProfile;
}

MyAppDialogSet::~MyAppDialogSet() {
  if(b2BCall != NULL)
    b2BCall->releaseAppDialogSet(this);
}

AppDialog* MyAppDialogSet::createAppDialog(const SipMessage& msg) {
  return new MyAppDialog(mDum, b2BCall);
}

/* SharedPtr<UserProfile> MyAppDialogSet::getUserProfile() {
  return userProfile;
} */

SharedPtr<UserProfile> MyAppDialogSet::selectUASUserProfile(const SipMessage& msg) {
  //return getUserProfile();
  return mDum.getMasterUserProfile();
}

AppDialogSet* MyAppDialogSetFactory::createAppDialogSet(DialogUsageManager& dum, const SipMessage& msg) {
  return new MyAppDialogSet(dum);
}

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

