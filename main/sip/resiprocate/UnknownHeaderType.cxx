#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "UnknownHeaderType.hxx"
#include "HeaderTypes.hxx"

#include <cassert>
#include <string.h>
#include "resiprocate/os/ParseBuffer.hxx"

using namespace resip;

UnknownHeaderType::UnknownHeaderType(const char* name)
{
   assert(name);
   ParseBuffer pb(name, strlen(name));
   const char* anchor = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mName = pb.data(anchor);
   assert(!mName.empty());
   assert(Headers::getType(mName.data(), mName.size()) == Headers::UNKNOWN);
}

UnknownHeaderType::UnknownHeaderType(const Data& name)
{
   ParseBuffer pb(name);
   const char* anchor = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mName = pb.data(anchor);
   assert(!mName.empty());
   assert(Headers::getType(mName.data(), mName.size()) == Headers::UNKNOWN);
}
