#include <sipstack/FloatParameter.hxx>
#include <sipstack/Symbols.hxx>

using namespace Vocal2;
using namespace std;

FloatParameter::FloatParameter(ParameterTypes::Type type, 
                               const char* startData, unsigned int dataSize)
   : Parameter(type),
     mValue(0.0)
{
   mValue = atof(startData);
}

FloatParameter::FloatParameter(ParameterTypes::Type type, float value)
   : Parameter(type),
     mValue(value)
{}
      
float& FloatParameter::value()
{
   return mValue;
}

Parameter* 
FloatParameter::clone() const
{
   return new FloatParameter(*this);
}

ostream&
FloatParameter::encode(ostream& stream) const
{
   return stream << getName() << Symbols::EQUALS << mValue;
}
