#include "UnknownHeaderType.hxx"
#include "HeaderTypes.hxx"

#include <cassert>

using namespace Vocal2;

UnknownHeaderType::UnknownHeaderType(const char* name)
{
   assert(name);
   mName = name;
   assert(Headers::getType(mName.data(), mName.size()) == Headers::UNKNOWN);
}
