#include "rutil/ResipAssert.h"

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "resip/dum/DialogUsageManager.hxx"
#include "repro/ReproRADIUSServerAuthManager.hxx"
#include "resip/dum/ServerAuthManager.hxx"
#include "resip/dum/UserAuthInfo.hxx"
#include "repro/UserStore.hxx"
#include "repro/AclStore.hxx"

#ifdef USE_RADIUS_CLIENT

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

using namespace resip;
using namespace repro;


ReproRADIUSServerAuthManager::ReproRADIUSServerAuthManager(DialogUsageManager& dum,
                                               AclStore& aclDb,
                                               bool useAuthInt,
                                               bool rejectBadNonces,
                                               const resip::Data& configurationFile,
                                               bool challengeThirdParties,
                                               const Data& staticRealm):
   RADIUSServerAuthManager(dum, dum.dumIncomingTarget(), configurationFile, challengeThirdParties, staticRealm),
   mDum(dum),
   mAclDb(aclDb),
   mUseAuthInt(useAuthInt),
   mRejectBadNonces(rejectBadNonces)
{
}

ReproRADIUSServerAuthManager::~ReproRADIUSServerAuthManager()
{
}

bool 
ReproRADIUSServerAuthManager::useAuthInt() const
{
   return mUseAuthInt;
}

bool
ReproRADIUSServerAuthManager::rejectBadNonces() const
{
   return mRejectBadNonces;
}

ServerAuthManager::AsyncBool
ReproRADIUSServerAuthManager::requiresChallenge(const SipMessage& msg)
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

#endif

/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock http://danielpocock.com
 * All rights reserved.
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

