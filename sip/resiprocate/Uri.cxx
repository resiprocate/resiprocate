#include <sipstack/Uri.hxx>

using namespace Vocal2;

Uri::Uri(const Uri& rhs)
   : ParserCategory(rhs),
     mScheme(rhs.mScheme),
     mHost(rhs.mHost),
     mUser(rhs.mUser),
     mAor(rhs.mAor),
     mPort(rhs.mPort),
     mPassword(rhs.mPassword)
{
}

const Data&
Uri::getAor() const
{
   // could keep last mUser, mHost, mPort around and compare
   // rather than always reallocate...
   mAor = mUser + Symbols::AT_SIGN + mHost + Symbols::COLON + Data(mPort);
   return mAor;
}

void
Uri::parse()
{
   assert(0);
}

ParserCategory*
Uri::clone() const
{
   return new Uri(*this);
}
 
std::ostream& 
Uri::encode(std::ostream& str) const
{
   assert(0);
   return str;
}

void
Uri::parseEmbeddedHeaders()
{
   assert(0);
}
