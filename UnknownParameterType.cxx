#include "UnknownParameterType.hxx"
#include "ParameterTypeEnums.hxx"

#include <cassert>
#include <string.h>
#include "resiprocate/util/ParseBuffer.hxx"

using namespace Vocal2;

UnknownParameterType::UnknownParameterType(const char* name)
{
   assert(name);
   ParseBuffer pb(name, strlen(name));
   const char* anchor = pb.skipWhitespace();
   pb.skipNonWhitespace();
   mName = pb.data(anchor);
   assert(!mName.empty());
   assert(ParameterTypes::getType(mName.data(), mName.size()) == ParameterTypes::UNKNOWN);
}
