#include "UnknownParameterType.hxx"
#include "ParameterTypeEnums.hxx"

#include <cassert>

using namespace Vocal2;

UnknownParameterType::UnknownParameterType(const char* name)
{
   assert(name);
   mName = name;
   assert(ParameterTypes::getType(mName.data(), mName.size()) == ParameterTypes::UNKNOWN);
}
