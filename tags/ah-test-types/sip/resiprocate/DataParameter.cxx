#include <sipstack/DataParameter.hxx>
#include <sipstack/Symbols.hxx>

using namespace Vocal2;
using namespace std;

DataParameter::DataParameter(ParameterTypes::Type type,
                             const char* startData, unsigned int dataSize)
   : Parameter(type), 
     mData(startData, dataSize)
{}

DataParameter::DataParameter(ParameterTypes::Type type)
   : Parameter(type) 
{}

Data& 
DataParameter::value()
{
   return mData;
}

Parameter* 
DataParameter::clone() const
{
   return new DataParameter(*this);
}

ostream& 
DataParameter::encode(ostream& stream) const
{
   return stream << getName() << Symbols::EQUALS << mData;
}
