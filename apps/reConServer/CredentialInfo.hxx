#if !defined(CredentialInfo_hxx)
#define CredentialInfo_hxx

#include <iostream>
#include "apps/reConServer/AppSubsystem.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Message.hxx"
#include "resip/dum/DumCommand.hxx"
#include "resip/dum/DumFeatureMessage.hxx"

#include <memory>

namespace reconserver
{

class B2BCall;
class B2BCallManager;

class CredentialInfo : public resip::DumCommand
{
   public:
      enum InfoMode
      {
        UserUnknown,       // the user/realm is not known
        RetrievedCredential, // the secret has been retrieved
        Error              // some error occurred
      };

      CredentialInfo(std::shared_ptr<B2BCall> call, const resip::Data& user, const resip::Data& realm, const resip::Data& transactionId, resip::TransactionUser* transactionUser, B2BCallManager* b);

      std::shared_ptr<B2BCall> call() const noexcept { return mCall; }

      const resip::Data& user() const noexcept { return mUser; }
      resip::Data& user() noexcept { return mUser; }

      const resip::Data& realm() const noexcept { return mRealm; }
      resip::Data& realm() noexcept { return mRealm; }

      const resip::Data& secret() const noexcept { return mSecret; }
      resip::Data& secret() noexcept { return mSecret; }

      const InfoMode mode() const noexcept { return mMode; }
      InfoMode& mode() noexcept { return mMode; }

      virtual void executeCommand();

      virtual resip::Message* clone() const;

      virtual EncodeStream& encode(EncodeStream& strm) const;
      virtual EncodeStream& encodeBrief(EncodeStream& strm) const;

   private:
      std::shared_ptr<B2BCall> mCall;
      resip::Data mUser;
      resip::Data mRealm;
      resip::Data mSecret;
      InfoMode mMode;

      B2BCallManager* mB2BCallManager;

};

EncodeStream&
operator<<(EncodeStream& strm, const CredentialInfo& msg);

}

#endif

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

