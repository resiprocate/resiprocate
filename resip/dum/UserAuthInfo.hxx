#if !defined(RESIP_USER_AUTH_INFO_HXX)
#define RESIP_USER_AUTH_INFO_HXX 

#include "rutil/Data.hxx"
#include "resip/dum/DumFeatureMessage.hxx"

namespace resip
{

class UserAuthInfo : public resip::DumFeatureMessage
{
   public:

      enum InfoMode 
      {
        UserUnknown,       // the user/realm is not known
        RetrievedA1,       // the A1 string has been retrieved
        Stale,             // the nonce is stale, challenge again
        DigestAccepted,    // the digest was accepted, no A1 returned
        DigestNotAccepted, // the digest was wrong, challenge again/deny
        Error              // some error occurred
      };

      UserAuthInfo( const resip::Data& user,
                    const resip::Data& realm,
                    InfoMode mode,
                    const resip::Data& transactionId);

      UserAuthInfo( const resip::Data& user,
                    const resip::Data& realm,
                    const resip::Data& a1,
                    const resip::Data& transactionId);

      UserAuthInfo( const resip::Data& user,
                    const resip::Data& realm,
                    const resip::Data& transactionId,
                    resip::TransactionUser* transactionUser);
      ~UserAuthInfo();
         
      InfoMode getMode() const;

      // returns a blank Data("") if (mode != RetrievedA1)
      const resip::Data& getA1() const;
      const resip::Data& getRealm() const;
      const resip::Data& getUser() const;

      void setMode(InfoMode mode);
      void setA1(const resip::Data& a1);

      virtual resip::Data brief() const;
      virtual resip::Message* clone() const;

      virtual EncodeStream& encode(EncodeStream& strm) const;
      virtual EncodeStream& encodeBrief(EncodeStream& strm) const;

   private:
      InfoMode mMode;
      resip::Data mUser;
      resip::Data mRealm;
      resip::Data mA1;
};

EncodeStream& 
operator<<(EncodeStream& strm, const UserAuthInfo& msg);

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
