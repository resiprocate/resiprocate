
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/UserProfile.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/SipMessage.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


UserProfile::UserProfile(Profile *baseProfile) : Profile(baseProfile)
{
}

void
UserProfile::setDefaultFrom(const NameAddr& from)
{
   mDefaultFrom = from;
}

NameAddr& 
UserProfile::getDefaultFrom()
{
   return mDefaultFrom;
}

void
UserProfile::setInstanceId(const Data& id)
{
   mInstanceId = id;
}

const Data&
UserProfile::getInstanceId() const
{
   return mInstanceId;
}

void 
UserProfile::addGruu(const Data& aor, const NameAddr& contact)
{
}

bool 
UserProfile::hasGruu(const Data& aor) const
{
   return false;
}

bool 
UserProfile::hasGruu(const Data& aor, const Data& instance) const
{
   return false;
}

NameAddr&
UserProfile:: getGruu(const Data& aor)
{
   assert(0);
   static NameAddr gruu;
   return gruu;
}

NameAddr&
UserProfile:: getGruu(const Data& aor, const NameAddr& contact)
{
   assert(0);
   static NameAddr gruu;
   return gruu;
}

void 
UserProfile::disableGruu()
{
   assert(0);
}

void 
UserProfile::setDigestCredential( const Data& aor, const Data& realm, const Data& user, const Data& password)
{
   DigestCredential cred(aor, realm, user, password);
   DebugLog (<< "Adding credential: " << cred);
   mDigestCredentials.erase(cred);
   mDigestCredentials.insert(cred);
}
     
const UserProfile::DigestCredential&
UserProfile::getDigestCredential( const Data& realm )
{
   DigestCredential dc;
   dc.realm = realm;
   
   DigestCredentials::const_iterator i = mDigestCredentials.find(dc);
   if (i != mDigestCredentials.end())
   {
      return *i;
   }
   
   static const DigestCredential empty;
   return empty;
}

const UserProfile::DigestCredential&
UserProfile::getDigestCredential( const SipMessage& challenge )
{
   StackLog (<< Inserter(mDigestCredentials));
   DebugLog (<< "Using From header: " <<  challenge.header(h_From).uri().getAor() << " to find credential");   
   const Data& aor = challenge.header(h_From).uri().getAor();
   for (DigestCredentials::const_iterator it = mDigestCredentials.begin(); 
        it != mDigestCredentials.end(); it++)
   {
      if (it->aor == aor)
      {
         return *it;
      }
   }
   const Data& user = challenge.header(h_From).uri().user();
   for (DigestCredentials::const_iterator it = mDigestCredentials.begin(); 
        it != mDigestCredentials.end(); it++)
   {
      if (it->user == user)
      {
         return *it;
      }
   }

   // !jf! why not just throw here? 
   static const DigestCredential empty;
   return empty;
}

UserProfile::DigestCredential::DigestCredential(const Data& a, const Data& r, const Data& u, const Data& p) :
   aor(a),
   realm(r),
   user(u),
   password(p)
{
}

UserProfile::DigestCredential::DigestCredential() : 
   aor(Data::Empty),
   realm(Data::Empty),
   user(Data::Empty),
   password(Data::Empty)
{
}  

bool
UserProfile::DigestCredential::operator<(const DigestCredential& rhs) const
{
   if (realm < rhs.realm)
   {
      return true;
   }
   else if (realm == rhs.realm)
   {
      return aor < rhs.aor;
   }
   else
   {
      return false;
   }
}

std::ostream&
resip::operator<<(std::ostream& strm, const UserProfile::DigestCredential& cred)
{
   strm << "realm=" << cred.realm 
        << " aor=" << cred.aor
        << " user=" << cred.user ;
   return strm;
}

