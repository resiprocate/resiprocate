#include <sipstack/ExistsParameter.hxx>

using namespace Vocal2;
using namespace std;

ExistsParameter::ExistsParameter(ParameterTypes::Type type, const char* startData, unsigned int dataSize)
   : Parameter(type),
     mValue(true)
{}

ExistsParameter::ExistsParameter(ParameterTypes::Type type)
   : Parameter(type),
     mValue(true)
{}
      
bool& ExistsParameter::value()
{
   return mValue;
}

Parameter* 
ExistsParameter::clone() const
{
   return new ExistsParameter(*this);
}

ostream&
ExistsParameter::encode(ostream& stream) const
{
   if (mValue)
   {
      stream << getName();
   }
   return stream;
}





