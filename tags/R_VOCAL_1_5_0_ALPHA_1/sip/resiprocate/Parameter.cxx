#include <sipstack/Parameter.hxx>

using namespace Vocal2;
using namespace std;

Parameter::Parameter(ParameterTypes::Type type)
   : mType(type)
{}


ParameterTypes::Type 
Parameter::getType() const
{
   return mType;
}


const Data& 
Parameter::getName() const
{
   return ParameterTypes::ParameterNames[mType];
}

