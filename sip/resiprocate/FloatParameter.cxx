#include <sipstack/FloatParameter.hxx>
#include <sipstack/ParseException.hxx>
#include <sipstack/Symbols.hxx>
#include <util/ParseBuffer.hxx>

using namespace Vocal2;
using namespace std;

FloatParameter::FloatParameter(ParameterTypes::Type type, 
                               ParseBuffer& pb)
   : Parameter(type)
{
   if (*pb.position() != '=')
   {
      throw ParseException("parameter constructor expected '='", __FILE__, __LINE__);
   }
   pb.skipChar();

   // .dlb. not zero terminated; no error detection
   mValue = atof(pb.position());
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
