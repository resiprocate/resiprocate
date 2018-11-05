#include "apps/reConServer/AppSubsystem.hxx"
#include "apps/reConServer/CredentialGrabber.hxx"
#include "apps/reConServer/CredentialInfo.hxx"
#include "rutil/Data.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::RECONSERVER

using namespace reconserver;
using namespace resip;
using namespace soci;

CredentialGrabber::CredentialGrabber(SharedPtr<soci::connection_pool> pool, const resip::Data& databaseQueryUserCredential) :
   mPool(pool),
   mDatabaseQueryUserCredential(databaseQueryUserCredential)
{
   DebugLog(<<"instantiated");
}

CredentialGrabber::~CredentialGrabber()
{
}

bool
CredentialGrabber::process(resip::ApplicationMessage* msg)
{
   DebugLog(<<"invoked");
   CredentialInfo* ci = dynamic_cast<CredentialInfo*>(msg);
   if (ci)
   {
      session db(*mPool);

      std::string secret;
      std::string user(ci->user().c_str());
      std::string realm(ci->realm().c_str());
      DebugLog(<< "looking up " << user << "@" << realm);

      soci::indicator ind;
      int retries = 2;
      while(retries-- > 0)
      {
         try
         {
            db << mDatabaseQueryUserCredential, soci::into(secret, ind), soci::use(user), soci::use(realm);
            retries = 0;
         }
         catch (soci::soci_error const & e)
         {
            ErrLog(<<"SOCI error: " << e.what());
            if(retries > 0)
            {
               db.reconnect();
            }
            else
            {
               WarningLog(<<"reconnect failed");
               ci->mode() = CredentialInfo::Error;
               return true;
            }
         }
      }

      if(!db.got_data())
      {
         WarningLog(<<"no credential found for " << user << "@" << realm);
         ci->mode() = CredentialInfo::UserUnknown;
      }
      if(ind != soci::i_ok)
      {
         WarningLog(<<"credential is NULL or something else is wrong (ind != soci::i_ok)");
         ci->mode() = CredentialInfo::Error;
      }

      if(!secret.empty())
      {
         DebugLog(<<"found credential for authenticating " << user << " in realm " << realm);
         ci->secret() = Data(secret.c_str());
         ci->mode() = CredentialInfo::RetrievedCredential;
      }
      else
      {
         DebugLog(<<"didn't find individual credential for authenticating " << user << "@" << realm);
         ci->mode() = CredentialInfo::Error;
      }

      DebugLog(<< "Returning result for " << user << "@" << realm << " : " << ci->secret());
      return true;
   }

   WarningLog(<< "Did not recognize message type...");
   return false;
}

CredentialGrabber*
CredentialGrabber::clone() const
{
   return new CredentialGrabber(mPool, mDatabaseQueryUserCredential);
}

/* ====================================================================
 *
 * Copyright 2017 Daniel Pocock http://danielpocock.com  All rights reserved.
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
 *
 */

