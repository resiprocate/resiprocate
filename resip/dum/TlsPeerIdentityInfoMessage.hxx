#if !defined(RESIP_TLSPEERIDENTITYINFOMESSAGE_HXX)
#define RESIP_TLSPEERIDENTITYINFOMESSAGE_HXX

#include <set>

#include "rutil/Data.hxx"
#include "resip/dum/DumFeatureMessage.hxx"

namespace resip
{

class TlsPeerIdentityInfoMessage : public resip::DumFeatureMessage
{
   public:
      TlsPeerIdentityInfoMessage(const Data& transactionId,
                     resip::TransactionUser* transactionUser);
      ~TlsPeerIdentityInfoMessage();

      const std::set<resip::Data>& peerNames() const{return mPeerNames;}
      std::set<resip::Data>& peerNames(){return mPeerNames;}

      const std::set<resip::Data>& identities() const{return mIdentities;}
      std::set<resip::Data>& identities(){return mIdentities;}

      const bool authorized() const{return mAuthorized;}
      bool& authorized(){return mAuthorized;}

      virtual resip::Data brief() const;
      virtual resip::Message* clone() const;

      virtual std::ostream& encode(std::ostream& strm) const;
      virtual std::ostream& encodeBrief(std::ostream& strm) const;

   private:
      std::set<resip::Data> mPeerNames;
      std::set<resip::Data> mIdentities;
      bool mAuthorized;
};

std::ostream&
operator<<(std::ostream& strm, const TlsPeerIdentityInfoMessage& msg);

}

#endif

/* ====================================================================
 *
 * Copyright 2014 Daniel Pocock http://danielpocock.com  All rights reserved.
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

