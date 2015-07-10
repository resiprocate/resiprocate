#include "rutil/ResipAssert.h"

#include "resip/dum/DialogUsageManager.hxx"
#include "repro/ReproServerAuthManager.hxx"
#include "resip/dum/ServerAuthManager.hxx"
#include "resip/dum/UserAuthInfo.hxx"
#include "repro/UserStore.hxx"
#include "repro/AclStore.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace resip;
using namespace repro;


ReproServerAuthManager::ReproServerAuthManager(DialogUsageManager& dum,
                                               Dispatcher* authRequestDispatcher,
                                               AclStore& aclDb,
                                               bool useAuthInt,
                                               bool rejectBadNonces,
                                               bool challengeThirdParties,
                                               const Data& staticRealm):
   ServerAuthManager(dum, dum.dumIncomingTarget(), challengeThirdParties, staticRealm),
   mDum(dum),
   mAuthRequestDispatcher(authRequestDispatcher),
   mAclDb(aclDb),
   mUseAuthInt(useAuthInt),
   mRejectBadNonces(rejectBadNonces)
{
}

ReproServerAuthManager::~ReproServerAuthManager()
{
}

bool 
ReproServerAuthManager::useAuthInt() const
{
   return mUseAuthInt;
}

bool
ReproServerAuthManager::rejectBadNonces() const
{
   return mRejectBadNonces;
}

ServerAuthManager::AsyncBool
ReproServerAuthManager::requiresChallenge(const SipMessage& msg)
{
   resip_assert(msg.isRequest());
   if(!mAclDb.isRequestTrusted(msg))
   {
      return ServerAuthManager::requiresChallenge(msg);
   }
   else
   {
      return False;
   }
}

void 
ReproServerAuthManager::requestCredential(const Data& user, 
                                          const Data& realm, 
                                          const SipMessage& msg,
                                          const Auth& auth,
                                          const Data& transactionId )
{
   // Build a UserAuthInfo object and pass to UserAuthGrabber to have a1 password filled in
   UserAuthInfo* async = new UserAuthInfo(user,realm,transactionId,&mDum);
   std::auto_ptr<ApplicationMessage> app(async);
   mAuthRequestDispatcher->post(app);
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
