#include <sipstack/ExistsParameter.hxx>
#include <util/ParseBuffer.hxx>

using namespace Vocal2;
using namespace std;

ExistsParameter::ExistsParameter(ParameterTypes::Type type, ParseBuffer& pb)
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





