#include <sipstack/IntegerParameter.hxx>
#include <sipstack/Symbols.hxx>
#include <util/ParseBuffer.hxx>
#include <sipstack/ParseException.hxx>

using namespace Vocal2;
using namespace std;

IntegerParameter::IntegerParameter(ParameterTypes::Type type,
                                   ParseBuffer& pb)
   : Parameter(type),
     mValue(0)
{
   if (*pb.position() != '=')
   {
      throw ParseException("parameter constructor expected '='", __FILE__, __LINE__);
   }
   pb.skipChar();
   // .dlb. error detection?
   mValue = pb.integer();
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
