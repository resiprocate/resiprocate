#include <cassert>
#include <sipstack/DataParameter.hxx>
#include <sipstack/Symbols.hxx>
#include <util/ParseBuffer.hxx>
#include <sipstack/ParseException.hxx>

using namespace Vocal2;
using namespace std;

DataParameter::DataParameter(ParameterTypes::Type type,
                             ParseBuffer& pb)
   : Parameter(type), 
     mValue(),
     mQuoted(false)
{
   if (*pb.position() != '=')
   {
      throw ParseException("parameter constructor expected '='", __FILE__, __LINE__);
   }
   pb.skipChar();
   pb.skipWhitespace(); // .dlb. space allowed only before "
   if (*pb.position() == '"')
   {
      setQuoted(true);
      pb.skipChar();
      const char* pos = pb.position();
      pb.skipToEndQuote();
      mValue = Data(pos, pb.position() - pos);
      pb.skipChar();
   }
   else
   {
      const char* pos = pb.position();
      static const char* WhitespaceOrParamTerm = " \t\r\n;?";
      pb.skipToOneOf(WhitespaceOrParamTerm);
      mValue = Data(pos, pb.position() - pos);
   }
}

DataParameter::DataParameter(ParameterTypes::Type type)
   : Parameter(type),
     mValue(),
     mQuoted(false)
{
}

Data& 
DataParameter::value()
{
   return mValue;
}

Parameter* 
DataParameter::clone() const
{
   return new DataParameter(*this);
}

ostream& 
DataParameter::encode(ostream& stream) const
{
   if (mQuoted)
   {
      return stream << getName() << Symbols::EQUALS << Symbols::DOUBLE_QUOTE << mValue << Symbols::DOUBLE_QUOTE;
   }
   else
   {
      return stream << getName() << Symbols::EQUALS << mValue;
   }
}
