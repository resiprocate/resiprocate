#if !defined(RESIP_USERPROFILE_HXX)
#define RESIP_USERPROFILE_HXX

#include <iosfwd>
#include <set>
#include "resiprocate/Headers.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/dum/Profile.hxx"

namespace resip
{

class Data;

class UserProfile : public Profile
{
   public:  
      UserProfile(Profile *baseProfile = 0);
      
      virtual void setDefaultFrom(const NameAddr& from);
      virtual NameAddr& getDefaultFrom();

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
            DigestCredential(const Data& aor, const Data& realm, const Data& username, const Data& password);
            Data aor;
            Data realm;
            Data user;
            Data password;

            bool operator<(const DigestCredential& rhs) const;
      };
      
      /// The following functions deal with clearing, setting and getting of digest credentals 
      virtual void  clearDigestCredentials();
      virtual void  setDigestCredential( const Data& aor, const Data& realm, const Data& user, const Data& password);
      virtual const DigestCredential& getDigestCredential( const Data& realm );
      virtual const DigestCredential& getDigestCredential( const SipMessage& challenge );      

   private:
      NameAddr mDefaultFrom;
      Data mInstanceId;
      
      typedef std::set<DigestCredential> DigestCredentials;
      DigestCredentials mDigestCredentials;
};
  
std::ostream& 
operator<<(std::ostream&, const UserProfile::DigestCredential& cred);
 
}

#endif
