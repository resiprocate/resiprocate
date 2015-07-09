#if !defined(RESIP_USERPROFILE_HXX)
#define RESIP_USERPROFILE_HXX

#include <iosfwd>
#include <set>
#include "resip/stack/Headers.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/dum/Profile.hxx"

namespace resip
{

class Data;

class UserProfile : public Profile
{
   public:  
      UserProfile();
      UserProfile(SharedPtr<Profile> baseProfile);
      virtual ~UserProfile();
      
      virtual void setDefaultFrom(const NameAddr& from);
      virtual NameAddr& getDefaultFrom();

      virtual void setServiceRoute(const NameAddrs& sRoute);
      virtual NameAddrs& getServiceRoute();
      
      virtual void setImsAuthUser(const Data& userName, const Data& host) { mImsAuthUserName = userName; mImsAuthHost = host; }      
      virtual Data& getImsAuthUserName() { return mImsAuthUserName; }      
      virtual Data& getImsAuthHost() { return mImsAuthHost; }

      // Returns a UserProfile that will return a UserProfile that can be used
      // to send requests according to RFC 3323 and RFC 3325
      virtual SharedPtr<UserProfile> getAnonymousUserProfile() const;      
      bool isAnonymous() const;
      
      // !cj! - this GRUU stuff looks very suspect
      // !dcm! -- yep, I don't think you can adda gruu..and disabling is weird.
      //Anon should be on a per-call level...all of these will prob. go away.
      //.dcm. anonymous gruu's will be used by making annon userprofile where
      //one is avail. Up to the ap to keep the req'd reg up.
      //1 gruu per userprofile for now.
      virtual void addGruu(const Data& aor, const NameAddr& contact);
      virtual bool hasGruu(const Data& aor) const;
      virtual bool hasGruu(const Data& aor, const Data& instance) const;
      virtual NameAddr& getGruu(const Data& aor);
      virtual NameAddr& getGruu(const Data& aor, const NameAddr& contact);

      //should do Supported wrangling--but what if required is desired? Same as 100rel?
      virtual bool& gruuEnabled() { return mGruuEnabled; }
      virtual bool gruuEnabled() const { return mGruuEnabled; }

      virtual bool hasPublicGruu() const { return !mPubGruu.host().empty(); }
      virtual const Uri& getPublicGruu() { return mPubGruu; }
      virtual void setPublicGruu(const Uri& gruu) { mPubGruu = gruu; }

      virtual bool hasTempGruu() const { return !mTempGruu.host().empty(); }
      virtual const Uri& getTempGruu() { return mTempGruu; }
      virtual void setTempGruu(const Uri& gruu) { mTempGruu = gruu; }
      
      struct DigestCredential
      {
            DigestCredential(); 
            DigestCredential(const Data& realm, 
                             const Data& username, 
                             const Data& pwd,
                             bool isPasswordA1Hash);
            DigestCredential(const Data& realm);
                             
            Data realm;
            Data user;
            Data password;            
            bool isPasswordA1Hash;

            bool operator<(const DigestCredential& rhs) const;
      };
      
      /// The following functions deal with clearing, setting and getting of digest credentals 
      virtual void clearDigestCredentials();
      /// For the password you may either provide the plain text password (isPasswordA1Hash = false)
      /// or the Digest A1 MD5 Hash (isPasswordA1Hash = true).  Note:  If the A1 hash is provided
      /// then the realm MUST match the realm in the challenge or authentication will fail.  If the
      /// plain text password is provided, then we will form the A1 hash using the realm from
      /// the challenge.
      virtual void setDigestCredential( const Data& realm, 
                                        const Data& user, 
                                        const Data& password,
                                        bool isPasswordA1Hash=false);
      virtual const DigestCredential& getDigestCredential( const Data& realm  );
      // DigestCacheUseLimit is used to indicate the maximum number of times a particular Proxy or WWW Authorization
      // header will be used in requests within a dialogset.  When this limit is reached then the
      // next request in the DiaglogSet will go out without digest credentials.  This setting can be used to
      // work around bugs/limitations in third-party implementations that have difficulty properly dealing with
      // cached credentials.  A setting of 0 (default) will disable the limit and all requests in a Dialogset will 
      // have the same cached Authorization header on them, until they are re-challenged by the far end.  A setting of 
      // 1 disables caching entirely and future requests within the dialog set will go out without any authorization 
      // headers.
      virtual void setDigestCacheUseLimit(unsigned long digestCacheUseLimit) { mDigestCacheUseLimit = digestCacheUseLimit; }
      virtual unsigned long getDigestCacheUseLimit() { return mDigestCacheUseLimit; }

      // Enable this to enable RFC5626 support in DUM - adds regId to registrations, and 
      // ;ob parameter to Path, Route, and Contact headers
      // Warning:  You MUST set an instanceId, a regId and an outbound proxy if you enable 
      // clientOutbound support.  You MUST also ensure that you add the following Supported
      // options:
      // profile->addSupportedOptionTag(Token(Symbols::Outbound));  // RFC 5626 - outbound
      // profile->addSupportedOptionTag(Token(Symbols::Path));      // RFC 3327 - path
      virtual bool& clientOutboundEnabled() { return mClientOutboundEnabled; }
      virtual bool clientOutboundEnabled() const { return mClientOutboundEnabled; }

      // Outbound (RFC5626) instanceId used in contact headers
      virtual bool hasInstanceId();
      virtual void setInstanceId(const Data& id);
      virtual const Data& getInstanceId() const;

      // Outbound (RFC5626) regId used in registrations
      virtual void setRegId(int regId) { mRegId = regId; }
      virtual int getRegId() { return mRegId; }

      // Returns the current Flow Tuple that is being used for communication on usages
      // that use this profile
      const Tuple& getClientOutboundFlowTuple() const { return mClientOutboundFlowTuple; }
      void clearClientOutboundFlowTuple() { mClientOutboundFlowTuple = Tuple(); }
      void setClientOutboundFlowTuple(const Tuple& outboundFlowTuple) { mClientOutboundFlowTuple = outboundFlowTuple; }  // Only for advanced users

   protected:
      virtual UserProfile* clone() const;

   private:
      NameAddr mDefaultFrom;
      Data mInstanceId;
      NameAddrs mServiceRoute;
      Data mImsAuthUserName;
      Data mImsAuthHost;
      bool mGruuEnabled;
      Uri mPubGruu;
      Uri mTempGruu;
      const static NameAddr mAnonymous;

      int mRegId;
      bool mClientOutboundEnabled;
      friend class DialogUsageManager;  // Give DialogUsageManager, ClientRegistration, and Dialog access to mClientOutboundFlowKey
      friend class ClientRegistration;
      friend class Dialog;
      Tuple mClientOutboundFlowTuple;
      
      typedef std::set<DigestCredential> DigestCredentials;
      DigestCredentials mDigestCredentials;
      unsigned long mDigestCacheUseLimit;

      friend EncodeStream& operator<<(EncodeStream&, const UserProfile& profile);
};
  
EncodeStream& 
operator<<(EncodeStream&, const UserProfile& profile);

EncodeStream& 
operator<<(EncodeStream&, const UserProfile::DigestCredential& cred);
 
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
