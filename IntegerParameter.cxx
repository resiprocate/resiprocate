#include <sipstack/IntegerParameter.hxx>
#include <sipstack/Symbols.hxx>

using namespace Vocal2;
using namespace std;

IntegerParameter::IntegerParameter(ParameterTypes::Type type,
                                   const char* startData, unsigned int dataSize)
   : Parameter(type),
     mValue(0)
{
   mValue = atoi(startData);
}

IntegerParameter::IntegerParameter(ParameterTypes::Type type, int value)
   : Parameter(type),
     mValue(value)
{}
      
int& IntegerParameter::value()
{
   return mValue;
}

Parameter* 
IntegerParameter::clone() const
{
   return new IntegerParameter(*this);
}

ostream&
IntegerParameter::encode(ostream& stream) const
{
   return stream << getName() << Symbols::EQUALS << mValue;
}
