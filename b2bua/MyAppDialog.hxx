
#ifndef __MyAppDialog_h
#define __MyAppDialog_h

#include "resip/stack/SipMessage.hxx"

#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/dum/DialogUsageManager.hxx"

namespace b2bua
{

class MyAppDialog;
class MyAppDialogSet;
class MyDialogSetHandler;

}

#include "B2BCall.hxx"

namespace b2bua
{

class MyAppDialog : public resip::AppDialog {

protected:
  B2BCall *b2BCall;
  
public:

  MyAppDialog(resip::HandleManager& ham);
  MyAppDialog(resip::HandleManager& ham, B2BCall *b2BCall);
  virtual ~MyAppDialog();
  B2BCall *getB2BCall();
  void setB2BCall(B2BCall *b2BCall); 

};

class MyAppDialogSet : public resip::AppDialogSet {

protected:
  B2BCall *b2BCall;
  resip::SharedPtr<resip::UserProfile> userProfile;

public:
  MyAppDialogSet(resip::DialogUsageManager& dum);
  MyAppDialogSet(resip::DialogUsageManager& dum, B2BCall *b2BCall, resip::SharedPtr<resip::UserProfile>& userProfile);
  virtual ~MyAppDialogSet();
  virtual resip::AppDialog* createAppDialog(const resip::SipMessage& msg);
  // virtual resip::SharedPtr<resip::UserProfile> getUserProfile();
  virtual resip::SharedPtr<resip::UserProfile> selectUASUserProfile(const resip::SipMessage& msg);
  void setB2BCall(B2BCall *b2BCall)
    { this->b2BCall = b2BCall; };

  friend class MyDialogSetHandler;
};

class MyAppDialogSetFactory : public resip::AppDialogSetFactory {

public:
  virtual resip::AppDialogSet* createAppDialogSet(resip::DialogUsageManager& dum, const resip::SipMessage& msg);

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

