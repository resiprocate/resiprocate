#if !defined(RESIP_USERPROFILE_HXX)
#define RESIP_USERPROFILE_HXX

#include <iosfwd>
#include <set>
#include "resiprocate/Headers.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/Win32ExportDum.hxx"

namespace resip
{

class Data;

class DUM_API UserProfile : public Profile
{
   public:  
      UserProfile(Profile *baseProfile = 0);
      
      virtual void setDefaultFrom(const NameAddr& from);
      virtual const NameAddr& getDefaultFrom() const;

      // !cj! - this GRUU stuff looks very suspect
      virtual void addGruu(const Data& aor, const NameAddr& contact);
      virtual bool hasGruu(const Data& aor) const;
      virtual bool hasGruu(const Data& aor, const Data& instance) const;
      virtual NameAddr& getGruu(const Data& aor);
      virtual NameAddr& getGruu(const Data& aor, const NameAddr& contact);
      virtual void disableGruu();

      virtual void setInstanceId(const Data& id);
      virtual const Data& getInstanceId() const;
      
      struct DigestCredential
      {
            DigestCredential(); 
            DigestCredential(const Data& realm, 
                             const Data& username, 
                             const Data& pwd);
            DigestCredential(const Data& realm);
                             
            Data realm;
            Data user;
//            Data passwordHashA1;
            Data password;            

            bool operator<(const DigestCredential& rhs) const;
      };
      
      /// The following functions deal with clearing, setting and getting of digest credentals 
      virtual void clearDigestCredentials();
      virtual void setDigestCredential( const Data& realm, 
                                        const Data& user, 
                                        const Data& password);
      virtual const DigestCredential& getDigestCredential( const Data& realm  );

   private:
      NameAddr mDefaultFrom;
      Data mInstanceId;
      
      typedef std::set<DigestCredential> DigestCredentials;
      DigestCredentials mDigestCredentials;

      friend std::ostream& operator<<(std::ostream&, const UserProfile& profile);
};
  
std::ostream& 
operator<<(std::ostream&, const UserProfile& profile);

std::ostream& 
operator<<(std::ostream&, const UserProfile::DigestCredential& cred);
 
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
